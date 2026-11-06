#!/usr/bin/env python3
"""Trace un nuage de points (x, y) depuis un fichier CSV."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path

import matplotlib.pyplot as plt


def load_xy(path: Path) -> tuple[list[float], list[float]]:
    with path.open(newline="", encoding="utf-8") as f:
        lines = [ln for ln in f if not ln.lstrip().startswith("#")]
    reader = csv.DictReader(lines)
    if not reader.fieldnames or "x" not in reader.fieldnames or "y" not in reader.fieldnames:
        raise ValueError("Le CSV doit contenir les colonnes x et y")
    xs: list[float] = []
    ys: list[float] = []
    for row in reader:
        xs.append(float(row["x"]))
        ys.append(float(row["y"]))
    return xs, ys


def main() -> None:
    parser = argparse.ArgumentParser(description="Nuage de points x / y")
    parser.add_argument(
        "csv",
        nargs="?",
        type=Path,
        default=Path(__file__).resolve().parent / "galaxy.csv",
        help="Fichier CSV (colonnes x, y ; lignes # ignorées)",
    )
    parser.add_argument("-o", "--output", type=Path, help="Image PNG (sinon affichage interactif)")
    parser.add_argument("--title", default="", help="Titre du graphique")
    args = parser.parse_args()

    if not args.csv.is_file():
        raise SystemExit(f"Fichier introuvable : {args.csv}")

    x, y = load_xy(args.csv)
    if not x:
        raise SystemExit("Aucun point lu (vérifiez les colonnes x, y).")

    fig, ax = plt.subplots(figsize=(8, 8))
    ax.scatter(x, y, s=4, alpha=0.6, c="#2563eb", edgecolors="none")
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_aspect("equal", adjustable="box")
    ax.grid(True, alpha=0.3)
    title = args.title or args.csv.name
    ax.set_title(title)

    if args.output:
        fig.savefig(args.output, dpi=150, bbox_inches="tight")
        print(f"Enregistré : {args.output}")
    else:
        plt.show()


if __name__ == "__main__":
    main()
