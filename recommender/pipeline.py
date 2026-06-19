#!/usr/bin/env python3

from __future__ import annotations

import argparse
import datetime as dt
import shutil
import subprocess
import sys
from pathlib import Path

RECO_DIR = Path(__file__).resolve().parent
REPO_ROOT = RECO_DIR.parent
BUILD_DIR = RECO_DIR / "build"
DATA_DIR = RECO_DIR / "data"
ML_DIR = REPO_ROOT / "ML-Model"


def run(cmd: list[str], cwd: Path | None = None) -> None:
    printable = " ".join(str(c) for c in cmd)
    location = f" (cwd={cwd})" if cwd else ""
    print(f"$ {printable}{location}")
    subprocess.run(cmd, cwd=str(cwd) if cwd else None, check=True)


def build_benchmark() -> Path:
    """Конфигурирует и собирает цель benchmark_app, возвращает путь к исполняемому файлу."""
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    run(["cmake", "-S", str(REPO_ROOT), "-B", str(BUILD_DIR), "-DCMAKE_BUILD_TYPE=Release"])
    run(["cmake", "--build", str(BUILD_DIR), "--target", "benchmark_app",
         "--config", "Release", "-j"])

    candidates = [
        BUILD_DIR / "benchmark_app",
        BUILD_DIR / "Release" / "benchmark_app.exe",
        BUILD_DIR / "Release" / "benchmark_app",
        BUILD_DIR / "benchmark_app.exe",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate

    found = list(BUILD_DIR.rglob("benchmark_app*"))
    found = [p for p in found if p.is_file()]
    if found:
        return found[0]
    raise FileNotFoundError("Не найден собранный benchmark_app в каталоге сборки")


def run_benchmark(executable: Path) -> Path:
    """Запускает бенчмарк; results.csv пишется в рабочий каталог -> DATA_DIR."""
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    run([str(executable)], cwd=DATA_DIR)
    result_csv = DATA_DIR / "results.csv"
    if not result_csv.exists():
        raise FileNotFoundError(f"Бенчмарк не создал {result_csv}")
    return result_csv


def install_dataset(result_csv: Path) -> Path:
    """Копирует свежий results.csv в ML-Model/, предварительно сохранив старый в recommender/data."""
    target = ML_DIR / "results.csv"
    if target.exists():
        stamp = dt.datetime.now().strftime("%Y%m%d-%H%M%S")
        backup = DATA_DIR / f"results.ml-prev-{stamp}.csv"
        shutil.copy2(target, backup)
        print(f"[backup] прежний ML-Model/results.csv сохранён в {backup}")
    shutil.copy2(result_csv, target)
    print(f"[install] {result_csv} -> {target}")
    return target


def train_model(python_exe: str) -> None:
    """Запускает обучение модели Dev 4 (перезаписывает ML-Model/model.pkl)."""
    learner = ML_DIR / "ModelLearner.py"
    if not learner.exists():
        raise FileNotFoundError(f"Не найден {learner}")
    run([python_exe, str(learner)], cwd=ML_DIR)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Сборка бенчмарка -> запуск -> (опц.) установка датасета -> (опц.) обучение.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("--skip-build", action="store_true",
                        help="не собирать заново, использовать уже собранный benchmark_app")
    parser.add_argument("--skip-run", action="store_true",
                        help="не запускать бенчмарк (например, использовать существующий data/results.csv)")
    parser.add_argument("--install-dataset", action="store_true",
                        help="скопировать свежий results.csv в ML-Model/ (ПЕРЕЗАПИСЫВАЕТ данные Dev 4)")
    parser.add_argument("--train", action="store_true",
                        help="обучить модель после установки датасета (ПЕРЕЗАПИСЫВАЕТ model.pkl)")
    parser.add_argument("--python", default=sys.executable,
                        help="интерпретатор python для обучения модели")
    return parser


def main(argv=None) -> int:
    args = build_parser().parse_args(argv)

    if not args.skip_build:
        executable = build_benchmark()
    else:
        candidates = [p for p in BUILD_DIR.rglob("benchmark_app*") if p.is_file()]
        if not candidates:
            print("Нет собранного benchmark_app — уберите --skip-build.", file=sys.stderr)
            return 1
        executable = candidates[0]

    result_csv = DATA_DIR / "results.csv"
    if not args.skip_run:
        result_csv = run_benchmark(executable)
    elif not result_csv.exists():
        print(f"Нет {result_csv} — уберите --skip-run.", file=sys.stderr)
        return 1

    print(f"\nСвежий датасет: {result_csv}")

    if args.install_dataset or args.train:
        install_dataset(result_csv)
    else:
        print("\nДатасет НЕ установлен в ML-Model/ (нет --install-dataset).")
        print("Чтобы воспроизвести модель полностью, выполните:")
        print("  python recommender/pipeline.py --install-dataset --train")
        return 0

    if args.train:
        train_model(args.python)
        print("\nМодель переобучена: ML-Model/model.pkl")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
