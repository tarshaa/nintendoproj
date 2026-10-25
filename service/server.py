#!/usr/bin/env python3
"""Single-device prototype service. HTTP binds only loopback; LAN requires TLS.

POST /v1/probe -> exact synthetic bytes (max256)
POST /v1/worlds -> camera .pwg bytes, returns a 32-character job id
GET /v1/jobs/<id> -> 202 pending, 200 PWL1 package, or 422 failure
All /v1 endpoints require the paired device bearer token, never the model API key.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import hmac
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import re
import secrets
import ssl
import threading
import time
from config import PRIVATE, private_write, settings
from photo import SIZE, decode
from recipe import pack
from runtime_check import verify
from vision import generate


class Jobs:
    def __init__(self, config, root=PRIVATE / "jobs", generator=generate, limit=10):
        self.config, self.root, self.generator, self.limit = config, root, generator, limit
        self.lock, self.pool = threading.Lock(), ThreadPoolExecutor(max_workers=1)
        self.jobs, self.digests = {}, {}

    def submit(self, photo):
        png, checksum = decode(photo)
        digest = hashlib.sha256(photo).hexdigest()
        with self.lock:
            old = self.digests.get(digest)
            if old and self.jobs[old]["state"] != "failed":
                return old
            if not self.config.get("api_key"):
                raise ValueError("API setup needed on the Mac")
            # A conservative local cap, persisted across process restarts.
            day = time.strftime("%Y-%m-%d", time.gmtime())
            ledger = self.root / "attempts.json"
            counts = json.loads(ledger.read_text()) if ledger.exists() else {}
            if counts.get(day, 0) >= self.limit:
                raise ValueError("Today's test limit reached; increase it explicitly on the Mac")
            if any(j["state"] == "pending" for j in self.jobs.values()):
                raise ValueError("Another photo is being generated; wait and retry")
            counts[day] = counts.get(day, 0) + 1
            private_write(ledger, json.dumps(counts).encode())
            job_id = secrets.token_hex(16)
            self.jobs[job_id] = {"state": "pending", "started": time.monotonic(), "digest": digest}
            self.digests[digest] = job_id
            folder = self.root / job_id
            private_write(folder / "source.pwg", photo)
            private_write(folder / "photo.png", png)
            self.pool.submit(self.run, job_id, png, checksum)
            return job_id

    def run(self, job_id, png, checksum):
        try:
            recipe, provenance = self.generator(png, self.config)
            package = pack(recipe, checksum)
            verify(package)
            folder = self.root / job_id
            private_write(folder / "world.pwl", package)
            private_write(folder / "recipe.json", json.dumps(recipe, indent=2).encode())
            with self.lock:
                job = self.jobs[job_id]
                seconds = round(time.monotonic() - job["started"], 3)
                private_write(folder / "result.json", json.dumps({"source_sha256": job["digest"],
                              "server_seconds": seconds, "provenance": provenance,
                              "evidence": "Generated and structurally validated; hardware playtest pending"}, indent=2).encode())
                job.update(state="ready", package=package, seconds=seconds)
        except Exception as error:
            safe = str(error)[:180] if isinstance(error, ValueError) else "Generation interrupted; your photo remains saved. Retry on the DSi."
            with self.lock:
                self.jobs[job_id].update(state="failed", error=safe)

    def get(self, job_id):
        with self.lock:
            job = self.jobs.get(job_id)
            return dict(job) if job else None


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def setup(self):
        super().setup()
        self.connection.settimeout(15)

    def log_message(self, *_):
        pass  # URLs, request bodies and credentials are never printed.

    def reply(self, status, data, content_type="text/plain"):
        if isinstance(data, str): data = data.encode()
        self.send_response(status)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Content-Type", content_type)
        self.send_header("Cache-Control", "no-store")
        self.send_header("Connection", "close")
        self.end_headers()
        self.wfile.write(data)
        self.close_connection = True

    def authorized(self):
        token = self.server.jobs.config.get("device_token", "")
        return bool(token) and hmac.compare_digest(self.headers.get("Authorization", ""), "Bearer " + token)

    def do_GET(self):
        if self.path == "/health":
            return self.reply(200, "Photo Worlds service\n")
        if not self.authorized(): return self.reply(401, "Pair this device first")
        match = re.fullmatch(r"/v1/jobs/([a-f0-9]{32})", self.path)
        if not match: return self.reply(404, "Not found")
        job = self.server.jobs.get(match[1])
        if not job: return self.reply(404, "Unknown job; resubmit the saved photo")
        if job["state"] == "pending": return self.reply(202, "Interpreting photo and checking routes")
        if job["state"] == "failed": return self.reply(422, job["error"])
        return self.reply(200, job["package"], "application/octet-stream")

    def do_POST(self):
        if not self.authorized(): return self.reply(401, "Pair this device first")
        if self.path not in ("/v1/probe", "/v1/worlds"): return self.reply(404, "Not found")
        if self.headers.get("Transfer-Encoding"): return self.reply(400, "Content-Length required")
        try:
            length = int(self.headers.get("Content-Length", "-1"))
            if self.path == "/v1/probe":
                if not 1 <= length <= 256: return self.reply(413, "Probe size invalid")
            elif length not in (SIZE, SIZE + 4):
                return self.reply(413, "Capture size invalid")
            body = self.rfile.read(length)
            if len(body) != length: return self.reply(400, "Incomplete upload")
            if self.path == "/v1/probe": return self.reply(200, body, "application/octet-stream")
            return self.reply(202, self.server.jobs.submit(body))
        except ValueError as error:
            return self.reply(422, str(error)[:180])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8788)
    parser.add_argument("--cert"); parser.add_argument("--key")
    args = parser.parse_args()
    if args.host != "127.0.0.1" and not (args.cert and args.key):
        parser.error("LAN access requires a TLS certificate; loopback can sit behind an HTTPS tunnel")
    config = settings()
    if not config["device_token"]:
        raise SystemExit("Run setup_key.py before starting the service")
    server = ThreadingHTTPServer((args.host, args.port), Handler)
    server.jobs = Jobs(config)
    if args.cert and args.key:
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.minimum_version = ssl.TLSVersion.TLSv1_2
        context.load_cert_chain(args.cert, args.key)
        server.socket = context.wrap_socket(server.socket, server_side=True)
    print("Photo Worlds service started. Credentials and photos are kept private.", flush=True)
    try: server.serve_forever()
    finally:
        server.server_close()
        server.jobs.pool.shutdown(wait=True)


if __name__ == "__main__":
    main()
