#include "CsvWriter.hpp"

#include <ostream>

void WriteCsvHeader(std::ostream& out) {
    out
        << "approach,"
        << "D,W,F,"
        << "profile,"
        << "dist,zipf_s,locality,"
        << "ops,repeat,"
        << "avg_latency_us,p99_latency_us,throughput_ops_sec,memory_bytes"
        << '\n';
}

void WriteCsvRow(
    std::ostream& out,
    const std::string& approach,
    const WorkloadConfig& cfg,
    int repeat,
    const BenchmarkResult& result
) {
    out
        << approach << ','
        << cfg.D << ','
        << cfg.W << ','
        << cfg.F << ','
        << cfg.profile_name << ','
        << cfg.dist << ','
        << cfg.zipf_s << ','
        << cfg.locality << ','
        << cfg.ops << ','
        << repeat << ','
        << result.avg_latency_us << ','
        << result.p99_latency_us << ','
        << result.throughput_ops_sec << ','
        << result.memory_bytes
        << '\n';
}