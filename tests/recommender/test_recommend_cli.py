"""Интеграционные тесты CLI рекомендателя (recommender/recommend.py)."""

from __future__ import annotations

import importlib
import io
import json
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "recommender"))

from fixtures import SAMPLE_PREDICTIONS  # noqa: E402


CLI_ARGS = [
    "--D", "10", "--W", "100", "--F", "0.6",
    "--p-read", "0.6", "--p-write", "0.2", "--p-mkdir", "0.05",
    "--p-ls", "0.05", "--p-mv", "0.03", "--p-find", "0.07",
    "--dist", "zipf", "--zipf-s", "1.5", "--locality", "0.3",
]


class RecommendCliTest(unittest.TestCase):
    def setUp(self) -> None:
        if "recommend" in sys.modules:
            del sys.modules["recommend"]
        self.recommend = importlib.import_module("recommend")

    @patch("recommend.predict.predict_metrics", return_value=SAMPLE_PREDICTIONS)
    def test_main_prints_recommendation(self, _mock_predict) -> None:
        stdout = io.StringIO()
        with patch("sys.stdout", stdout):
            code = self.recommend.main(CLI_ARGS + ["--strategy", "memory"])
        self.assertEqual(code, 0)
        output = stdout.getvalue()
        self.assertIn("Рекомендуемый подход: A", output)
        self.assertIn("memory_bytes", output)

    @patch("recommend.predict.predict_metrics", return_value=SAMPLE_PREDICTIONS)
    def test_json_output_structure(self, _mock_predict) -> None:
        stdout = io.StringIO()
        with patch("sys.stdout", stdout):
            code = self.recommend.main(CLI_ARGS + ["--strategy", "throughput", "--json"])
        self.assertEqual(code, 0)
        payload = json.loads(stdout.getvalue())
        self.assertEqual(payload["recommended_approach"], "B")
        self.assertIn("A", payload["predictions"])
        self.assertEqual(payload["strategy"], "single:throughput_ops_sec")

    @patch("recommend.predict.predict_metrics", return_value=SAMPLE_PREDICTIONS)
    def test_constrained_cli(self, _mock_predict) -> None:
        stdout = io.StringIO()
        with patch("sys.stdout", stdout):
            code = self.recommend.main(
                CLI_ARGS + [
                    "--strategy", "constrained",
                    "--objective", "memory",
                    "--max-p99", "700",
                ]
            )
        self.assertEqual(code, 0)
        self.assertIn("Рекомендуемый подход: C", stdout.getvalue())


if __name__ == "__main__":
    unittest.main()
