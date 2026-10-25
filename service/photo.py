"""Bounded parser for the DSi's existing camera-save format."""
import struct
import zlib

PIXELS = 256 * 192
SIZE = 12 + PIXELS * 2


def decode(data):
    if len(data) not in (SIZE, SIZE + 4):
        raise ValueError("Capture is incomplete")
    magic, count, checksum = struct.unpack_from("<III", data)
    if magic != 0x50574731 or count != PIXELS:
        raise ValueError("Expected a DSi camera capture")
    value = 2166136261
    rgb = bytearray()
    for (pixel,) in struct.iter_unpack("<H", data[12:SIZE]):
        value = ((value ^ pixel) * 16777619) & 0xFFFFFFFF
        for shift in (0, 5, 10):
            c = (pixel >> shift) & 31
            rgb.append((c << 3) | (c >> 2))
    if value != checksum:
        raise ValueError("Capture checksum failed")
    def chunk(tag, payload):
        return struct.pack(">I", len(payload)) + tag + payload + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)
    rows = b"".join(b"\0" + rgb[y * 768:(y + 1) * 768] for y in range(192))
    png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 256, 192, 8, 2, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))
    return png, checksum
