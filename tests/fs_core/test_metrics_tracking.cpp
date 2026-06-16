#include <gtest/gtest.h>

#include "case_A/case_a.hpp"
#include "case_B/case_b.hpp"
#include "case_C/case_c.hpp"

#include <string>

template <typename T>
class MetricsTest : public ::testing::Test {
protected:
    T fs;
    void SetUp() override { 
        fs.Clear(); 
    }
};

using FSTypes = ::testing::Types<Case_A, Case_B, Case_C>;
TYPED_TEST_SUITE(MetricsTest, FSTypes);

// проверка обновления всех счетчиков и логических инвариантов
TYPED_TEST(MetricsTest, SystemCountersUpdateProperly) {
    auto initial_state = this->fs.SystemState();
    ASSERT_TRUE(initial_state.has_value());
    EXPECT_EQ(initial_state->dir_count, 1); // корень "/"
    EXPECT_EQ(initial_state->file_count, 0);
    EXPECT_EQ(initial_state->node_count, 1); // корень это 1 узел
    EXPECT_EQ(initial_state->total_file_bytes, 0);

    EXPECT_GT(initial_state->total_size_bytes, 0); 
    this->fs.MakeDir("/branch_1");
    this->fs.MakeDir("/branch_1/branch_2");
    this->fs.WriteToFile("/branch_1/branch_2/leaf", "hello"); // 5 байт
    auto final_state = this->fs.SystemState();
    ASSERT_TRUE(final_state.has_value());
    
    EXPECT_EQ(final_state->dir_count, 3); // "/", "branch_1", "branch_2"
    EXPECT_EQ(final_state->file_count, 1); // "leaf"
    EXPECT_EQ(final_state->total_file_bytes, 5); 
    EXPECT_EQ(final_state->node_count, final_state->dir_count + final_state->file_count);
    EXPECT_GT(final_state->total_size_bytes, final_state->total_file_bytes);
}

// получение функцией информации о конкретном узле
TYPED_TEST(MetricsTest, NodeStateCheck) {
    this->fs.MakeDir("/branch");
    this->fs.WriteToFile("/branch/leaf", "data");

    auto branch_info = this->fs.NodeState("/branch");
    ASSERT_TRUE(branch_info.has_value());
    EXPECT_EQ(branch_info->type, NodeType::Dir);
    EXPECT_EQ(branch_info->path, "/branch");
    // size_bytes отражает накладные расходы памяти на узел (а не размер данных),
    // поэтому для любого существующего узла он строго положителен
    EXPECT_GT(branch_info->size_bytes, 0u);

    auto leaf_info = this->fs.NodeState("/branch/leaf");
    ASSERT_TRUE(leaf_info.has_value());
    EXPECT_EQ(leaf_info->type, NodeType::File);
    EXPECT_EQ(leaf_info->path, "/branch/leaf");
    EXPECT_GT(leaf_info->size_bytes, 0u);
}

// размер узла в NodeState не зависит от объёма данных файла
TYPED_TEST(MetricsTest, NodeSizeIndependentOfFileData) {
    ASSERT_TRUE(this->fs.WriteToFile("/file", "x"));
    auto small = this->fs.NodeState("/file");
    ASSERT_TRUE(small.has_value());

    ASSERT_TRUE(this->fs.WriteToFile("/file", std::string(10000, 'y')));
    auto large = this->fs.NodeState("/file");
    ASSERT_TRUE(large.has_value());

    // данные файла не учитываются в size_bytes узла
    EXPECT_EQ(small->size_bytes, large->size_bytes);
    // но объём хранимых данных при этом действительно вырос
    auto state = this->fs.SystemState();
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(state->total_file_bytes, 10000u);
}

// полное обнуление метрик при очистке
TYPED_TEST(MetricsTest, ClearResetsAllMetrics) {
    this->fs.MakeDir("/temp_branch");
    this->fs.WriteToFile("/temp_branch/temp_leaf", "temporary_data");
    
    this->fs.Clear();
    
    auto cleared_state = this->fs.SystemState();
    ASSERT_TRUE(cleared_state.has_value());
    
    EXPECT_EQ(cleared_state->dir_count, 1); 
    EXPECT_EQ(cleared_state->file_count, 0);
    EXPECT_EQ(cleared_state->node_count, 1);
    EXPECT_EQ(cleared_state->total_file_bytes, 0);
}