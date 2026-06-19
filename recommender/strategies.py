
from __future__ import annotations

from dataclasses import dataclass, field

METRICS = (
    "avg_latency_us",
    "p99_latency_us",
    "memory_bytes",
    "throughput_ops_sec",
)

LOWER_IS_BETTER = {
    "avg_latency_us": True,
    "p99_latency_us": True,
    "memory_bytes": True,
    "throughput_ops_sec": False,
}

SINGLE_METRIC_STRATEGIES = {
    "latency": "avg_latency_us",
    "p99": "p99_latency_us",
    "memory": "memory_bytes",
    "throughput": "throughput_ops_sec",
}

# взвешенные многокритериальные профили имя -> веса метрик в сумме 1.0
WEIGHT_PROFILES = {
    "balanced": {
        "avg_latency_us": 0.40,
        "p99_latency_us": 0.20,
        "memory_bytes": 0.20,
        "throughput_ops_sec": 0.20,
    },
    "latency_critical": {
        "avg_latency_us": 0.50,
        "p99_latency_us": 0.30,
        "memory_bytes": 0.10,
        "throughput_ops_sec": 0.10,
    },
    "memory_critical": {
        "memory_bytes": 0.60,
        "avg_latency_us": 0.20,
        "p99_latency_us": 0.10,
        "throughput_ops_sec": 0.10,
    },
    "throughput_critical": {
        "throughput_ops_sec": 0.60,
        "avg_latency_us": 0.20,
        "p99_latency_us": 0.10,
        "memory_bytes": 0.10,
    },
}

# дружелюбные имена целевой метрики для стратегии с ограничениями
OBJECTIVE_ALIASES = {
    "latency": "avg_latency_us",
    "p99": "p99_latency_us",
    "memory": "memory_bytes",
    "throughput": "throughput_ops_sec",
}

_COMPARATORS = {
    "<=": lambda value, bound: value <= bound,
    ">=": lambda value, bound: value >= bound,
    "<": lambda value, bound: value < bound,
    ">": lambda value, bound: value > bound,
}


@dataclass
class Recommendation:

    strategy: str
    recommended_approach: str | None
    scores: dict
    feasible: bool = True
    notes: list[str] = field(default_factory=list)


def _check_predictions(predictions: dict) -> None:
    if not predictions:
        raise ValueError("predictions пуст")
    for approach, metrics in predictions.items():
        missing = [m for m in METRICS if m not in metrics]
        if missing:
            raise ValueError(f"У подхода {approach} нет метрик: {missing}")


def single_metric(predictions: dict, metric: str) -> Recommendation:
    """выбор по одной метрике (min для cost-метрик, max для throughput)"""
    if metric not in LOWER_IS_BETTER:
        raise ValueError(f"Неизвестная метрика: {metric}")

    lower_is_better = LOWER_IS_BETTER[metric]
    scores = {approach: metrics[metric] for approach, metrics in predictions.items()}

    best_approach = (
        min(scores, key=scores.get) if lower_is_better else max(scores, key=scores.get)
    )
    return Recommendation(
        strategy=f"single:{metric}",
        recommended_approach=best_approach,
        scores=scores,
    )


def weighted(predictions: dict, weights: dict, strategy_name: str = "weighted") -> Recommendation:
    """
    многокритериальный выбор каждая метрика нормируется min-max ПО ПОДХОДАМ и
    переводится в штраф (0 = лучший подход по метрике, 1 = худший), для throughput
    направление инвертируется. в итоге взвешенная сумма штрафов, выбираем минимум
    """
    approaches = list(predictions)
    scores = {approach: 0.0 for approach in approaches}

    for metric, weight in weights.items():
        if weight == 0:
            continue

        values = [predictions[approach][metric] for approach in approaches]
        low, high = min(values), max(values)
        span = high - low

        for approach in approaches:
            value = predictions[approach][metric]
            if span == 0:
                penalty = 0.0
            elif LOWER_IS_BETTER[metric]:
                penalty = (value - low) / span
            else:
                penalty = (high - value) / span
            scores[approach] += weight * penalty

    best_approach = min(scores, key=scores.get)
    return Recommendation(
        strategy=strategy_name,
        recommended_approach=best_approach,
        scores=scores,
    )


def _violation(value: float, op: str, bound: float) -> float:
    if _COMPARATORS[op](value, bound):
        return 0.0
    return abs(value - bound)


def constrained(predictions: dict, objective: str, constraints: list[tuple]) -> Recommendation:
    """
    оптимизация целевой метрики при ограничениях
    """
    if objective not in LOWER_IS_BETTER:
        raise ValueError(f"Неизвестная целевая метрика: {objective}")
    for metric, op, _ in constraints:
        if metric not in LOWER_IS_BETTER:
            raise ValueError(f"Неизвестная метрика в ограничении: {metric}")
        if op not in _COMPARATORS:
            raise ValueError(f"Неизвестный оператор ограничения: {op}")

    approaches = list(predictions)
    feasible_pool = [
        approach
        for approach in approaches
        if all(_COMPARATORS[op](predictions[approach][m], val) for m, op, val in constraints)
    ]

    notes: list[str] = []

    if feasible_pool:
        lower_is_better = LOWER_IS_BETTER[objective]
        scores = {approach: predictions[approach][objective] for approach in approaches}
        chooser = min if lower_is_better else max
        best_approach = chooser(feasible_pool, key=lambda a: predictions[a][objective])
        return Recommendation(
            strategy=f"constrained:min({objective})",
            recommended_approach=best_approach,
            scores=scores,
            feasible=True,
            notes=[f"Ограничениям удовлетворяют: {', '.join(feasible_pool)}"],
        )

    violations = {
        approach: sum(_violation(predictions[approach][m], op, val) for m, op, val in constraints)
        for approach in approaches
    }
    best_approach = min(violations, key=violations.get)
    notes.append("Ни один подход не удовлетворяет всем ограничениям; "
                 "выбран с наименьшим суммарным нарушением.")
    return Recommendation(
        strategy=f"constrained:min({objective})",
        recommended_approach=best_approach,
        scores=violations,
        feasible=False,
        notes=notes,
    )


def recommend(
    predictions: dict,
    strategy: str,
    *,
    objective: str | None = None,
    constraints: list[tuple] | None = None,
) -> Recommendation:
    _check_predictions(predictions)
    key = strategy.lower().replace("-", "_")

    if key in SINGLE_METRIC_STRATEGIES:
        return single_metric(predictions, SINGLE_METRIC_STRATEGIES[key])

    if key in WEIGHT_PROFILES:
        return weighted(predictions, WEIGHT_PROFILES[key], strategy_name=key)

    if key == "constrained":
        objective_metric = OBJECTIVE_ALIASES.get(
            (objective or "memory").lower(), objective or "memory_bytes"
        )
        return constrained(predictions, objective_metric, constraints or [])

    available = (
        list(SINGLE_METRIC_STRATEGIES)
        + list(WEIGHT_PROFILES)
        + ["constrained"]
    )
    raise ValueError(f"Неизвестная стратегия '{strategy}'. Доступно: {available}")


def available_strategies() -> list[str]:
    return list(SINGLE_METRIC_STRATEGIES) + list(WEIGHT_PROFILES) + ["constrained"]
