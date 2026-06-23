#include <gtest/gtest.h>

#include "BenchmarkRunner.hpp"
#include "Operations.hpp"
#include "WorkloadConfig.hpp"
#include "WorkloadGenerator.hpp"
#include "fake_file_system.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace {

bool LooksAbsolute(const std::string& path) {
    return !path.empty() && path.front() == '/' && path.find("//") == std::string::npos;
}

bool SameOperation(const Operation& a, const Operation& b) {
    return a.type == b.type &&
           a.path1 == b.path1 &&
           a.path2 == b.path2 &&
           a.data == b.data &&
           a.pattern == b.pattern;
}

bool SameSequence(const std::vector<Operation>& a, const std::vector<Operation>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (!SameOperation(a[i], b[i])) return false;
    }
    return true;
}


void ExpectOperationPathsValid(const Operation& op) {
    EXPECT_TRUE(LooksAbsolute(op.path1)) << "path1=" << op.path1;
    if (op.type == OperationType::Move) {
        EXPECT_TRUE(LooksAbsolute(op.path2)) << "path2=" << op.path2;
    }
}

} 


TEST(WorkloadGeneratorTest, ConstructorRejectsInvalidConfig) {
    WorkloadConfig cfg;
    cfg.ops = 0;
    EXPECT_THROW(WorkloadGenerator generator(cfg), std::runtime_error);
}


TEST(WorkloadGeneratorTest, GenerateMeasuredReturnsRequestedOps) {
    WorkloadConfig cfg;
    cfg.ops = 137;

    WorkloadGenerator generator(cfg);
    auto ops = generator.GenerateMeasured();

    EXPECT_EQ(ops.size(), static_cast<size_t>(cfg.ops));
}


TEST(WorkloadGeneratorTest, SetupIsNotEmpty) {
    WorkloadConfig cfg;
    WorkloadGenerator generator(cfg);

    auto setup = generator.GenerateSetup();
    EXPECT_FALSE(setup.empty());
}


TEST(WorkloadGeneratorTest, GeneratedPathsAreValid) {
    WorkloadConfig cfg;
    cfg.ops = 500;

    WorkloadGenerator generator(cfg);

    for (const auto& op : generator.GenerateSetup()) {
        ExpectOperationPathsValid(op);
    }
    for (const auto& op : generator.GenerateMeasured()) {
        ExpectOperationPathsValid(op);
    }
}


TEST(WorkloadGeneratorTest, DeterministicForSameSeed) {
    WorkloadConfig cfg;
    cfg.ops = 200;
    cfg.seed = 12345;

    WorkloadGenerator first(cfg);
    WorkloadGenerator second(cfg);

    auto first_setup = first.GenerateSetup();
    auto second_setup = second.GenerateSetup();
    EXPECT_TRUE(SameSequence(first_setup, second_setup));

    auto first_measured = first.GenerateMeasured();
    auto second_measured = second.GenerateMeasured();
    EXPECT_TRUE(SameSequence(first_measured, second_measured));
}


TEST(WorkloadGeneratorTest, GeneratedWorkloadIsExecutable) {
    WorkloadConfig cfg;
    cfg.ops = 300;

    WorkloadGenerator generator(cfg);
    auto setup = generator.GenerateSetup();
    auto measured = generator.GenerateMeasured();

    FakeFileSystem fs;
    BenchmarkRunner runner;

    EXPECT_NO_THROW(runner.RunSetup(fs, setup));
    EXPECT_NO_THROW(runner.Run(fs, measured));

    EXPECT_EQ(fs.TotalCalls(), static_cast<int>(setup.size() + measured.size()));
}
