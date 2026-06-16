#include "BenchmarkRunner.hpp"
#include "CsvWriter.hpp"
#include "WorkloadConfig.hpp"
#include "WorkloadGenerator.hpp"

#include "case_a.hpp"
#include "case_b.hpp"
#include "case_c.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

struct AccessPattern {
    std::string dist;
    double zipf_s;
    double locality;
};

std::vector<WorkloadConfig> MakeOperationProfiles() {
    std::vector<WorkloadConfig> profiles;

    auto add_profile = [&](const std::string& name,
                           double read,
                           double write,
                           double mkdir,
                           double ls,
                           double mv,
                           double find) {
        WorkloadConfig cfg;

        cfg.profile_name = name;

        cfg.p_read = read;
        cfg.p_write = write;
        cfg.p_mkdir = mkdir;
        cfg.p_ls = ls;
        cfg.p_mv = mv;
        cfg.p_find = find;

        cfg.ops = 200;
        cfg.repeats = 1;

        cfg.Validate();
        profiles.push_back(cfg);
    };

    // Порядок вероятностей:
    // read, write, mkdir, ls, mv, find

    add_profile("build_system", 0.45, 0.30, 0.10, 0.05, 0.05, 0.05);
    add_profile("file_manager", 0.20, 0.10, 0.15, 0.30, 0.20, 0.05);
    add_profile("repo_server", 0.60, 0.20, 0.05, 0.05, 0.03, 0.07);
    add_profile("backup", 0.55, 0.05, 0.05, 0.20, 0.02, 0.13);
    add_profile("refactoring", 0.20, 0.15, 0.10, 0.10, 0.35, 0.10);
    add_profile("static_web", 0.85, 0.03, 0.01, 0.03, 0.01, 0.07);

    return profiles;
}

std::vector<WorkloadConfig> MakeGrid() {
    std::vector<WorkloadConfig> grid;

    const std::vector<int> D_values = {
        2, 5, 10, 15
    };

    const std::vector<int> W_values = {
        10, 30, 100, 300
    };

    const std::vector<double> F_values = {
        0.3, 0.6, 0.95
    };

    const std::vector<AccessPattern> access_patterns = {
        {"uniform", 1.5, 0.0},
        {"zipf", 1.5, 0.3},
        {"zipf", 2.0, 0.8}
    };

    const auto profiles = MakeOperationProfiles();

    for (const auto& profile : profiles) {
        for (int D : D_values) {
            for (int W : W_values) {
                for (double F : F_values) {
                    for (const auto& access : access_patterns) {
                        WorkloadConfig cfg = profile;

                        cfg.D = D;
                        cfg.W = W;
                        cfg.F = F;

                        cfg.dist = access.dist;
                        cfg.zipf_s = access.zipf_s;
                        cfg.locality = access.locality;

                        cfg.Validate();
                        grid.push_back(cfg);
                    }
                }
            }
        }
    }

    return grid;
}

std::unique_ptr<IFileSystem> CreateFileSystem(const std::string& approach) {
    if (approach == "case_a") {
        return std::make_unique<Case_A>();
    }

    if (approach == "case_b") {
        return std::make_unique<Case_B>();
    }

    if (approach == "case_c") {
        return std::make_unique<Case_C>();
    }

    throw std::runtime_error("Unknown approach: " + approach);
}

BenchmarkResult AggregateResults(const std::vector<BenchmarkResult>& results) {
    BenchmarkResult aggregated;

    if (results.empty()) {
        return aggregated;
    }

    const double n = static_cast<double>(results.size());

    for (const auto& result : results) {
        aggregated.avg_latency_us += result.avg_latency_us;
        aggregated.p99_latency_us += result.p99_latency_us;
        aggregated.throughput_ops_sec += result.throughput_ops_sec;
        aggregated.memory_bytes += result.memory_bytes;
    }

    aggregated.avg_latency_us /= n;
    aggregated.p99_latency_us /= n;
    aggregated.throughput_ops_sec /= n;
    aggregated.memory_bytes = static_cast<size_t>(
        static_cast<double>(aggregated.memory_bytes) / n
    );

    if (results.size() == 1) {
        return aggregated;
    }

    for (const auto& result : results) {
        const double avg_diff =
            result.avg_latency_us - aggregated.avg_latency_us;

        const double p99_diff =
            result.p99_latency_us - aggregated.p99_latency_us;

        const double throughput_diff =
            result.throughput_ops_sec - aggregated.throughput_ops_sec;

        const double memory_diff =
            static_cast<double>(result.memory_bytes) -
            static_cast<double>(aggregated.memory_bytes);

        aggregated.avg_latency_us_std += avg_diff * avg_diff;
        aggregated.p99_latency_us_std += p99_diff * p99_diff;
        aggregated.throughput_ops_sec_std += throughput_diff * throughput_diff;
        aggregated.memory_bytes_std += memory_diff * memory_diff;
    }

    const double denominator = static_cast<double>(results.size() - 1);

    aggregated.avg_latency_us_std =
        std::sqrt(aggregated.avg_latency_us_std / denominator);

    aggregated.p99_latency_us_std =
        std::sqrt(aggregated.p99_latency_us_std / denominator);

    aggregated.throughput_ops_sec_std =
        std::sqrt(aggregated.throughput_ops_sec_std / denominator);

    aggregated.memory_bytes_std =
        std::sqrt(aggregated.memory_bytes_std / denominator);

    return aggregated;
}

int main() {
    try {
        const std::vector<std::string> approaches = {
            "case_a",
            "case_b",
            "case_c"
        };

        const std::vector<WorkloadConfig> grid = MakeGrid();

        if (grid.empty()) {
            throw std::runtime_error("Grid is empty");
        }

        std::ofstream csv("results.csv");
        if (!csv.is_open()) {
            throw std::runtime_error("Cannot open results.csv");
        }

        WriteCsvHeader(csv);

        BenchmarkRunner runner;

        std::cout << "Grid size: " << grid.size() << " configurations\n";
        std::cout << "Approaches: " << approaches.size() << '\n';
        std::cout << "Repeats: " << grid.front().repeats << '\n';
        std::cout << "Total measured runs: "
                  << grid.size() * approaches.size() * grid.front().repeats
                  << '\n';
        std::cout << "CSV rows after aggregation: "
                  << grid.size() * approaches.size()
                  << '\n';

        for (const auto& approach : approaches) {
            for (const auto& base_cfg : grid) {
                std::vector<BenchmarkResult> repeat_results;
                repeat_results.reserve(static_cast<size_t>(base_cfg.repeats));

                for (int repeat = 0; repeat < base_cfg.repeats; ++repeat) {
                    WorkloadConfig cfg = base_cfg;
                    cfg.seed = 42 + repeat;

                    auto fs = CreateFileSystem(approach);

                    WorkloadGenerator generator(cfg);

                    auto setup_ops = generator.GenerateSetup();
                    auto measured_ops = generator.GenerateMeasured();

                    runner.RunSetup(*fs, setup_ops);

                    BenchmarkResult result = runner.Run(*fs, measured_ops);
                    repeat_results.push_back(result);

                    std::cout
                        << "Done repeat: "
                        << approach
                        << ", profile=" << cfg.profile_name
                        << ", D=" << cfg.D
                        << ", W=" << cfg.W
                        << ", F=" << cfg.F
                        << ", dist=" << cfg.dist
                        << ", zipf_s=" << cfg.zipf_s
                        << ", locality=" << cfg.locality
                        << ", repeat=" << repeat
                        << '\n';
                }

                BenchmarkResult aggregated = AggregateResults(repeat_results);

                WriteCsvRow(csv, approach, base_cfg, aggregated);

                std::cout
                    << "Saved aggregated row: "
                    << approach
                    << ", profile=" << base_cfg.profile_name
                    << ", D=" << base_cfg.D
                    << ", W=" << base_cfg.W
                    << ", F=" << base_cfg.F
                    << ", dist=" << base_cfg.dist
                    << ", zipf_s=" << base_cfg.zipf_s
                    << ", locality=" << base_cfg.locality
                    << '\n';
            }
        }

        std::cout << "Results written to results.csv\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << '\n';
        return 1;
    }
}