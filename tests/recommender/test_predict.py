"""Тесты слоя предсказаний с mock Interface (recommender/predict.py)."""

from __future__ import annotations

import importlib
import sys
import unittest
from pathlib import Path
from unittest.mock import MagicMock, patch

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "recommender"))

import predict 

from fixtures import SAMPLE_PARAMS, SAMPLE_PREDICTIONS 


class PredictMetricsTest(unittest.TestCase):
    def setUp(self) -> None:
        predict._interface = None

    @patch.object(predict, "_load_interface")
    def test_returns_predictions_only(self, mock_load: MagicMock) -> None:
        mock_iface = MagicMock()
        mock_iface.getPredictions.return_value = {
            "predictions": SAMPLE_PREDICTIONS,
            "recommended_approach": "B",
            "strategy": "latency",
        }
        mock_load.return_value = mock_iface

        result = predict.predict_metrics(SAMPLE_PARAMS)

        self.assertEqual(result, SAMPLE_PREDICTIONS)
        mock_iface.getPredictions.assert_called_once()
        call_kwargs = mock_iface.getPredictions.call_args.kwargs
        self.assertEqual(call_kwargs["D"], SAMPLE_PARAMS["D"])
        self.assertEqual(call_kwargs["dist"], SAMPLE_PARAMS["dist"])
        self.assertEqual(call_kwargs["strategy"], "latency")

    def test_missing_interface_raises(self) -> None:
        with patch.object(predict, "INTERFACE_PATH", Path("/nonexistent/Interface.py")):
            predict._interface = None
            with self.assertRaises(FileNotFoundError):
                predict.predict_metrics(SAMPLE_PARAMS)


if __name__ == "__main__":
    unittest.main()
