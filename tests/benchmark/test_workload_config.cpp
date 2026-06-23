#include <gtest/gtest.h>

#include "WorkloadConfig.hpp"

#include <stdexcept>

TEST(WorkloadConfigTest, DefaultConfigIsValid) {
    WorkloadConfig cfg;
    EXPECT_NO_THROW(cfg.Validate());
}

TEST(WorkloadConfigTest, ProbabilitiesMustSumToOne) {
    WorkloadConfig cfg;
    cfg.p_read = 0.5; 
    EXPECT_THROW(cfg.Validate(), std::runtime_error);
}


TEST(WorkloadConfigTest, NegativeProbabilityRejected) {
    WorkloadConfig cfg;
    cfg.p_read = -0.1;
    cfg.p_write = 0.6;
    cfg.p_mkdir = 0.1;
    cfg.p_ls = 0.1;
    cfg.p_mv = 0.1;
    cfg.p_find = 0.2;
    ASSERT_DOUBLE_EQ(cfg.p_read + cfg.p_write + cfg.p_mkdir + cfg.p_ls + cfg.p_mv + cfg.p_find, 1.0);
    EXPECT_THROW(cfg.Validate(), std::runtime_error);
}

TEST(WorkloadConfigTest, InvalidTreeParamsRejected) {
    {
        WorkloadConfig cfg;
        cfg.D = 0;
        EXPECT_THROW(cfg.Validate(), std::runtime_error);
    }
    {
        WorkloadConfig cfg;
        cfg.W = 0;
        EXPECT_THROW(cfg.Validate(), std::runtime_error);
    }
    {
        WorkloadConfig cfg;
        cfg.F = -0.1;
        EXPECT_THROW(cfg.Validate(), std::runtime_error);
    }
    {
        WorkloadConfig cfg;
        cfg.F = 1.5;
        EXPECT_THROW(cfg.Validate(), std::runtime_error);
    }
}

TEST(WorkloadConfigTest, InvalidDistributionRejected) {
    WorkloadConfig cfg;
    cfg.dist = "gaussian";
    EXPECT_THROW(cfg.Validate(), std::runtime_error);
}

// для распределения Ципфа параметр s должен быть положительным.
TEST(WorkloadConfigTest, NonPositiveZipfParamRejected) {
    WorkloadConfig cfg;
    cfg.dist = "zipf";
    cfg.zipf_s = 0.0;
    EXPECT_THROW(cfg.Validate(), std::runtime_error);
}

TEST(WorkloadConfigTest, ValidZipfConfigAccepted) {
    WorkloadConfig cfg;
    cfg.dist = "zipf";
    cfg.zipf_s = 1.5;
    EXPECT_NO_THROW(cfg.Validate());
}

TEST(WorkloadConfigTest, LocalityOutOfRangeRejected) {
    {
        WorkloadConfig cfg;
        cfg.locality = -0.1;
        EXPECT_THROW(cfg.Validate(), std::runtime_error);
    }
    {
        WorkloadConfig cfg;
        cfg.locality = 1.1;
        EXPECT_THROW(cfg.Validate(), std::runtime_error);
    }
}

// параметры больше 0
TEST(WorkloadConfigTest, InvalidExperimentParamsRejected) {
    {
        WorkloadConfig cfg;
        cfg.ops = 0;
        EXPECT_THROW(cfg.Validate(), std::runtime_error);
    }
    {
        WorkloadConfig cfg;
        cfg.repeats = 0;
        EXPECT_THROW(cfg.Validate(), std::runtime_error);
    }
}
