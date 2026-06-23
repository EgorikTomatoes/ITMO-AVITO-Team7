from pathlib import Path
import runpy

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
    "balanced": ("balanced_score", "min"),
}


def get_model_path():
    model_path = BASE_DIR / "model.pkl"

    if model_path.exists():
        return model_path

    runpy.run_path(BASE_DIR / "ModelLearner.py")
    return model_path


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

    model_path = get_model_path()
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
    strategy_scores = {}

    if strategy == "balanced":
        weights = {
            "avg_latency_us": 0.4,
            "p99_latency_us": 0.2,
            "memory_bytes": 0.2,
            "throughput_ops_sec": 0.2,
        }

        for approach_name in approaches:
            strategy_scores[approach_name] = 0

        for metric_name, weight in weights.items():
            values = [
                predictions[approach_name][metric_name]
                for approach_name in approaches
            ]
            min_value = min(values)
            max_value = max(values)

            for approach_name in approaches:
                value = predictions[approach_name][metric_name]

                if max_value == min_value:
                    metric_score = 0
                elif metric_name == "throughput_ops_sec":
                    metric_score = (max_value - value) / (max_value - min_value)
                else:
                    metric_score = (value - min_value) / (max_value - min_value)

                strategy_scores[approach_name] += weight * metric_score

        for approach_name in approaches:
            value = strategy_scores[approach_name]
            if best_value is None or value < best_value:
                best_value = value
                recommended_approach = approach_name

    else:
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

    result = {
        "predictions": predictions,
        "strategy": strategy,
        "strategy_metric": strategy_metric,
        "recommended_approach": recommended_approach,
    }

    if strategy == "balanced":
        result["strategy_scores"] = strategy_scores

    return result
