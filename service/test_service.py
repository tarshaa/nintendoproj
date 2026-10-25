import copy
from http.client import HTTPConnection
import json
from pathlib import Path
import struct
import tempfile
import threading
import time
import unittest
from http.server import ThreadingHTTPServer
from photo import PIXELS, decode
from recipe import pack, validate
from runtime_check import verify
from server import Handler, Jobs


def capture():
    pixels = [0x801F, 0x83E0, 0xFC00] + [0xFFFF] * (PIXELS - 3)
    checksum = 2166136261
    for p in pixels: checksum = ((checksum ^ p) * 16777619) & 0xFFFFFFFF
    return struct.pack("<III", 0x50574731, PIXELS, checksum) + struct.pack(f"<{PIXELS}H", *pixels)


def fixture():
    # Deliberately synthetic. Never used as a fallback for a real photo.
    elements = [dict(label="Book", observation="Synthetic test only", box=[.1,.1,.3,.3],
                     shape="book", color=[110,170,200], property="Broad rigid cover"),
                dict(label="Mushroom", observation="Synthetic test only", box=[.6,.3,.3,.5],
                     shape="mushroom", color=[190,110,75], property="Rounded raised cap")]
    positions = [(0,0,0),(-3,4.5,.25),(3,4.5,.25),(-3,9,.5),(3,9,.5),(-3,13.5,.75),(3,13.5,.75),(0,18,1)]
    nodes = [dict(element=i%2, x=x,z=z,y=y,radius=2,spring=False,checkpoint=i in (0,3,4,7)) for i,(x,z,y) in enumerate(positions)]
    return dict(supported=True,reason="",title="Synthetic traversal test",objective="Reach the high book.",
                elements=elements,invention="Synthetic geometry for protocol tests, not interpreted from a photo.",platforms=nodes,goal=7)


class RecipeTests(unittest.TestCase):
    def test_native_physics_and_package(self):
        png, checksum = decode(capture())
        self.assertTrue(png.startswith(b"\x89PNG"))
        package = pack(fixture(), checksum)
        self.assertEqual(len(package), 164 + 44 * 8)
        verify(package)

    def test_bad_worlds_do_not_silently_fall_back(self):
        for change in (lambda r:r.update(supported=False),
                       lambda r:r["platforms"][1].update(x=0,z=0),
                       lambda r:r["platforms"][1].update(x=float("nan")),
                       lambda r:r["platforms"][4].update(z=80),
                       lambda r:r["platforms"][2].update(spring=True),
                       lambda r:r["elements"][0].update(box=[.9,0,.5,.5])):
            r=fixture();change(r)
            with self.assertRaises(ValueError): validate(r)
        bad=bytearray(capture());bad[30]^=1
        with self.assertRaises(ValueError): decode(bytes(bad))

    def test_changed_geometry_changes_package(self):
        a=fixture();b=copy.deepcopy(a)
        for p in b["platforms"]: p["x"]=-p["x"]+1
        self.assertNotEqual(pack(a,1),pack(b,1))


class HTTPTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.calls=0
        def generate(_png,_config):
            self.calls+=1
            return fixture(),{"test_only":True}
        self.jobs=Jobs({"api_key":"synthetic-test-key","device_token":"a"*64},Path(self.temp.name),generator=generate)
        self.server=ThreadingHTTPServer(("127.0.0.1",0),Handler);self.server.jobs=self.jobs
        self.thread=threading.Thread(target=self.server.serve_forever,daemon=True);self.thread.start()

    def tearDown(self):
        self.server.shutdown();self.server.server_close();self.jobs.pool.shutdown();self.temp.cleanup()

    def request(self,method,path,body=None,authorized=True):
        conn=HTTPConnection("127.0.0.1",self.server.server_port,timeout=10)
        headers={"Authorization":"Bearer "+"a"*64} if authorized else {}
        conn.request(method,path,body,headers);response=conn.getresponse();result=(response.status,response.read());conn.close();return result

    def test_upload_poll_native_validation_and_duplicate(self):
        self.assertEqual(self.request("POST","/v1/probe",b"test",False)[0],401)
        self.assertEqual(self.request("POST","/v1/probe",b"synthetic round trip"),(200,b"synthetic round trip"))
        data=capture();status,job=self.request("POST","/v1/worlds",data);self.assertEqual(status,202)
        for _ in range(100):
            status,result=self.request("GET","/v1/jobs/"+job.decode())
            if status!=202:break
            time.sleep(.02)
        self.assertEqual(status,200,result);self.assertTrue(result.startswith(b"PWL1"))
        verify(result)
        self.assertEqual(self.request("POST","/v1/worlds",data),(202,job));self.assertEqual(self.calls,1)
        self.assertEqual(self.request("GET","/v1/jobs/"+job.decode(),authorized=False)[0],401)

    def test_corrupt_capture_and_missing_api_are_explicit(self):
        bad=bytearray(capture());bad[20]^=1
        self.assertEqual(self.request("POST","/v1/worlds",bytes(bad))[0],422)
        self.jobs.config["api_key"]=""
        status,result=self.request("POST","/v1/worlds",capture());self.assertEqual(status,422)
        self.assertIn(b"API setup",result);self.assertEqual(self.calls,0)

    def test_provider_failure_has_no_ready_package(self):
        def fail(*_): raise ValueError("Synthetic provider failure")
        self.jobs.generator=fail
        status,job=self.request("POST","/v1/worlds",capture());self.assertEqual(status,202)
        for _ in range(100):
            status,result=self.request("GET","/v1/jobs/"+job.decode())
            if status!=202:break
            time.sleep(.01)
        self.assertEqual(status,422);self.assertFalse(result.startswith(b"PWL1"))


if __name__=="__main__":unittest.main()
