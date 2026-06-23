"""Unit-тесты логики выбора подхода (recommender/strategies.py)."""

from __future__ import annotations

import copy
import sys
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "recommender"))

import strategies  # noqa: E402

from fixtures import SAMPLE_PREDICTIONS  # noqa: E402


class SingleMetricStrategyTest(unittest.TestCase):
    def setUp(self) -> None:
        self.predictions = copy.deepcopy(SAMPLE_PREDICTIONS)

    def test_memory_picks_minimum(self) -> None:
        rec = strategies.recommend(self.predictions, "memory")
        self.assertEqual(rec.recommended_approach, "A")
        self.assertEqual(rec.scores["A"], 100.0)

    def test_throughput_picks_maximum(self) -> None:
        rec = strategies.recommend(self.predictions, "throughput")
        self.assertEqual(rec.recommended_approach, "B")
        self.assertEqual(rec.scores["B"], 50_000.0)

    def test_latency_picks_minimum(self) -> None:
        rec = strategies.recommend(self.predictions, "latency")
        self.assertEqual(rec.recommended_approach, "B")

    def test_p99_picks_minimum(self) -> None:
        rec = strategies.recommend(self.predictions, "p99")
        self.assertEqual(rec.recommended_approach, "B")


class WeightedStrategyTest(unittest.TestCase):
    def setUp(self) -> None:
        self.predictions = copy.deepcopy(SAMPLE_PREDICTIONS)

    def test_balanced_prefers_b(self) -> None:
        rec = strategies.recommend(self.predictions, "balanced")
        self.assertEqual(rec.recommended_approach, "B")
        self.assertLess(rec.scores["B"], rec.scores["A"])
        self.assertLess(rec.scores["B"], rec.scores["C"])

    def test_memory_critical_picks_a(self) -> None:
        rec = strategies.recommend(self.predictions, "memory-critical")
        self.assertEqual(rec.recommended_approach, "A")

    def test_throughput_critical_picks_b(self) -> None:
        rec = strategies.recommend(self.predictions, "throughput-critical")
        self.assertEqual(rec.recommended_approach, "B")

    def test_equal_metrics_yield_zero_penalties(self) -> None:
        flat = {
            "A": dict(SAMPLE_PREDICTIONS["A"]),
            "B": dict(SAMPLE_PREDICTIONS["A"]),
        }
        rec = strategies.weighted(flat, strategies.WEIGHT_PROFILES["balanced"], "balanced")
        self.assertEqual(rec.scores["A"], 0.0)
        self.assertEqual(rec.scores["B"], 0.0)


class ConstrainedStrategyTest(unittest.TestCase):
    def setUp(self) -> None:
        self.predictions = copy.deepcopy(SAMPLE_PREDICTIONS)

    def test_feasible_min_memory_with_p99_limit(self) -> None:
        rec = strategies.recommend(
            self.predictions,
            "constrained",
            objective="memory",
            constraints=[("p99_latency_us", "<=", 700.0)],
        )
        self.assertTrue(rec.feasible)
        # B и C проходят p99<=700; у C меньше память, чем у B
        self.assertEqual(rec.recommended_approach, "C")

    def test_infeasible_reports_violations(self) -> None:
        rec = strategies.recommend(
            self.predictions,
            "constrained",
            objective="memory",
            constraints=[("p99_latency_us", "<=", 100.0)],
        )
        self.assertFalse(rec.feasible)
        self.assertGreater(rec.scores["B"], 0.0)
        self.assertIn("нарушением", rec.notes[0])

    def test_min_throughput_constraint(self) -> None:
        rec = strategies.recommend(
            self.predictions,
            "constrained",
            objective="latency",
            constraints=[("throughput_ops_sec", ">=", 45_000.0)],
        )
        self.assertTrue(rec.feasible)
        self.assertEqual(rec.recommended_approach, "B")


class RecommendValidationTest(unittest.TestCase):
    def test_unknown_strategy_raises(self) -> None:
        with self.assertRaises(ValueError):
            strategies.recommend(SAMPLE_PREDICTIONS, "unknown")

    def test_empty_predictions_raises(self) -> None:
        with self.assertRaises(ValueError):
            strategies.recommend({}, "memory")

    def test_missing_metric_raises(self) -> None:
        broken = {"A": {"avg_latency_us": 1.0}}
        with self.assertRaises(ValueError):
            strategies.recommend(broken, "memory")

    def test_available_strategies_includes_constrained(self) -> None:
        names = strategies.available_strategies()
        self.assertIn("constrained", names)
        self.assertIn("balanced", names)


if __name__ == "__main__":
    unittest.main()
