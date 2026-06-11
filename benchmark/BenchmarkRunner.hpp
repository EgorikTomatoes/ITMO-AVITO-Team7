#pragma once

#include "../Interface/file_system.hpp"
#include "Operations.hpp"

#include <vector>

struct BenchmarkResult {
    double avg_latency_us = 0.0;
    double p99_latency_us = 0.0;
    double throughput_ops_sec = 0.0;
    size_t memory_bytes = 0;
};

class BenchmarkRunner {
public:
    BenchmarkResult Run(IFileSystem& fs, const std::vector<Operation>& operations);
};