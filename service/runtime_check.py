"""Execute the same C movement/collision code used by the DSi before release."""
import hashlib
from pathlib import Path
import subprocess
import tempfile
from config import PRIVATE

PROJECT = Path(__file__).resolve().parents[1]


def verify(package):
    sources = [PROJECT / p for p in ("source/generated_world.c", "source/generated_world.h",
                                     "source/generated_render.c", "tests/generated_check.c")]
    digest = hashlib.sha256(b"".join(p.read_bytes() for p in sources)).hexdigest()[:16]
    folder = PRIVATE / "checks"
    folder.mkdir(parents=True, exist_ok=True, mode=0o700)
    binary = folder / ("world-check-" + digest)
    if not binary.exists():
        result = subprocess.run(["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                                 str(sources[0]), str(sources[2]), str(sources[3]), "-lm", "-o", str(binary)],
                                capture_output=True, timeout=30)
        if result.returncode:
            (folder / "compile-error.log").write_bytes(result.stderr)
            raise ValueError("Native route validator unavailable on the server")
    with tempfile.NamedTemporaryFile(dir=folder, suffix=".pwl") as f:
        f.write(package);f.flush()
        result = subprocess.run([str(binary), f.name], capture_output=True, timeout=15)
    if result.returncode:
        raise ValueError("The generated route failed the DSi movement test; photo saved for retry")
