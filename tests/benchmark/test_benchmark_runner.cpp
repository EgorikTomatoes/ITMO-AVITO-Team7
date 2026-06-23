#include <gtest/gtest.h>

#include "BenchmarkRunner.hpp"
#include "Operations.hpp"
#include "fake_file_system.hpp"

#include <vector>

// тут проверяются по весьма ясным причинам только структурные поведения раннера (они же диспетчеризация операций, источник памяти, граничные случаи), но не конкретные тайминги)

TEST(BenchmarkRunnerTest, EmptyOperationsYieldZeroResult) {
    FakeFileSystem fs;
    BenchmarkRunner runner;

    BenchmarkResult result = runner.Run(fs, {});

    EXPECT_EQ(result.avg_latency_us, 0.0);
    EXPECT_EQ(result.p99_latency_us, 0.0);
    EXPECT_EQ(result.throughput_ops_sec, 0.0);
    EXPECT_EQ(result.memory_bytes, 0u);
    EXPECT_EQ(fs.TotalCalls(), 0);
}

// RunSetup выполняет каждую переданную операцию ровно один раз
TEST(BenchmarkRunnerTest, RunSetupAppliesEachOperation) {
    FakeFileSystem fs;
    BenchmarkRunner runner;

    std::vector<Operation> setup = {
        Operation{OperationType::Mkdir, "/a", "", "", ""},
        Operation{OperationType::Mkdir, "/b", "", "", ""},
        Operation{OperationType::Write, "/a/f", "", "data", ""},
    };

    runner.RunSetup(fs, setup);

    EXPECT_EQ(fs.mkdir_calls, 2);
    EXPECT_EQ(fs.write_calls, 1);
    EXPECT_EQ(fs.TotalCalls(), 3);
}

// каждый тип операции диспетчеризуется в соответствующий метод ФС
TEST(BenchmarkRunnerTest, RunDispatchesOperationsByType) {
    FakeFileSystem fs;
    BenchmarkRunner runner;

    std::vector<Operation> ops = {
        Operation{OperationType::Read, "/f", "", "", ""},
        Operation{OperationType::Write, "/f", "", "data", ""},
        Operation{OperationType::Mkdir, "/d", "", "", ""},
        Operation{OperationType::List, "/d", "", "", ""},
        Operation{OperationType::Move, "/f", "/g", "", ""},
        Operation{OperationType::Find, "/", "", "", "*"},
    };

    BenchmarkResult result = runner.Run(fs, ops);

    EXPECT_EQ(fs.read_calls, 1);
    EXPECT_EQ(fs.write_calls, 1);
    EXPECT_EQ(fs.mkdir_calls, 1);
    EXPECT_EQ(fs.list_calls, 1);
    EXPECT_EQ(fs.move_calls, 1);
    EXPECT_EQ(fs.find_calls, 1);

    // метрики неотрицательны и память берётся из SystemState()
    EXPECT_GE(result.avg_latency_us, 0.0);
    EXPECT_GE(result.p99_latency_us, 0.0);
    EXPECT_GE(result.throughput_ops_sec, 0.0);
    EXPECT_EQ(result.memory_bytes, FakeFileSystem::kMemoryBytes);
}

// memory_bytes результата берётся из SystemState() нашей шедевро фс
TEST(BenchmarkRunnerTest, MemoryComesFromSystemState) {
    FakeFileSystem fs;
    BenchmarkRunner runner;

    std::vector<Operation> ops = {
        Operation{OperationType::Read, "/f", "", "", ""},
    };

    BenchmarkResult result = runner.Run(fs, ops);
    EXPECT_EQ(result.memory_bytes, FakeFileSystem::kMemoryBytes);
}
