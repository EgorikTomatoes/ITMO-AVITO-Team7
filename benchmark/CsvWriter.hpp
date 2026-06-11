#pragma once

#include "BenchmarkRunner.hpp"
#include "WorkloadConfig.hpp"

#include <iosfwd>
#include <string>

void WriteCsvHeader(std::ostream& out);

void WriteCsvRow(
    std::ostream& out,
    const std::string& approach,
    const WorkloadConfig& cfg,
    int repeat,
    const BenchmarkResult& result
);