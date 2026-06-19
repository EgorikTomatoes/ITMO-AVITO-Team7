# recommender — программа-рекомендатель и пайплайн (Dev 5)

Модуль решает две задачи ТЗ:

- **§6 — программа-рекомендатель**: по параметрам нагрузки предсказывает метрики
  для подходов A/B/C и по выбранной стратегии рекомендует лучший подход.
- **§8.1 — воспроизводимый пайплайн**: собрать C++-бенчмарк → запустить →
  получить `results.csv` → положить в `ML-Model/` → обучить модель.

Чужие папки (`benchmark/`, `case_A/B/C`, `utils/`, `ML-Model/`) модуль **не изменяет**:
он их только вызывает (бенчмарк — через cmake, предсказания — импортом
`ML-Model/Interface.py`).

## Зоны ответственности

- Слой предсказаний (вектор метрик по A/B/C) — Dev 4, `ML-Model/Interface.getPredictions`.
- Выбор подхода из вектора метрик (стратегии) — здесь, `strategies.py`.
  Встроенный в Interface `recommended_approach` игнорируется.

## Установка окружения

```bash
python3 -m venv .venv
source .venv/bin/activate   
pip install -r recommender/requirements.txt
```

## Запуск рекомендателя (§6)

```bash
python recommender/recommend.py \
    --D 10 --W 100 --F 0.6 \
    --p-read 0.6 --p-write 0.2 --p-mkdir 0.05 --p-ls 0.05 --p-mv 0.03 --p-find 0.07 \
    --dist zipf --zipf-s 1.5 --locality 0.3 \
    --strategy balanced
```

`--json` выводит машиночитаемый результат.

### Стратегии (`--strategy`)

Однометричные:
- `latency` — минимальная средняя латентность;
- `p99` — минимальная p99-латентность;
- `memory` — минимальная память;
- `throughput` — максимальная пропускная способность.

Взвешенные многокритериальные (нормировка метрик min-max по подходам, throughput
инвертируется, итог — взвешенная сумма штрафов):
- `balanced`, `latency-critical`, `memory-critical`, `throughput-critical`.

Стратегия с ограничениями:
- `constrained` — оптимизирует `--objective` (`memory`/`latency`/`p99`/`throughput`)
  при ограничениях `--max-avg-latency`, `--max-p99`, `--max-memory`, `--min-throughput`.

```bash
python recommender/recommend.py --D 10 --W 100 --F 0.6 \
    --p-read 0.6 --p-write 0.2 --p-mkdir 0.05 --p-ls 0.05 --p-mv 0.03 --p-find 0.07 \
    --dist zipf --zipf-s 1.5 --locality 0.3 \
    --strategy constrained --objective memory --max-p99 5000
```

Если ни один подход не проходит ограничения, рекомендатель честно сообщает об этом и
возвращает подход с наименьшим суммарным нарушением.

## Воспроизводимый пайплайн (§8.1)

```bash
# 1) собрать и прогнать бенчмарк -> recommender/data/results.csv (ML-Model не трогается)
python recommender/pipeline.py

# 2) полная воспроизводимость: установить датасет в ML-Model/ и переобучить модель
python recommender/pipeline.py --install-dataset --train
```

- Артефакты сборки — `recommender/build/`, свежий датасет — `recommender/data/`.
- Запись в `ML-Model/` (датасет и `model.pkl`) выполняется только при явных флагах
  `--install-dataset` / `--train`; прежний `ML-Model/results.csv` бэкапится в
  `recommender/data/`.
- Полная сетка конфигураций строится самим бенчмарком (`MakeGrid` в `benchmark/main.cpp`),
  пайплайн её не дублирует.
