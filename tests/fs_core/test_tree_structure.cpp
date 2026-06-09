#include <gtest/gtest.h>

#include "case_A/case_a.hpp"
// #include "case_B/case_b.hpp"
// #include "case_C/case_c.hpp"

template <typename T>
class TreeStructureTest : public ::testing::Test {
protected:
    T fs;
    void SetUp() override { 
        fs.Clear(); 
    }
};

using FSTypes = ::testing::Types<Case_A /*, Case_B, Case_C*/>;
TYPED_TEST_SUITE(TreeStructureTest, FSTypes);

// внтуренние узлы(ветки)
TYPED_TEST(TreeStructureTest, AddBranchNodes) {
    EXPECT_TRUE(this->fs.MakeDir("/branch_A"));
    EXPECT_TRUE(this->fs.MakeDir("/branch_A/sub_branch_B"));
    
    EXPECT_TRUE(this->fs.Exists("/branch_A"));
    EXPECT_TRUE(this->fs.Exists("/branch_A/sub_branch_B"));
    EXPECT_FALSE(this->fs.Exists("/branch_C"));
}

// создание новых листов с данными
TYPED_TEST(TreeStructureTest, AddLeafNodesWithData) {
    this->fs.MakeDir("/data_branch");

    EXPECT_TRUE(this->fs.WriteToFile("/data_branch/leaf_1", "payload_123"));
    EXPECT_TRUE(this->fs.Exists("/data_branch/leaf_1"));
    
    auto content = this->fs.ReadFile("/data_branch/leaf_1");
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(content.value(), "payload_123"); 
}

// изменение родителя узла
TYPED_TEST(TreeStructureTest, ChangeNodeParent) {
    this->fs.MakeDir("/branch_A");
    this->fs.MakeDir("/branch_B");
    
    this->fs.WriteToFile("/branch_A/leaf_node", "payload_data");

    EXPECT_TRUE(this->fs.Move("/branch_A/leaf_node", "/branch_B/leaf_node"));
    
    EXPECT_FALSE(this->fs.Exists("/branch_A/leaf_node"));
    EXPECT_TRUE(this->fs.Exists("/branch_B/leaf_node"));
    
    EXPECT_EQ(this->fs.ReadFile("/branch_B/leaf_node").value_or(""), "payload_data");
}