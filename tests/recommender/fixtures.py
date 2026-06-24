"""Общие фикстуры для тестов рекомендателя."""

SAMPLE_PREDICTIONS = {
    "A": {
        "avg_latency_us": 25.0,
        "p99_latency_us": 800.0,
        "memory_bytes": 100.0,
        "throughput_ops_sec": 40_000.0,
    },
    "B": {
        "avg_latency_us": 20.0,
        "p99_latency_us": 650.0,
        "memory_bytes": 500.0,
        "throughput_ops_sec": 50_000.0,
    },
    "C": {
        "avg_latency_us": 30.0,
        "p99_latency_us": 700.0,
        "memory_bytes": 300.0,
        "throughput_ops_sec": 35_000.0,
    },
}

SAMPLE_PARAMS = {
    "D": 10,
    "W": 100,
    "F": 0.6,
    "p_read": 0.6,
    "p_write": 0.2,
    "p_mkdir": 0.05,
    "p_ls": 0.05,
    "p_mv": 0.03,
    "p_find": 0.07,
    "dist": "zipf",
    "zipf_s": 1.5,
    "locality": 0.3,
}
