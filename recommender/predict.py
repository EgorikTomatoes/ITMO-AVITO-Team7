
from __future__ import annotations

import importlib.util
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
INTERFACE_PATH = REPO_ROOT / "ML-Model" / "Interface.py"

_interface = None


def _load_interface():
    global _interface
    if _interface is None:
        if not INTERFACE_PATH.exists():
            raise FileNotFoundError(
                f"Не найден слой предсказаний Dev 4: {INTERFACE_PATH}"
            )
        spec = importlib.util.spec_from_file_location("dev4_interface", INTERFACE_PATH)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        _interface = module
    return _interface


def predict_metrics(params: dict) -> dict:
    """
    Возвращает словарь предсказанных метрик по подходам:
        {"A": {...}, "B": {...}, "C": {...}}

    params должен содержать ключи:
        D, W, F, p_read, p_write, p_mkdir, p_ls, p_mv, p_find, dist, zipf_s, locality
    """
    interface = _load_interface()

    result = interface.getPredictions(
        D=params["D"],
        W=params["W"],
        F=params["F"],
        p_read=params["p_read"],
        p_write=params["p_write"],
        p_mkdir=params["p_mkdir"],
        p_ls=params["p_ls"],
        p_mv=params["p_mv"],
        p_find=params["p_find"],
        dist=params["dist"],
        zipf_s=params["zipf_s"],
        locality=params["locality"],
        strategy="latency",
    )
    return result["predictions"]
