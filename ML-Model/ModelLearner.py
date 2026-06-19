from sklearn.ensemble import RandomForestRegressor
from sklearn.model_selection import train_test_split
from sklearn.metrics import r2_score, mean_absolute_error, mean_squared_error
from sklearn.compose import TransformedTargetRegressor
import joblib
import json
from pathlib import Path

import numpy as np
import pandas as pd

BASE_DIR = Path(__file__).resolve().parent
BEST_PARAMS_PATH = BASE_DIR / "best_params.json"


def load_best_params():
    if not BEST_PARAMS_PATH.exists():
        return {}

    with BEST_PARAMS_PATH.open("r") as file:
        params_data = json.load(file)

    return params_data.get("best_params", {})


def create_model(target_name, model_params=None):
    base_model = RandomForestRegressor(
        n_estimators=400,
        random_state=42,
        n_jobs=-1
    )
    model = base_model
    if target_name in {"avg_latency_us", "p99_latency_us"}:
        model = TransformedTargetRegressor(
            regressor=base_model,
            func=np.log1p,
            inverse_func=np.expm1
        )
    if model_params:
        model.set_params(**model_params)
    return model


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

X = data[feature_columns]
y = data[target_columns]

X_train, X_test, y_train, y_test = train_test_split(
    X,
    y,
    test_size=0.2,
    random_state=42
)

models = {}
best_params = load_best_params()
y_pred = pd.DataFrame(index=y_test.index)

for target_name in target_columns:
    model = create_model(target_name, best_params.get(target_name))
    model.fit(X_train, y_train[target_name])
    models[target_name] = model
    y_pred[target_name] = model.predict(X_test)

print("Model Created!")
print("R2", r2_score(y_test, y_pred))
print("MAE", mean_absolute_error(y_test, y_pred))
print("RMSE", np.sqrt(mean_squared_error(y_test, y_pred)), "\n")

model_path = BASE_DIR / "model.pkl"
joblib.dump({
    "models": models,
    "feature_columns": feature_columns,
    "target_columns": target_columns,
    "log_transformed_targets": sorted({"avg_latency_us", "p99_latency_us"}),
    "best_params": best_params,
}, model_path)
