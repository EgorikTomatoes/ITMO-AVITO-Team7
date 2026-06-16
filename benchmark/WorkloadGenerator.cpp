#include "WorkloadGenerator.hpp"

#include <cmath>
#include <stdexcept>

WorkloadGenerator::WorkloadGenerator(const WorkloadConfig& cfg)
    : cfg_(cfg), rng_(cfg.seed) {
    cfg_.Validate();
    ResetModel();
}

void WorkloadGenerator::ResetModel() {
    files_.clear();
    dirs_.clear();

    dirs_.push_back("/");

    next_file_id_ = 0;
    next_dir_id_ = 0;

    hot_dir_ = "/";
    setup_generated_ = false;
}

std::vector<Operation> WorkloadGenerator::GenerateSetup() {
    ResetModel();

    std::vector<Operation> setup;

    std::vector<std::string> parent_dirs;
    parent_dirs.push_back("/");

    std::bernoulli_distribution file_dist(cfg_.F);

    for (int depth = 1; depth <= cfg_.D; ++depth) {
        std::vector<std::string> next_level_dirs;

        for (int i = 0; i < cfg_.W; ++i) {
            const std::string& parent = parent_dirs[RandomIndex(parent_dirs.size())];

            bool must_create_dir = (depth < cfg_.D && i == 0);
            bool create_file = !must_create_dir && file_dist(rng_);

            if (create_file) {
                std::string name =
                    "file_d" + std::to_string(depth) +
                    "_" + std::to_string(next_file_id_++) + ".txt";

                std::string path = JoinPath(parent, name);
                files_.push_back(path);

                setup.push_back(Operation{
                    OperationType::Write,
                    path,
                    "",
                    "initial data",
                    ""
                });
            } else {
                std::string name =
                    "dir_d" + std::to_string(depth) +
                    "_" + std::to_string(next_dir_id_++);

                std::string path = JoinPath(parent, name);
                dirs_.push_back(path);
                next_level_dirs.push_back(path);

                setup.push_back(Operation{
                    OperationType::Mkdir,
                    path,
                    "",
                    "",
                    ""
                });
            }
        }

        if (!next_level_dirs.empty()) {
            parent_dirs = next_level_dirs;
        }
    }

    if (files_.empty()) {
        std::string path = JoinPath("/", "initial_file.txt");
        files_.push_back(path);

        setup.push_back(Operation{
            OperationType::Write,
            path,
            "",
            "initial data",
            ""
        });
    }

    setup_generated_ = true;
    return setup;
}

std::vector<Operation> WorkloadGenerator::GenerateMeasured() {
    if (!setup_generated_) {
        GenerateSetup();
    }

    std::vector<Operation> operations;
    operations.reserve(static_cast<size_t>(cfg_.ops));

    for (int i = 0; i < cfg_.ops; ++i) {
        operations.push_back(GenerateOne());
    }

    return operations;
}

Operation WorkloadGenerator::GenerateOne() {
    std::discrete_distribution<int> op_dist({
        cfg_.p_read,
        cfg_.p_write,
        cfg_.p_mkdir,
        cfg_.p_ls,
        cfg_.p_mv,
        cfg_.p_find
    });

    const int op = op_dist(rng_);

    switch (op) {
        case 0:
            return Operation{OperationType::Read, RandomFile(), "", "", ""};

        case 1: {
            if (!files_.empty() && Probability(0.7)) {
                return Operation{OperationType::Write, RandomFile(), "", "hello", ""};
            }

            std::string path = NewFilePath();
            files_.push_back(path);
            hot_dir_ = ParentPath(path);

            return Operation{OperationType::Write, path, "", "hello", ""};
        }

        case 2: {
            std::string path = NewDirPath();
            dirs_.push_back(path);
            hot_dir_ = path;

            return Operation{OperationType::Mkdir, path, "", "", ""};
        }

        case 3:
            return Operation{OperationType::List, RandomDir(), "", "", ""};

        case 4: {
            if (files_.empty()) {
                std::string path = NewFilePath();
                files_.push_back(path);
                hot_dir_ = ParentPath(path);

                return Operation{OperationType::Write, path, "", "hello", ""};
            }

            const size_t index = RandomIndex(files_.size());

            std::string src = files_[index];
            std::string dest = NewFilePath();

            files_[index] = dest;
            hot_dir_ = ParentPath(dest);

            return Operation{OperationType::Move, src, dest, "", ""};
        }

        case 5:
            return Operation{OperationType::Find, RandomDir(), "", "", "*"};
    }

    return Operation{OperationType::Read, RandomFile(), "", "", ""};
}

std::string WorkloadGenerator::RandomFile() {
    if (files_.empty()) {
        throw std::runtime_error("No files available");
    }

    if (Probability(cfg_.locality) && !hot_dir_.empty()) {
        std::vector<std::string> local_files;

        const std::string prefix = (hot_dir_ == "/") ? "/" : hot_dir_ + "/";

        for (const auto& file : files_) {
            if (file.rfind(prefix, 0) == 0) {
                local_files.push_back(file);
            }
        }

        if (!local_files.empty()) {
            const std::string& file = local_files[RandomIndex(local_files.size())];
            hot_dir_ = ParentPath(file);
            return file;
        }
    }

    const std::string& file = files_[RandomIndex(files_.size())];
    hot_dir_ = ParentPath(file);

    return file;
}

std::string WorkloadGenerator::RandomDir() {
    if (dirs_.empty()) {
        throw std::runtime_error("No directories available");
    }

    if (Probability(cfg_.locality) && !hot_dir_.empty()) {
        return hot_dir_;
    }

    const std::string& dir = dirs_[RandomIndex(dirs_.size())];
    hot_dir_ = dir;

    return dir;
}

std::string WorkloadGenerator::NewFilePath() {
    std::string dir = RandomDir();

    return JoinPath(
        dir,
        "new_file_" + std::to_string(next_file_id_++) + ".txt"
    );
}

std::string WorkloadGenerator::NewDirPath() {
    std::string dir = RandomDir();

    return JoinPath(
        dir,
        "new_dir_" + std::to_string(next_dir_id_++)
    );
}

size_t WorkloadGenerator::RandomIndex(size_t n) {
    if (n == 0) {
        throw std::runtime_error("Cannot choose random index from empty range");
    }

    if (cfg_.dist == "zipf") {
        std::vector<double> weights(n);

        for (size_t i = 0; i < n; ++i) {
            weights[i] = 1.0 / std::pow(static_cast<double>(i + 1), cfg_.zipf_s);
        }

        std::discrete_distribution<size_t> dist(weights.begin(), weights.end());
        return dist(rng_);
    }

    std::uniform_int_distribution<size_t> dist(0, n - 1);
    return dist(rng_);
}

bool WorkloadGenerator::Probability(double p) {
    std::bernoulli_distribution dist(p);
    return dist(rng_);
}

std::string WorkloadGenerator::JoinPath(const std::string& dir, const std::string& name) {
    if (dir.empty() || dir == "/") {
        return "/" + name;
    }

    return dir + "/" + name;
}

std::string WorkloadGenerator::ParentPath(const std::string& path) {
    if (path.empty() || path == "/") {
        return "/";
    }

    const size_t pos = path.find_last_of('/');

    if (pos == std::string::npos || pos == 0) {
        return "/";
    }

    return path.substr(0, pos);
}