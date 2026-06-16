#include "CsvWriter.hpp"

#include <ostream>

void WriteCsvHeader(std::ostream& out) {
    out
        << "approach,"
        << "D,W,F,"
        << "profile,"
        << "p_read,p_write,p_mkdir,p_ls,p_mv,p_find,"
        << "dist,zipf_s,locality,"
        << "ops,repeats,"
        << "avg_latency_us,p99_latency_us,throughput_ops_sec,memory_bytes,"
        << "avg_latency_us_std,p99_latency_us_std,throughput_ops_sec_std,memory_bytes_std"
        << '\n';
}

void WriteCsvRow(
    std::ostream& out,
    const std::string& approach,
    const WorkloadConfig& cfg,
    const BenchmarkResult& result
) {
    out
        << approach << ','
        << cfg.D << ','
        << cfg.W << ','
        << cfg.F << ','
        << cfg.profile_name << ','
        << cfg.p_read << ','
        << cfg.p_write << ','
        << cfg.p_mkdir << ','
        << cfg.p_ls << ','
        << cfg.p_mv << ','
        << cfg.p_find << ','
        << cfg.dist << ','
        << cfg.zipf_s << ','
        << cfg.locality << ','
        << cfg.ops << ','
        << cfg.repeats << ','
        << result.avg_latency_us << ','
        << result.p99_latency_us << ','
        << result.throughput_ops_sec << ','
        << result.memory_bytes << ','
        << result.avg_latency_us_std << ','
        << result.p99_latency_us_std << ','
        << result.throughput_ops_sec_std << ','
        << result.memory_bytes_std
        << '\n';
}