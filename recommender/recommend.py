#!/usr/bin/env python3
"""
Программа-рекомендатель (ТЗ §6).

На вход принимает параметры нагрузки (D, W, F, вероятности операций, профиль доступа)
и выбранную стратегию, получает у слоя предсказаний Dev 4 вектор метрик по каждому
подходу A/B/C, применяет выбранную стратегию (включая стратегию с ограничениями) и
печатает форматированный отчёт: предсказанные метрики по подходам + стратегия +
итоговая рекомендация

Программа не запускает бенчмарк, только арифметика поверх предсказаний.

Пример:
    python recommender/recommend.py \\
        --D 10 --W 100 --F 0.6 \\
        --p-read 0.6 --p-write 0.2 --p-mkdir 0.05 --p-ls 0.05 --p-mv 0.03 --p-find 0.07 \\
        --dist zipf --zipf-s 1.5 --locality 0.3 \\
        --strategy balanced

    python recommender/recommend.py ... --strategy constrained \\
        --objective memory --max-p99 5000
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import predict 
import strategies

METRIC_LABELS = {
    "avg_latency_us": "avg_latency_us",
    "p99_latency_us": "p99_latency_us",
    "memory_bytes": "memory_bytes",
    "throughput_ops_sec": "throughput_ops/s",
}

PROB_KEYS = ("p_read", "p_write", "p_mkdir", "p_ls", "p_mv", "p_find")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Рекомендатель оптимального подхода (A/B/C) по предсказанным метрикам.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    workload = parser.add_argument_group("параметры нагрузки")
    workload.add_argument("--D", type=int, required=True, help="глубина дерева директорий")
    workload.add_argument("--W", type=int, required=True, help="ширина (детей на узел)")
    workload.add_argument("--F", type=float, required=True, help="доля файлов")
    workload.add_argument("--p-read", type=float, required=True)
    workload.add_argument("--p-write", type=float, required=True)
    workload.add_argument("--p-mkdir", type=float, required=True)
    workload.add_argument("--p-ls", type=float, required=True)
    workload.add_argument("--p-mv", type=float, required=True)
    workload.add_argument("--p-find", type=float, required=True)
    workload.add_argument("--dist", choices=["uniform", "zipf"], required=True)
    workload.add_argument("--zipf-s", type=float, default=1.5, help="параметр s для zipf")
    workload.add_argument("--locality", type=float, default=0.0, help="локальность доступа [0..1]")

    strat = parser.add_argument_group("стратегия")
    strat.add_argument(
        "--strategy",
        default="balanced",
        choices=[
            "latency",
            "p99",
            "memory",
            "throughput",
            "balanced",
            "latency-critical",
            "memory-critical",
            "throughput-critical",
            "constrained",
        ],
        help="как выбираем подход из вектора метрик",
    )

    constr = parser.add_argument_group("опции стратегии constrained")
    constr.add_argument(
        "--objective",
        choices=["memory", "latency", "p99", "throughput"],
        default="memory",
        help="метрика, которую минимизируем/максимизируем при ограничениях",
    )
    constr.add_argument("--max-avg-latency", type=float, help="ограничение: avg_latency_us <= X")
    constr.add_argument("--max-p99", type=float, help="ограничение: p99_latency_us <= X")
    constr.add_argument("--max-memory", type=float, help="ограничение: memory_bytes <= X")
    constr.add_argument("--min-throughput", type=float, help="ограничение: throughput_ops_sec >= X")

    parser.add_argument("--json", action="store_true", help="вывод в формате JSON")

    return parser


def collect_params(args: argparse.Namespace) -> dict:
    return {
        "D": args.D,
        "W": args.W,
        "F": args.F,
        "p_read": args.p_read,
        "p_write": args.p_write,
        "p_mkdir": args.p_mkdir,
        "p_ls": args.p_ls,
        "p_mv": args.p_mv,
        "p_find": args.p_find,
        "dist": args.dist,
        "zipf_s": args.zipf_s,
        "locality": args.locality,
    }


def collect_constraints(args: argparse.Namespace) -> list[tuple]:
    constraints: list[tuple] = []
    if args.max_avg_latency is not None:
        constraints.append(("avg_latency_us", "<=", args.max_avg_latency))
    if args.max_p99 is not None:
        constraints.append(("p99_latency_us", "<=", args.max_p99))
    if args.max_memory is not None:
        constraints.append(("memory_bytes", "<=", args.max_memory))
    if args.min_throughput is not None:
        constraints.append(("throughput_ops_sec", ">=", args.min_throughput))
    return constraints


def format_report(params: dict, predictions: dict, rec: strategies.Recommendation) -> str:
    lines: list[str] = []
    lines.append("=" * 78)
    lines.append("РЕКОМЕНДАТЕЛЬ ПОДХОДА К ФАЙЛОВОЙ СИСТЕМЕ")
    lines.append("=" * 78)

    prob_str = ", ".join(f"{k}={params[k]:g}" for k in PROB_KEYS)
    lines.append(
        f"Нагрузка: D={params['D']}, W={params['W']}, F={params['F']:g}, "
        f"dist={params['dist']}, zipf_s={params['zipf_s']:g}, locality={params['locality']:g}"
    )
    lines.append(f"Вероятности операций: {prob_str}")
    lines.append("")

    lines.append("Предсказанные метрики по подходам:")
    header = f"  {'approach':<10}" + "".join(f"{METRIC_LABELS[m]:>20}" for m in strategies.METRICS)
    lines.append(header)
    lines.append("  " + "-" * (len(header) - 2))
    for approach in sorted(predictions):
        row = f"  {approach:<10}"
        for metric in strategies.METRICS:
            row += f"{predictions[approach][metric]:>20,.3f}"
        lines.append(row)
    lines.append("")

    lines.append(f"Стратегия: {rec.strategy}")
    if rec.scores:
        if rec.strategy.startswith("single:"):
            score_label = "значение целевой метрики"
        elif rec.strategy.startswith("constrained"):
            score_label = "значение целевой метрики" if rec.feasible else "суммарное нарушение"
        else:
            score_label = "штраф (меньше=лучше)"
        lines.append(f"Скоры стратегии ({score_label}):")
        for approach in sorted(rec.scores):
            lines.append(f"  {approach}: {rec.scores[approach]:,.4f}")

    for note in rec.notes:
        lines.append(f"  ! {note}")

    lines.append("")
    if not rec.feasible:
        lines.append("ВНИМАНИЕ: ограничения невыполнимы для всех подходов.")
    lines.append(f"==> Рекомендуемый подход: {rec.recommended_approach}")
    lines.append("=" * 78)
    return "\n".join(lines)


def main(argv=None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    params = collect_params(args)

    prob_sum = sum(params[k] for k in PROB_KEYS)
    if abs(prob_sum - 1.0) > 1e-6:
        print(
            f"[предупреждение] Сумма вероятностей операций = {prob_sum:g} (ожидается 1.0).",
            file=sys.stderr,
        )

    constraints = collect_constraints(args)
    if args.strategy == "constrained" and not constraints:
        print(
            "[предупреждение] Стратегия constrained без ограничений работает как чистая "
            "оптимизация целевой метрики (--objective).",
            file=sys.stderr,
        )

    try:
        predictions = predict.predict_metrics(params)
    except Exception as exc:  # noqa: BLE001
        print(f"Ошибка предсказания: {exc}", file=sys.stderr)
        return 1

    rec = strategies.recommend(
        predictions,
        args.strategy,
        objective=args.objective,
        constraints=constraints,
    )

    if args.json:
        output = {
            "params": params,
            "predictions": predictions,
            "strategy": rec.strategy,
            "recommended_approach": rec.recommended_approach,
            "scores": rec.scores,
            "feasible": rec.feasible,
            "notes": rec.notes,
        }
        print(json.dumps(output, ensure_ascii=False, indent=2))
    else:
        print(format_report(params, predictions, rec))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
