#!/usr/bin/env python3
# =============================================================================
# Compages: A C++20 OpenGL wrapper.
# Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
#
# This file is part of Compages.
#
# Compages is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Compages is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Compages.  If not, see <http://www.gnu.org/licenses/>.
# =============================================================================
"""Write Shotgun.glb, the animated weapon of 53_DoomLike (standard library only).

The gun is made of boxes and points down -Z, the way a camera looks, in
metres. Its clips are Idle (a slow sway, looped), Fire (the kick and the pump)
and Reload (tilted aside, three shells pushed in, a pump). The node animated is
"Shotgun", under a still "ShotgunRig" that the game places in front of the eye.

    python3 make_shotgun.py [output.glb]
"""

from __future__ import annotations

import json
import math
import struct
import sys
from pathlib import Path


def quat_x(degrees: float) -> list[float]:
    h = math.radians(degrees) * 0.5
    return [math.sin(h), 0.0, 0.0, math.cos(h)]


def quat_xz(x_degrees: float, z_degrees: float) -> list[float]:
    """A turn around X followed by one around Z."""
    hx = math.radians(x_degrees) * 0.5
    hz = math.radians(z_degrees) * 0.5
    ax, aw = math.sin(hx), math.cos(hx)
    cz, cw = math.sin(hz), math.cos(hz)
    # q = qz * qx
    return [cw * ax, cz * ax, cz * aw, cw * aw]


def box(lo: tuple[float, float, float], hi: tuple[float, float, float]):
    """24 corners (4 per face, for sharp normals) and 36 indices."""
    (x0, y0, z0), (x1, y1, z1) = lo, hi
    faces = [
        ((0, 0, 1), [(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)]),
        ((0, 0, -1), [(x1, y0, z0), (x0, y0, z0), (x0, y1, z0), (x1, y1, z0)]),
        ((1, 0, 0), [(x1, y0, z1), (x1, y0, z0), (x1, y1, z0), (x1, y1, z1)]),
        ((-1, 0, 0), [(x0, y0, z0), (x0, y0, z1), (x0, y1, z1), (x0, y1, z0)]),
        ((0, 1, 0), [(x0, y1, z1), (x1, y1, z1), (x1, y1, z0), (x0, y1, z0)]),
        ((0, -1, 0), [(x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1)]),
    ]
    positions, normals, uvs, indices = [], [], [], []
    for normal, corners in faces:
        base = len(positions)
        for corner, uv in zip(corners, [(0, 0), (1, 0), (1, 1), (0, 1)]):
            positions.append(corner)
            normals.append(normal)
            uvs.append(uv)
        indices += [base, base + 1, base + 2, base, base + 2, base + 3]
    return positions, normals, uvs, indices


class Writer:
    """Packs buffers, accessors and views into one binary chunk."""

    def __init__(self) -> None:
        self.blob = bytearray()
        self.views: list[dict] = []
        self.accessors: list[dict] = []

    def _view(self, data: bytes, target: int | None) -> int:
        while len(self.blob) % 4:
            self.blob.append(0)
        view = {"buffer": 0, "byteOffset": len(self.blob), "byteLength": len(data)}
        if target is not None:
            view["target"] = target
        self.blob += data
        self.views.append(view)
        return len(self.views) - 1

    def floats(self, rows: list, kind: str, target: int | None = 34962) -> int:
        width = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}[kind]
        flat = [float(v) for row in rows for v in (row if width > 1 else [row])]
        view = self._view(struct.pack(f"<{len(flat)}f", *flat), target)
        columns = [flat[i::width] for i in range(width)]
        self.accessors.append({
            "bufferView": view, "componentType": 5126, "count": len(rows),
            "type": kind, "min": [min(c) for c in columns], "max": [max(c) for c in columns],
        })
        return len(self.accessors) - 1

    def indices(self, values: list[int]) -> int:
        view = self._view(struct.pack(f"<{len(values)}H", *values), 34963)
        self.accessors.append({
            "bufferView": view, "componentType": 5123, "count": len(values), "type": "SCALAR",
        })
        return len(self.accessors) - 1


def main() -> None:
    output = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("Shotgun.glb")
    w = Writer()

    materials = [
        {"name": "Gunmetal", "pbrMetallicRoughness": {
            "baseColorFactor": [0.10, 0.10, 0.11, 1.0], "metallicFactor": 0.9, "roughnessFactor": 0.35}},
        {"name": "Walnut", "pbrMetallicRoughness": {
            "baseColorFactor": [0.42, 0.22, 0.09, 1.0], "metallicFactor": 0.0, "roughnessFactor": 0.7}},
        {"name": "Brass", "pbrMetallicRoughness": {
            "baseColorFactor": [0.85, 0.62, 0.22, 1.0], "metallicFactor": 1.0, "roughnessFactor": 0.3}},
    ]

    # (name, material, [boxes]) in the space of the animated node.
    parts = [
        ("Receiver", 0, [((-0.032, -0.055, -0.10), (0.032, 0.022, 0.14))]),
        ("Barrel", 0, [((-0.017, 0.000, -0.58), (0.017, 0.034, -0.10)),
                       ((-0.014, -0.034, -0.52), (0.014, -0.002, -0.10))]),
        ("Sight", 2, [((-0.004, 0.034, -0.575), (0.004, 0.044, -0.56))]),
        ("Grip", 1, [((-0.024, -0.16, 0.07), (0.024, -0.045, 0.13))]),
        ("Stock", 1, [((-0.028, -0.085, 0.14), (0.028, 0.012, 0.42))]),
        ("Pump", 1, [((-0.030, -0.050, -0.40), (0.030, 0.006, -0.22))]),
    ]

    meshes, nodes, part_nodes = [], [], {}
    for name, material, boxes in parts:
        positions, normals, uvs, indices = [], [], [], []
        for lo, hi in boxes:
            p, n, t, i = box(lo, hi)
            indices += [len(positions) + k for k in i]
            positions += p
            normals += n
            uvs += t
        primitive = {
            "attributes": {
                "POSITION": w.floats(positions, "VEC3"),
                "NORMAL": w.floats(normals, "VEC3"),
                "TEXCOORD_0": w.floats(uvs, "VEC2"),
            },
            "indices": w.indices(indices),
            "material": material,
        }
        meshes.append({"name": name, "primitives": [primitive]})
        nodes.append({"name": name, "mesh": len(meshes) - 1})
        part_nodes[name] = len(nodes) - 1

    # Where the shot leaves, for the flash and the tracers.
    nodes.append({"name": "Muzzle", "translation": [0.0, 0.017, -0.60]})
    muzzle = len(nodes) - 1

    children = [i for i in range(len(nodes))]
    nodes.append({"name": "Shotgun", "children": children})
    gun = len(nodes) - 1
    nodes.append({"name": "ShotgunRig", "children": [gun]})
    rig = len(nodes) - 1
    pump = part_nodes["Pump"]
    del muzzle

    animations = []

    def clip(name: str, channels: list[tuple[int, str, list[float], list]]) -> None:
        samplers, targets = [], []
        for node, path, times, values in channels:
            kind = "VEC4" if path == "rotation" else "VEC3"
            samplers.append({
                "input": w.floats(times, "SCALAR", None),
                "output": w.floats(values, kind, None),
                "interpolation": "LINEAR",
            })
            targets.append({"sampler": len(samplers) - 1, "target": {"node": node, "path": path}})
        animations.append({"name": name, "samplers": samplers, "channels": targets})

    still = [0.0, 0.0, 0.0]
    level = [0.0, 0.0, 0.0, 1.0]

    # Idle: breathing, two seconds, back where it started.
    clip("Idle", [
        (gun, "translation", [0.0, 1.0, 2.0], [still, [0.0, 0.006, 0.0], still]),
        (gun, "rotation", [0.0, 1.0, 2.0], [level, quat_x(1.2), level]),
        (pump, "translation", [0.0, 2.0], [still, still]),
    ])

    # Fire: the kick up and back, then the pump racked.
    clip("Fire", [
        (gun, "translation", [0.0, 0.04, 0.22, 0.55],
         [still, [0.0, 0.025, 0.09], [0.0, 0.004, 0.01], still]),
        (gun, "rotation", [0.0, 0.04, 0.25, 0.55],
         [level, quat_x(14.0), quat_x(2.0), level]),
        (pump, "translation", [0.0, 0.22, 0.34, 0.48, 0.55],
         [still, still, [0.0, 0.0, 0.10], still, still]),
    ])

    # Reload: aside and down, three shells pushed in, a pump, back.
    down = [0.03, -0.07, 0.02]
    push = [0.03, -0.06, 0.03]
    clip("Reload", [
        (gun, "translation", [0.0, 0.25, 0.40, 0.50, 0.60, 0.70, 0.80, 0.90, 1.15, 1.40],
         [still, down, push, down, push, down, push, down, down, still]),
        (gun, "rotation", [0.0, 0.25, 0.95, 1.40],
         [level, quat_xz(-12.0, 35.0), quat_xz(-12.0, 35.0), level]),
        (pump, "translation", [0.0, 0.95, 1.05, 1.15, 1.40],
         [still, still, [0.0, 0.0, 0.10], still, still]),
    ])

    document = {
        "asset": {"version": "2.0", "generator": "Compages make_shotgun.py"},
        "scene": 0,
        "scenes": [{"nodes": [rig]}],
        "nodes": nodes,
        "meshes": meshes,
        "materials": materials,
        "animations": animations,
        "accessors": w.accessors,
        "bufferViews": w.views,
        "buffers": [{"byteLength": len(w.blob)}],
    }

    text = json.dumps(document, separators=(",", ":")).encode()
    text += b" " * (-len(text) % 4)
    binary = bytes(w.blob) + b"\0" * (-len(w.blob) % 4)
    length = 12 + 8 + len(text) + 8 + len(binary)
    with output.open("wb") as out:
        out.write(struct.pack("<III", 0x46546C67, 2, length))
        out.write(struct.pack("<II", len(text), 0x4E4F534A) + text)
        out.write(struct.pack("<II", len(binary), 0x004E4942) + binary)
    print(f"wrote {output}")


if __name__ == "__main__":
    main()
