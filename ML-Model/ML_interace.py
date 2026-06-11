from sklearn.ensemble import RandomForestRegressor
from sklearn.model_selection import train_test_split
from sklearn.metrics import r2_score, mean_absolute_error, mean_squared_error

import numpy as np
import pandas as pd

data = pd.read_csv("benchmark_results.csv")

# После того, как данные будут считаны, нужно будет разделить их на обучающую и тестовую выборки

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

model = RandomForestRegressor(
    n_estimators=300,
    random_state=42,
    n_jobs=-1
)

# model.fit(X_train, y_train) // TODO: добавить считываемые данные

# y_pred = model.predict(X_test) // TODO: добавить данные проверочного тестирования
