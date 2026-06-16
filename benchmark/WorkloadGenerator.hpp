#pragma once

#include "Operations.hpp"
#include "WorkloadConfig.hpp"

#include <random>
#include <string>
#include <vector>

class WorkloadGenerator {
public:
    explicit WorkloadGenerator(const WorkloadConfig& cfg);

    std::vector<Operation> GenerateSetup();
    std::vector<Operation> GenerateMeasured();

private:
    WorkloadConfig cfg_;
    std::mt19937 rng_;

    std::vector<std::string> files_;
    std::vector<std::string> dirs_;

    int next_file_id_ = 0;
    int next_dir_id_ = 0;

    std::string hot_dir_ = "/";
    bool setup_generated_ = false;

private:
    void ResetModel();

    Operation GenerateOne();

    std::string RandomFile();
    std::string RandomDir();

    std::string NewFilePath();
    std::string NewDirPath();

    size_t RandomIndex(size_t n);
    bool Probability(double p);

    static std::string JoinPath(const std::string& dir, const std::string& name);
    static std::string ParentPath(const std::string& path);
};