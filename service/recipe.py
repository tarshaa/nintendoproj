"""Bounded generated platformer package, shared with source/generated_world.c.

This is a first runtime contract, not an arbitrary generated-code executor.
"""
import math
import struct
import zlib

SHAPES = ["mushroom", "leaf", "book", "box", "fruit", "plate", "cloth"]


def obj(properties):
    return {"type": "object", "properties": properties, "required": list(properties), "additionalProperties": False}


def arr(item, low, high):
    return {"type": "array", "items": item, "minItems": low, "maxItems": high}


TEXT = {"type": "string"}
NUMBER = {"type": "number"}
INTEGER = {"type": "integer"}
SCHEMA = obj({
    "supported": {"type": "boolean"}, "reason": TEXT,
    "title": TEXT, "objective": TEXT,
    "elements": arr(obj({"label": TEXT, "observation": TEXT,
                         "box": arr(NUMBER, 4, 4), "shape": {"type": "string", "enum": SHAPES},
                         "color": arr(INTEGER, 3, 3), "property": TEXT}), 0, 8),
    "invention": TEXT,
    "platforms": arr(obj({"element": INTEGER, "x": NUMBER, "z": NUMBER, "y": NUMBER,
                          "radius": NUMBER, "spring": {"type": "boolean"},
                          "checkpoint": {"type": "boolean"}}), 0, 40),
    "goal": INTEGER,
})


def require(test, message):
    if not test:
        raise ValueError(message)


def text(value, limit):
    require(isinstance(value, str) and 0 < len(value) <= limit, "Text length is unsupported")
    require(all(32 <= ord(c) <= 126 for c in value), "Use printable basic characters")
    return value.encode().ljust(limit + 1, b"\0")


def finite(value, low, high):
    require(type(value) in (int, float) and math.isfinite(value) and low <= value <= high,
            "World geometry is outside the playable bounds")


def validate(recipe):
    require(type(recipe) is dict and set(recipe) == set(SCHEMA["properties"]), "Unexpected recipe fields")
    require(recipe["supported"] is True, "Photo needs clearer supported subjects; try a different composition")
    text(recipe["title"], 47); text(recipe["objective"], 79)
    require(isinstance(recipe["invention"], str) and 0 < len(recipe["invention"]) <= 1600, "Missing invented-mechanics explanation")
    elements = recipe["elements"]
    require(isinstance(elements, list) and 2 <= len(elements) <= 8, "Need at least two identifiable photo elements")
    for e in elements:
        require(type(e) is dict and set(e) == {"label", "observation", "box", "shape", "color", "property"}, "Invalid element")
        text(e["label"], 23)
        require(e["shape"] in SHAPES, "Unsupported silhouette")
        require(all(isinstance(e[k], str) and 0 < len(e[k]) <= 800 for k in ("observation", "property")), "Missing visual evidence")
        require(len(e["box"]) == 4 and len(e["color"]) == 3, "Invalid element appearance")
        for v in e["box"]: finite(v, 0, 1)
        x, y, w, h = e["box"]
        require(w > 0 and h > 0 and x + w <= 1.001 and y + h <= 1.001, "Photo bounds invalid")
        for v in e["color"]: require(type(v) is int and 0 <= v <= 255, "Invalid color")
    nodes = recipe["platforms"]
    require(isinstance(nodes, list) and 8 <= len(nodes) <= 40, "Unsupported platform count")
    require(type(recipe["goal"]) is int and 0 < recipe["goal"] < len(nodes), "Invalid destination")
    for p in nodes:
        require(type(p) is dict and set(p) == {"element", "x", "z", "y", "radius", "spring", "checkpoint"}, "Invalid platform fields")
        require(type(p["element"]) is int and 0 <= p["element"] < len(elements), "Missing photo link")
        for k in ("x", "z"): finite(p[k], -80, 80)
        finite(p["y"], 0, 40); finite(p["radius"], 2, 4)
        require(type(p["spring"]) is bool and type(p["checkpoint"]) is bool, "Invalid mechanic")
        require(not p["spring"] or elements[p["element"]]["shape"] == "leaf", "Spring must be a flexible leaf")
    require(nodes[0]["checkpoint"] and nodes[recipe["goal"]]["checkpoint"], "Start and destination need safe rest")
    require(len({p["element"] for p in nodes}) >= 2, "Subjects must affect the actual world")
    for i, a in enumerate(nodes):
        for b in nodes[i + 1:]:
            distance = math.hypot(a["x"] - b["x"], a["z"] - b["z"])
            require(distance >= a["radius"] + b["radius"] + .25, "Overlapping platforms")
    # Conservative center-to-center reach; centers are flat collision/landing targets.
    edges = [[] for _ in nodes]
    for i, a in enumerate(nodes):
        for j, b in enumerate(nodes):
            dy = b["y"] - a["y"]
            speed = 13.8 if a["spring"] else 10.8
            discr = speed * speed - 48 * dy
            if i != j and discr >= 0 and -2.5 <= dy <= (3.0 if a["spring"] else 1.6):
                travel = (speed + math.sqrt(discr)) / 24
                if math.hypot(a["x"] - b["x"], a["z"] - b["z"]) <= 6.2 * travel:
                    edges[i].append(j)
    def reachable(start, removed=-1):
        seen, stack = {start}, [start]
        while stack:
            for n in edges[stack.pop()]:
                if n not in seen and n != removed:
                    seen.add(n); stack.append(n)
        return seen
    goal = recipe["goal"]
    require(len(reachable(0)) == len(nodes), "Unreachable area")
    require(all(goal in reachable(i) for i in range(len(nodes))), "A route has no way to the goal")
    require(all(goal in reachable(0, i) for i in range(1, len(nodes)) if i != goal),
            "Need two alternative routes; a required intermediate platform makes this linear")
    require(any(p["checkpoint"] for p in nodes[1:] if p is not nodes[goal]), "Add a mid-route rest")
    return edges


def pack(recipe, photo_checksum):
    validate(recipe)
    nodes, elements = recipe["platforms"], recipe["elements"]
    payload = struct.pack("<IHH", photo_checksum, len(nodes), recipe["goal"])
    payload += text(recipe["title"], 47) + text(recipe["objective"], 95)
    for p in nodes:
        e = elements[p["element"]]
        r, g, b = (c >> 3 for c in e["color"])
        flags = int(p["checkpoint"]) | (int(p["spring"]) << 1)
        payload += struct.pack("<ffffHBB24s", p["x"], p["z"], p["y"], p["radius"],
                               r | g << 5 | b << 10, SHAPES.index(e["shape"]), flags, text(e["label"], 23))
    return b"PWL1" + struct.pack("<II", len(payload), zlib.crc32(payload) & 0xFFFFFFFF) + payload
