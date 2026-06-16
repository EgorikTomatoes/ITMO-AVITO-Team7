#include <gtest/gtest.h>

#include "case_A/case_a.hpp"
#include "case_B/case_b.hpp"
#include "case_C/case_c.hpp"

#include <string>

template <typename T>
class EdgeCasesTest : public ::testing::Test {
protected:
    T fs;
    void SetUp() override {
        fs.Clear();
    }
};

using FSTypes = ::testing::Types<Case_A, Case_B, Case_C>;
TYPED_TEST_SUITE(EdgeCasesTest, FSTypes);


// пустой путь отвергается всеми операциями
TYPED_TEST(EdgeCasesTest, EmptyPathRejected) {
    EXPECT_FALSE(this->fs.WriteToFile("", "data"));
    EXPECT_FALSE(this->fs.MakeDir(""));
    EXPECT_FALSE(this->fs.ReadFile("").has_value());
    EXPECT_FALSE(this->fs.Exists(""));
    EXPECT_TRUE(this->fs.List("").empty());
    EXPECT_TRUE(this->fs.Find("", "*").empty());
}

// относительный путь (без ведущего /) отвергается
TYPED_TEST(EdgeCasesTest, RelativePathRejected) {
    EXPECT_FALSE(this->fs.MakeDir("relative"));
    EXPECT_FALSE(this->fs.WriteToFile("relative/file", "data"));
    EXPECT_FALSE(this->fs.Exists("relative"));
}

// путь с двойным слешем отвергается
TYPED_TEST(EdgeCasesTest, DoubleSlashPathRejected) {
    EXPECT_FALSE(this->fs.MakeDir("/a//b"));
    EXPECT_FALSE(this->fs.WriteToFile("/a//b", "data"));
    EXPECT_FALSE(this->fs.Exists("/a//b"));
}

// пути с компонентами "." и ".." отвергаются
TYPED_TEST(EdgeCasesTest, DotComponentsRejected) {
    ASSERT_TRUE(this->fs.MakeDir("/dir"));

    EXPECT_FALSE(this->fs.MakeDir("/dir/./sub"));
    EXPECT_FALSE(this->fs.MakeDir("/dir/../sub"));
    EXPECT_FALSE(this->fs.WriteToFile("/dir/./file", "x"));
    EXPECT_FALSE(this->fs.Exists("/dir/../dir"));
}


// корень нельзя создать, перезаписать, переместить или удалить запись о нём
TYPED_TEST(EdgeCasesTest, RootIsProtected) {
    EXPECT_FALSE(this->fs.MakeDir("/"));
    EXPECT_FALSE(this->fs.WriteToFile("/", "data"));
    EXPECT_FALSE(this->fs.Move("/", "/elsewhere"));
    EXPECT_FALSE(this->fs.ReadFile("/").has_value());
    EXPECT_TRUE(this->fs.Exists("/"));
}

// List корня возвращает узлы верхнего уровня
TYPED_TEST(EdgeCasesTest, ListRootReturnsTopLevel) {
    ASSERT_TRUE(this->fs.MakeDir("/a"));
    ASSERT_TRUE(this->fs.MakeDir("/b"));
    ASSERT_TRUE(this->fs.WriteToFile("/c", "x"));

    auto entries = this->fs.List("/");
    EXPECT_EQ(entries.size(), 3u);
}

// нельзя создать директорию там, где уже есть файл
TYPED_TEST(EdgeCasesTest, MakeDirOverFileFails) {
    ASSERT_TRUE(this->fs.WriteToFile("/node", "data"));
    EXPECT_FALSE(this->fs.MakeDir("/node"));

    // узел остаётся файлом
    EXPECT_EQ(this->fs.ReadFile("/node").value_or(""), "data");
}

// нельзя записать файл по пути существующей директории
TYPED_TEST(EdgeCasesTest, WriteOverDirectoryFails) {
    ASSERT_TRUE(this->fs.MakeDir("/node"));
    EXPECT_FALSE(this->fs.WriteToFile("/node", "data"));

    // узел остаётся директорией
    EXPECT_FALSE(this->fs.ReadFile("/node").has_value());
    EXPECT_TRUE(this->fs.Exists("/node"));
}


// глубокая вложенность директорий поддерживается
TYPED_TEST(EdgeCasesTest, DeepNesting) {
    std::string path;
    const int depth = 50;
    for (int i = 0; i < depth; ++i) {
        path += "/level_" + std::to_string(i);
        ASSERT_TRUE(this->fs.MakeDir(path)) << "failed at depth " << i;
    }
    ASSERT_TRUE(this->fs.WriteToFile(path + "/leaf", "deep"));

    EXPECT_TRUE(this->fs.Exists(path));
    EXPECT_TRUE(this->fs.Exists(path + "/leaf"));
    EXPECT_EQ(this->fs.ReadFile(path + "/leaf").value_or(""), "deep");
}

// много потомков в одной директории
TYPED_TEST(EdgeCasesTest, WideDirectory) {
    ASSERT_TRUE(this->fs.MakeDir("/wide"));
    const int width = 100;
    for (int i = 0; i < width; ++i) {
        ASSERT_TRUE(this->fs.WriteToFile("/wide/file_" + std::to_string(i), "x"));
    }

    EXPECT_EQ(this->fs.List("/wide").size(), static_cast<size_t>(width));
}


// шаблон "?" соответствует ровно одному символу
TYPED_TEST(EdgeCasesTest, FindSingleCharWildcard) {
    ASSERT_TRUE(this->fs.MakeDir("/dir"));
    ASSERT_TRUE(this->fs.WriteToFile("/dir/a.txt", "1"));
    ASSERT_TRUE(this->fs.WriteToFile("/dir/ab.txt", "2"));

    auto matches = this->fs.Find("/dir", "?.txt");
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches.front(), "/dir/a.txt");
}

// поиск с невалидным стартовым путём возвращает пустой список
TYPED_TEST(EdgeCasesTest, FindInvalidPathReturnsEmpty) {
    EXPECT_TRUE(this->fs.Find("not_a_path", "*").empty());
}


// повторный Clear на пустой системе безопасен и сохраняет инвариант корня
TYPED_TEST(EdgeCasesTest, RepeatedClearIsSafe) {
    this->fs.Clear();
    this->fs.Clear();

    EXPECT_TRUE(this->fs.Exists("/"));
    EXPECT_TRUE(this->fs.List("/").empty());
}

// после Clear система снова готова к работе
TYPED_TEST(EdgeCasesTest, UsableAfterClear) {
    ASSERT_TRUE(this->fs.MakeDir("/old"));
    this->fs.Clear();

    EXPECT_FALSE(this->fs.Exists("/old"));
    EXPECT_TRUE(this->fs.MakeDir("/new"));
    EXPECT_TRUE(this->fs.Exists("/new"));
}
