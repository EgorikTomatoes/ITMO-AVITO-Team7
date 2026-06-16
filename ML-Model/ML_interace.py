from sklearn.ensemble import RandomForestRegressor
from sklearn.model_selection import train_test_split
from sklearn.metrics import r2_score, mean_absolute_error, mean_squared_error
import joblib

import numpy as np
import pandas as pd

data = pd.read_csv("benchmark_results.csv")

data["approach"] = data["approach"].map({
    "A": 0,
    "B": 1,
    "C": 2
})

data["Dist"] = data["Dist"].map({
    "Uniform": 0,
    "Zipf1.5": 1,
    "Zipf2.0": 2
})

data = data.dropna()

feature_columns = [
    "approach",
    "D",
    "W",
    "F",
    "P_read",
    "P_write",
    "P_mkdir",
    "P_ls",
    "P_mv",
    "P_find",
    "Dist",
    "Loc"
]

target_columns = [
    "avg_latency",
    "p99_latency",
    "memory",
    "throughput"
]

X = data[feature_columns]
y = data[target_columns]

X_train, X_test, y_train, y_test = train_test_split(
    X,
    y,
    test_size=0.2,
    random_state=42
)

model = RandomForestRegressor(
    n_estimators=300,
    random_state=42,
    n_jobs=-1
)

model.fit(X_train, y_train)

y_pred = model.predict(X_test)

print(r2_score(y_test, y_pred))
print(mean_absolute_error(y_test, y_pred))
print(mean_squared_error(y_test, y_pred))

joblib.dump(model, "model.pkl")