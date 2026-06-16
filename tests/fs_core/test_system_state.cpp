#include <gtest/gtest.h>

#include "case_A/case_a.hpp"
#include "case_B/case_b.hpp"
#include "case_C/case_c.hpp"

template <typename T>
class SystemStateTest : public ::testing::Test {
protected:
    T fs;
    void SetUp() override {
        fs.Clear();
    }
};

using FSTypes = ::testing::Types<Case_A, Case_B, Case_C>;
TYPED_TEST_SUITE(SystemStateTest, FSTypes);

// начальное состояние: только корень
TYPED_TEST(SystemStateTest, InitialState) {
    auto state = this->fs.SystemState();
    ASSERT_TRUE(state.has_value());

    EXPECT_EQ(state->dir_count, 1u);
    EXPECT_EQ(state->file_count, 0u);
    EXPECT_EQ(state->node_count, 1u);
    EXPECT_EQ(state->total_file_bytes, 0u);
    EXPECT_GT(state->total_size_bytes, 0u);
}

// счётчики корректно растут при добавлении узлов
TYPED_TEST(SystemStateTest, CountersGrowWithNodes) {
    ASSERT_TRUE(this->fs.MakeDir("/d1"));
    ASSERT_TRUE(this->fs.MakeDir("/d1/d2"));
    ASSERT_TRUE(this->fs.WriteToFile("/d1/d2/file", "hello")); // 5 байт

    auto state = this->fs.SystemState();
    ASSERT_TRUE(state.has_value());

    EXPECT_EQ(state->dir_count, 3u);
    EXPECT_EQ(state->file_count, 1u);
    EXPECT_EQ(state->total_file_bytes, 5u);
    EXPECT_EQ(state->node_count, state->dir_count + state->file_count);
    EXPECT_GT(state->total_size_bytes, state->total_file_bytes);
}

// перезапись файла обновляет учтённые байты, но не число файлов
TYPED_TEST(SystemStateTest, OverwriteUpdatesByteCount) {
    ASSERT_TRUE(this->fs.WriteToFile("/file", "abcd"));
    ASSERT_TRUE(this->fs.WriteToFile("/file", "ab"));

    auto state = this->fs.SystemState();
    ASSERT_TRUE(state.has_value());

    EXPECT_EQ(state->file_count, 1u);
    EXPECT_EQ(state->total_file_bytes, 2u);
}

// суммарный размер данных учитывает несколько файлов
TYPED_TEST(SystemStateTest, TotalFileBytesAccumulate) {
    ASSERT_TRUE(this->fs.WriteToFile("/a", "123"));
    ASSERT_TRUE(this->fs.WriteToFile("/b", "45"));
    ASSERT_TRUE(this->fs.WriteToFile("/c", ""));

    auto state = this->fs.SystemState();
    ASSERT_TRUE(state.has_value());

    EXPECT_EQ(state->file_count, 3u);
    EXPECT_EQ(state->total_file_bytes, 5u);
}

// перемещение не меняет счётчики и объём данных
TYPED_TEST(SystemStateTest, MoveDoesNotChangeCounters) {
    ASSERT_TRUE(this->fs.MakeDir("/src"));
    ASSERT_TRUE(this->fs.MakeDir("/dst"));
    ASSERT_TRUE(this->fs.WriteToFile("/src/file", "data")); // 4 байта

    auto before = this->fs.SystemState();
    ASSERT_TRUE(before.has_value());

    ASSERT_TRUE(this->fs.Move("/src/file", "/dst"));

    auto after = this->fs.SystemState();
    ASSERT_TRUE(after.has_value());

    EXPECT_EQ(after->dir_count, before->dir_count);
    EXPECT_EQ(after->file_count, before->file_count);
    EXPECT_EQ(after->node_count, before->node_count);
    EXPECT_EQ(after->total_file_bytes, before->total_file_bytes);
}

// Clear полностью сбрасывает состояние к начальному
TYPED_TEST(SystemStateTest, ClearResetsState) {
    ASSERT_TRUE(this->fs.MakeDir("/dir"));
    ASSERT_TRUE(this->fs.WriteToFile("/dir/file", "payload"));

    this->fs.Clear();

    auto state = this->fs.SystemState();
    ASSERT_TRUE(state.has_value());

    EXPECT_EQ(state->dir_count, 1u);
    EXPECT_EQ(state->file_count, 0u);
    EXPECT_EQ(state->node_count, 1u);
    EXPECT_EQ(state->total_file_bytes, 0u);
}


// NodeState существующей директории
TYPED_TEST(SystemStateTest, NodeStateForDirectory) {
    ASSERT_TRUE(this->fs.MakeDir("/dir"));

    auto info = this->fs.NodeState("/dir");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->type, NodeType::Dir);
    EXPECT_EQ(info->path, "/dir");
}

// NodeState существующего файла
TYPED_TEST(SystemStateTest, NodeStateForFile) {
    ASSERT_TRUE(this->fs.WriteToFile("/file", "data"));

    auto info = this->fs.NodeState("/file");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->type, NodeType::File);
    EXPECT_EQ(info->path, "/file");
}

// NodeState корня
TYPED_TEST(SystemStateTest, NodeStateForRoot) {
    auto info = this->fs.NodeState("/");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->type, NodeType::Dir);
}

// NodeState несуществующего узла возвращает nullopt
TYPED_TEST(SystemStateTest, NodeStateMissingReturnsNullopt) {
    EXPECT_FALSE(this->fs.NodeState("/missing").has_value());
}

// NodeState невалидного пути возвращает nullopt
TYPED_TEST(SystemStateTest, NodeStateInvalidPathReturnsNullopt) {
    EXPECT_FALSE(this->fs.NodeState("invalid").has_value());
    EXPECT_FALSE(this->fs.NodeState("/a//b").has_value());
}
