#!/usr/bin/env python3
"""
EcoSort Confusion Matrix + Confidence Intervals
================================================
Reads a trial results CSV and outputs the 2x2 confusion matrix,
per-class accuracy, and Wilson confidence intervals.

Usage:
  python3 evaluation/confusion_matrix.py \
      --input evaluation/trial_results.csv \
      --output evaluation/results/

CSV format: trial_id, item_type, true_class, predicted_class, sensor_notes
  true_class / predicted_class: TRASH or RECYCLE
"""
import argparse, csv, json, math
from collections import defaultdict
from pathlib import Path

def wilson_ci(k, n, z=1.96):
    """Wilson score interval for proportion k/n at confidence level z."""
    if n == 0: return (0.0, 0.0)
    p = k / n
    denom = 1 + z**2 / n
    center = (p + z**2 / (2*n)) / denom
    margin = (z * math.sqrt(p*(1-p)/n + z**2/(4*n**2))) / denom
    return (max(0, center - margin), min(1, center + margin))

def run(input_csv, output_dir):
    trials = []
    with open(input_csv) as f:
        reader = csv.DictReader(f)
        for row in reader:
            if row.get("excluded", "").lower() in ("1", "true", "yes"): continue
            trials.append(row)

    classes = ["TRASH", "RECYCLE"]
    cm = defaultdict(int)  # (true, pred) -> count
    for t in trials:
        tc, pc = t["true_class"].strip().upper(), t["predicted_class"].strip().upper()
        cm[(tc, pc)] += 1

    n = len(trials)
    correct = sum(cm[(c,c)] for c in classes)
    accuracy = correct / n if n else 0

    lo, hi = wilson_ci(correct, n)
    print(f"\nEcoSort Evaluation  (N={n} trials)")
    print(f"Overall accuracy: {accuracy*100:.1f}%  95% CI: [{lo*100:.1f}%, {hi*100:.1f}%]\n")
    print(f"{'':>12} {'pred TRASH':>12} {'pred RECYCLE':>14}")
    for tc in classes:
        row_str = f"{tc:>12}"
        for pc in classes:
            row_str += f"{cm[(tc,pc)]:>14}"
        print(row_str)

    results = {"n_trials": n, "accuracy": accuracy,
               "ci_95_low": lo, "ci_95_high": hi,
               "confusion_matrix": {f"{t}_{p}": cm[(t,p)] for t in classes for p in classes}}
    Path(output_dir).mkdir(parents=True, exist_ok=True)
    out = Path(output_dir) / "confusion_matrix.json"
    with open(out, "w") as f:
        json.dump(results, f, indent=2)
    print(f"\nSaved: {out}")

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", default="evaluation/trial_results.csv")
    ap.add_argument("--output", default="evaluation/results/")
    args = ap.parse_args()
    run(args.input, args.output)
