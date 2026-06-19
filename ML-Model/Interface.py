from pathlib import Path

import joblib
import pandas as pd


BASE_DIR = Path(__file__).resolve().parent

approaches = {
    "A": 0,
    "B": 1,
    "C": 2,
}

strategies = {
    "latency": ("avg_latency_us", "min"),
    "p99": ("p99_latency_us", "min"),
    "memory": ("memory_bytes", "min"),
    "throughput": ("throughput_ops_sec", "max"),
}


def getPredictions(
    D,
    W,
    F,
    p_read,
    p_write,
    p_mkdir,
    p_ls,
    p_mv,
    p_find,
    dist,
    zipf_s,
    locality,
    strategy="latency",
):
    probabilities_sum = p_read + p_write + p_mkdir + p_ls + p_mv + p_find
    if probabilities_sum == 0:
        raise ValueError("Incorrect probabilities")

    dist = dist.lower()
    if dist == "uniform":
        dist_value = 0
    elif dist == "zipf":
        dist_value = 1
    else:
        raise ValueError("dist must be uniform or zipf")

    strategy = strategy.lower()
    if strategy not in strategies:
        raise ValueError("Unknown strategy")

    model_path = BASE_DIR / "BestModel.pkl"

    model_data = joblib.load(model_path)
    models = model_data["models"]
    feature_columns = model_data["feature_columns"]
    target_columns = model_data["target_columns"]

    predictions = {}

    for approach_name, approach_value in approaches.items():
        row = {
            "approach": approach_value,
            "D": D,
            "W": W,
            "F": F,
            "p_read": p_read,
            "p_write": p_write,
            "p_mkdir": p_mkdir,
            "p_ls": p_ls,
            "p_mv": p_mv,
            "p_find": p_find,
            "dist": dist_value,
            "zipf_s": zipf_s,
            "locality": locality,
        }

        X = pd.DataFrame([row], columns=feature_columns)
        predictions[approach_name] = {}

        for target_name in target_columns:
            prediction = models[target_name].predict(X)[0]
            predictions[approach_name][target_name] = float(prediction)

    strategy_metric, strategy_type = strategies[strategy]
    recommended_approach = None
    best_value = None

    for approach_name in approaches:
        value = predictions[approach_name][strategy_metric]
        if best_value is None:
            best_value = value
            recommended_approach = approach_name
        elif strategy_type == "min" and value < best_value:
            best_value = value
            recommended_approach = approach_name
        elif strategy_type == "max" and value > best_value:
            best_value = value
            recommended_approach = approach_name

    return {
        "predictions": predictions,
        "strategy": strategy,
        "strategy_metric": strategy_metric,
        "recommended_approach": recommended_approach,
    }
