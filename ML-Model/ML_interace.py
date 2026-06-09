from sklearn.ensemble import RandomForestRegressor
from sklearn.model_selection import train_test_split
from sklearn.metrics import r2_score, mean_absolute_error, mean_squared_error


model = RandomForestRegressor(
    n_estimators=300,
    random_state=42,
    n_jobs=-1
)

# model.fit(X_train, y_train) // TODO: добавить считываемые данные

# y_pred = model.predict(X_test) // TODO: добавить данные проверочного тестирования
