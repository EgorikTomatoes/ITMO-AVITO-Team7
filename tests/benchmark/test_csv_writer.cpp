#include <gtest/gtest.h>

#include "BenchmarkRunner.hpp"
#include "CsvWriter.hpp"
#include "WorkloadConfig.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace {

// разбивает одну CSV-строку на поля по запятой
std::vector<std::string> SplitCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    for (char c : line) {
        if (c == ',') {
            fields.push_back(current);
            current.clear();
        } else if (c != '\n' && c != '\r') {
            current += c;
        }
    }
    fields.push_back(current);
    return fields;
}

constexpr size_t kExpectedColumnCount = 24;

} 

TEST(CsvWriterTest, HeaderHasExpectedColumns) {
    std::ostringstream out;
    WriteCsvHeader(out);

    auto fields = SplitCsvLine(out.str());

    ASSERT_EQ(fields.size(), kExpectedColumnCount);
    EXPECT_EQ(fields.front(), "approach");
    EXPECT_EQ(fields.back(), "memory_bytes_std");
}

TEST(CsvWriterTest, RowMatchesHeaderColumnCount) {
    std::ostringstream header_out;
    WriteCsvHeader(header_out);
    const size_t header_columns = SplitCsvLine(header_out.str()).size();

    WorkloadConfig cfg;
    BenchmarkResult result;

    std::ostringstream row_out;
    WriteCsvRow(row_out, "case_a", cfg, result);

    auto row_fields = SplitCsvLine(row_out.str());
    EXPECT_EQ(row_fields.size(), header_columns);
    EXPECT_EQ(row_fields.size(), kExpectedColumnCount);
}

TEST(CsvWriterTest, RowWritesValuesInExpectedColumns) {
    WorkloadConfig cfg;
    cfg.profile_name = "profileX";
    cfg.D = 7;
    cfg.W = 11;

    BenchmarkResult result;

    std::ostringstream out;
    WriteCsvRow(out, "case_b", cfg, result);

    auto fields = SplitCsvLine(out.str());
    ASSERT_EQ(fields.size(), kExpectedColumnCount);

    EXPECT_EQ(fields[0], "case_b"); // approach
    EXPECT_EQ(fields[1], "7");// d
    EXPECT_EQ(fields[2], "11");// w
    EXPECT_EQ(fields[4], "profileX"); 
}
