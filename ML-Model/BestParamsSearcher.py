from pathlib import Path
import json

import joblib
import numpy as np
import pandas as pd
from sklearn.compose import TransformedTargetRegressor
from sklearn.ensemble import RandomForestRegressor
from sklearn.model_selection import GridSearchCV, train_test_split


BASE_DIR = Path(__file__).resolve().parent



def create_model(target_name):
    base_model = RandomForestRegressor(
        random_state=42,
        n_jobs=-1
    )

    if target_name in {"avg_latency_us", "p99_latency_us"}:
        return TransformedTargetRegressor(
            regressor=base_model,
            func=np.log1p,
            inverse_func=np.expm1
        )

    return base_model


def create_param_grid(target_name):
    param_grid = param_grid = {
        "n_estimators": [100, 200, 300],
        "max_depth": [None, 10, 20, 30],
        "min_samples_split": [2, 5, 10],
        "min_samples_leaf": [1, 2, 4],
        "max_features": [None, "sqrt", "log2"],
    }

    if target_name not in {"avg_latency_us", "p99_latency_us"}:
        return param_grid

    return {
        "regressor__n_estimators": [100, 200, 300],
        "regressor__max_depth": [None, 10, 20, 30],
        "regressor__min_samples_split": [2, 5, 10],
        "regressor__min_samples_leaf": [1, 2, 4],
        "regressor__max_features": [None, "sqrt", "log2"],
    }


def load_data():
    data = pd.read_csv(BASE_DIR / "results.csv")

    data["approach"] = data["approach"].map({
        "case_a": 0,
        "case_b": 1,
        "case_c": 2
    })

    data["dist"] = data["dist"].map({
        "uniform": 0,
        "zipf": 1
    })

    data = data.dropna()

    feature_columns = [
        "approach",
        "D",
        "W",
        "F",
        "p_read",
        "p_write",
        "p_mkdir",
        "p_ls",
        "p_mv",
        "p_find",
        "dist",
        "zipf_s",
        "locality"
    ]

    target_columns = [
        "avg_latency_us",
        "p99_latency_us",
        "memory_bytes",
        "throughput_ops_sec"
    ]

    return data[feature_columns], data[target_columns], feature_columns, target_columns


X, y, feature_columns, target_columns = load_data()

X_train, _, y_train, _ = train_test_split(
    X,
    y,
    test_size=0.2,
    random_state=42
)

best_models = {}
best_params = {}

for target_name in target_columns:
    grid_search = GridSearchCV(
        estimator=create_model(target_name),
        param_grid=create_param_grid(target_name),
        cv=5,
        scoring="neg_root_mean_squared_error",
        n_jobs=-1
    )

    grid_search.fit(X_train, y_train[target_name])

    best_models[target_name] = grid_search.best_estimator_
    best_params[target_name] = grid_search.best_params_

best_params_path = BASE_DIR / "best_params.json"
best_models_path = BASE_DIR / "best_models.pkl"

with best_params_path.open("w", encoding="utf-8") as file:
    json.dump({"best_params": best_params}, file, indent=2)

joblib.dump({
    "models": best_models,
    "feature_columns": feature_columns,
    "target_columns": target_columns,
    "log_transformed_targets": sorted({"avg_latency_us", "p99_latency_us"}),
}, best_models_path)

print(f"\nBest params saved to: {best_params_path}")
print(f"Best models saved to: {best_models_path}")
