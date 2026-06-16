#pragma once

#include <cmath>
#include <stdexcept>
#include <string>

struct WorkloadConfig {
    int D = 3;
    int W = 10;
    double F = 0.6;

    double p_read = 0.4;
    double p_write = 0.2;
    double p_mkdir = 0.1;
    double p_ls = 0.1;
    double p_mv = 0.1;
    double p_find = 0.1;

    std::string profile_name = "mixed";

    std::string dist = "uniform";
    double zipf_s = 1.5;
    double locality = 0.0;

    int ops = 200;
    int repeats = 1;
    int seed = 42;

    void Validate() const {
        const double sum = p_read + p_write + p_mkdir + p_ls + p_mv + p_find;

        if (std::abs(sum - 1.0) > 1e-9) {
            throw std::runtime_error("Operation probabilities must sum to 1");
        }

        if (p_read < 0.0 || p_write < 0.0 || p_mkdir < 0.0 ||
            p_ls < 0.0 || p_mv < 0.0 || p_find < 0.0) {
            throw std::runtime_error("Operation probabilities must be non-negative");
        }

        if (D <= 0 || W <= 0 || F < 0.0 || F > 1.0) {
            throw std::runtime_error("Invalid tree parameters");
        }

        if (dist != "uniform" && dist != "zipf") {
            throw std::runtime_error("dist must be either 'uniform' or 'zipf'");
        }

        if (dist == "zipf" && zipf_s <= 0.0) {
            throw std::runtime_error("zipf_s must be positive");
        }

        if (locality < 0.0 || locality > 1.0) {
            throw std::runtime_error("locality must be in [0, 1]");
        }

        if (ops <= 0 || repeats <= 0) {
            throw std::runtime_error("Invalid experiment parameters");
        }
    }
};