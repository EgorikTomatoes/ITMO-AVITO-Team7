# In-memory файловая система

Командный проект: три in-memory реализации файловой системы (подходы A/B/C), бенчмарк
нагрузок, ML-модели для аппроксимации метрик и программа-рекомендатель, которая по
параметрам нагрузки предсказывает производительность и выбирает оптимальный подход.

## Содержание

- [Структура репозитория](#структура-репозитория)
- [Зоны ответственности](#зоны-ответственности)
- [Требования](#требования)
- [Быстрый старт](#быстрый-старт)
- [Сборка и запуск (C++)](#сборка-и-запуск-c)
- [ML-модели (Python)](#ml-модели-python)
- [Программа-рекомендатель](#программа-рекомендатель)
- [Тесты](#тесты)
- [Метрики и подходы](#метрики-и-подходы)

## Структура репозитория

```
ITMO-AVITO-Team7/
├── Interface/          # общий интерфейс IFileSystem
├── case_A/             # подход A: наивное N-арное дерево
├── case_B/             # подход B: дерево + hash-индекс путей
├── case_C/             # подход C: плоский hash-map путей
├── utils/              # PathUtils, MemoryPool
├── benchmark/          # генератор нагрузок, runner, CSV, main (сетка MakeGrid)
├── ML-Model/           # обучение моделей, Interface.getPredictions, results.csv
├── recommender/        # CLI-рекомендатель, стратегии, пайплайн §8.1
└── tests/              # GoogleTest: ядро ФС, бенчмарк, рекомендатель
```

Подробнее по реализациям:
- `Interface/summary.md` — контракт `IFileSystem`
- `case_A/summary.md` — дерево и memory pool
- `recommender/README.md` — рекомендатель и стратегии (подробно)

## Зоны ответственности

| Участник | Зона | Ключевые каталоги |
|---|---|---|
| Dev 1 | Интерфейс, подход A | `Interface/`, `case_A/` |
| Dev 2 | Подходы B и C, утилиты | `case_B/`, `case_C/`, `utils/` |
| Dev 3 | Генератор нагрузок, бенчмарк | `benchmark/` |
| Dev 4 | ML-модели, предсказания | `ML-Model/` |
| Dev 5 | Рекомендатель, пайплайн, тесты | `recommender/`, `tests/` |

## Требования

**C++ (бенчмарк и реализации ФС):**
- компилятор с поддержкой **C++23**
- **CMake ≥ 3.16**

**Python (ML и рекомендатель):**
- Python 3.10+
- зависимости: `recommender/requirements.txt` (`joblib`, `scikit-learn`, `pandas`, `numpy`)

## Быстрый старт

```bash
# 1. Клонировать репозиторий
git clone https://github.com/EgorikTomatoes/ITMO-AVITO-Team7.git
cd ITMO-AVITO-Team7

# 2. Python-окружение (ML + рекомендатель)
python3 -m venv .venv
source .venv/bin/activate  # Windows: .venv\Scripts\activate
pip install -r recommender/requirements.txt

# 3. Обучить модель (model.pkl не хранится в git — >100 MB)
python ML-Model/ModelLearner.py

# 4. Запустить рекомендатель
python recommender/recommend.py \
  --D 10 --W 100 --F 0.6 \
  --p-read 0.6 --p-write 0.2 --p-mkdir 0.05 --p-ls 0.05 --p-mv 0.03 --p-find 0.07 \
  --dist zipf --zipf-s 1.5 --locality 0.3 \
  --strategy balanced
```

Если `model.pkl` отсутствует, `Interface.getPredictions` обучит модель автоматически
при первом вызове (медленнее, чем заранее обученная).

## Сборка и запуск (C++)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target benchmark_app -j
```

Запуск бенчмарка (полная сетка из `MakeGrid`, результат — `results.csv` в текущей директории):

```bash
cd build && ./benchmark_app
# или из корня, если указать рабочую директорию:
./build/benchmark_app
```

Бенчмарк прогоняет все три подхода (`case_a`, `case_b`, `case_c`) на сетке конфигураций
и пишет агрегированные метрики в CSV. Сетка задаётся в `benchmark/main.cpp`:
- `D ∈ {2, 5, 10, 15}`, `W ∈ {10, 30, 100, 300}`, `F ∈ {0.3, 0.6, 0.95}`
- 6 профилей операций × 3 паттерна доступа (uniform/zipf)

## ML-модели (Python)

Каталог `ML-Model/`:

| Файл | Назначение |
|---|---|
| `results.csv` | датасет бенчмарка (коммитнут) |
| `ModelLearner.py` | обучение Random Forest по каждой метрике → `model.pkl` |
| `BestParamsSearcher.py` | подбор гиперпараметров (GridSearchCV) |
| `Interface.py` | `getPredictions(...)` — вектор метрик по A/B/C |
| `best_params.json` | сохранённые гиперпараметры |

Обучение:

```bash
python ML-Model/ModelLearner.py
```

Предсказание из Python:

```python
from pathlib import Path
import importlib.util

spec = importlib.util.spec_from_file_location("iface", Path("ML-Model/Interface.py"))
iface = importlib.util.module_from_spec(spec)
spec.loader.exec_module(iface)

result = iface.getPredictions(
    D=10, W=100, F=0.6,
    p_read=0.6, p_write=0.2, p_mkdir=0.05, p_ls=0.05, p_mv=0.03, p_find=0.07,
    dist="zipf", zipf_s=1.5, locality=0.3,
    strategy="latency",
)
print(result["predictions"])  # {"A": {...}, "B": {...}, "C": {...}}
```

## Программа-рекомендатель

Модуль `recommender/` — точка входа для пользователя (ТЗ §6):

- **`recommend.py`** — CLI: параметры нагрузки + стратегия → таблица метрик A/B/C + рекомендация
- **`strategies.py`** — выбор подхода (однометричные, взвешенные, с ограничениями)
- **`predict.py`** — обёртка над `ML-Model/Interface.getPredictions`
- **`pipeline.py`** — воспроизводимый пайплайн §8.1 (сборка бенчмарка → CSV → обучение)

```bash
# рекомендация
python recommender/recommend.py --D 10 --W 100 --F 0.6 \
  --p-read 0.6 --p-write 0.2 --p-mkdir 0.05 --p-ls 0.05 --p-mv 0.03 --p-find 0.07 \
  --dist zipf --zipf-s 1.5 --locality 0.3 --strategy balanced

# полный пайплайн сбора данных и обучения
python recommender/pipeline.py --install-dataset --train
```

Стратегии: `latency`, `p99`, `memory`, `throughput`, `balanced`, `latency-critical`,
`memory-critical`, `throughput-critical`, `constrained`.

Подробности, примеры и таблица «какую стратегию выбрать» — в [`recommender/README.md`](recommender/README.md).

## Тесты

Каталог `tests/` содержит GoogleTest-наборы:

| Target | Что проверяет |
|---|---|
| `fs_core_tests` | CRUD, метрики, edge cases для Case_A/B/C |
| `benchmark_tests` | WorkloadConfig, Generator, CsvWriter, BenchmarkRunner |
| `recommender_tests` | логика рекомендательной системы |

Для сборки тестов нужен Google Test (подключение через `add_subdirectory(tests)` в корневом
`CMakeLists.txt` и FetchContent для gtest — при необходимости уточните у команды текущую
конфигурацию сборки).

## Метрики и подходы

**Три реализации** (`IFileSystem`):

| Подход | Класс | Идея |
|---|---|---|
| A | `TreeFileSystem` | N-арное дерево, линейный поиск детей |
| B | `TreeHashFileSystem` | дерево + `unordered_map` для индексации путей |
| C | `FlatFileSystem` | плоский `unordered_map` path → node |

**Измеряемые метрики** (бенчмарк → CSV → ML → рекомендатель):

- `avg_latency_us` — средняя латентность операции
- `p99_latency_us` — 99-й перцентиль латентности
- `throughput_ops_sec` — пропускная способность
- `memory_bytes` — память реализации (`SystemState::total_size_bytes`)

**Параметры нагрузки** (вход рекомендателя и признаки модели):

- структура: `D` (глубина), `W` (ширина), `F` (доля файлов)
- операции: `p_read`, `p_write`, `p_mkdir`, `p_ls`, `p_mv`, `p_find`
- доступ: `dist` (uniform/zipf), `zipf_s`, `locality`

Модель обучена на сетке из бенчмарка; для значений сильно за пределами
`D ∈ [2,15]`, `W ∈ [10,300]`, `F ∈ [0.3,0.95]` предсказания менее надёжны (экстраполяция).
