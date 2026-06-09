#include "BenchmarkRunner.hpp"

#include <chrono>
#include <algorithm>
#include <numeric>

namespace  {

void RunSingleOperation(IFileSystem& fs, const Operation& op) {
    switch (op.type) {
        case OperationType::Read:
            fs.ReadFile(op.path1);
            break;

        case OperationType::Write:
            fs.WriteToFile(op.path1, op.data);
            break;

        case OperationType::Mkdir:
            fs.MakeDir(op.path1);
            break;

        case OperationType::List:
            fs.List(op.path1);
            break;

        case OperationType::Move:
            fs.Move(op.path1, op.path2);
            break;

        case OperationType::Find:
            fs.Find(op.path1, op.pattern);
            break;
    }
}

}

BenchmarkResult BenchmarkRunner::Run(IFileSystem& fs, const std::vector<Operation>& operations) {
    std::vector<double> latencies;
    latencies.reserve(operations.size());

    auto totalStart = std::chrono::steady_clock::now();

    for (const auto& op : operations) {
        auto start = std::chrono::steady_clock::now();

        RunSingleOperation(fs, op);

        auto end = std::chrono::steady_clock::now();

        double us = std::chrono::duration<double, std::micro>(end - start).count();
        latencies.push_back(us);
    }

    auto totalEnd = std::chrono::steady_clock::now();

    double totalSeconds = std::chrono::duration<double>(totalEnd - totalStart).count();

    std::sort(latencies.begin(), latencies.end());

    double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);

    BenchmarkResult result;
    result.avg_latency_us = sum / latencies.size();

    size_t p99Index = static_cast<size_t>(0.99 * (latencies.size() - 1));
    result.p99_latency_us = latencies[p99Index];

    result.throughput_ops_sec = operations.size() / totalSeconds;

    auto systemState = fs.SystemState();
    result.memory_bytes = systemState ? systemState->total_size_bytes : 0;

    return result;
}