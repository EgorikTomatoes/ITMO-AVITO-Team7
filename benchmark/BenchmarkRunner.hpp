#pragma once

#include "../Interface/file_system.hpp"
#include "Operations.hpp"

#include <cstddef>
#include <vector>

struct BenchmarkResult {
    double avg_latency_us = 0.0;
    double p99_latency_us = 0.0;
    double throughput_ops_sec = 0.0;
    size_t memory_bytes = 0;

    double avg_latency_us_std = 0.0;
    double p99_latency_us_std = 0.0;
    double throughput_ops_sec_std = 0.0;
    double memory_bytes_std = 0.0;
};

class BenchmarkRunner {
public:
    void RunSetup(IFileSystem& fs, const std::vector<Operation>& operations);

    BenchmarkResult Run(IFileSystem& fs, const std::vector<Operation>& operations);
};