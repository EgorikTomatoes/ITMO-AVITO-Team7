#include <gtest/gtest.h>

#include "case_A/case_a.hpp"
#include "case_B/case_b.hpp"
#include "case_C/case_c.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace {
std::vector<std::string> Sorted(std::vector<std::string> values) {
    std::sort(values.begin(), values.end());
    return values;
}
}

template <typename T>
class CrudTest : public ::testing::Test {
protected:
    T fs;
    void SetUp() override {
        fs.Clear();
    }
};

using FSTypes = ::testing::Types<Case_A, Case_B, Case_C>;
TYPED_TEST_SUITE(CrudTest, FSTypes);


// запись нового файла и последующее чтение
TYPED_TEST(CrudTest, WriteThenReadReturnsSameData) {
    EXPECT_TRUE(this->fs.WriteToFile("/file", "hello world"));

    auto content = this->fs.ReadFile("/file");
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(content.value(), "hello world");
}

// перезапись существующего файла заменяет содержимое
TYPED_TEST(CrudTest, OverwriteReplacesContent) {
    ASSERT_TRUE(this->fs.WriteToFile("/file", "first"));
    EXPECT_TRUE(this->fs.WriteToFile("/file", "second"));

    auto content = this->fs.ReadFile("/file");
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(content.value(), "second");
}

// запись пустого содержимого допустима
TYPED_TEST(CrudTest, WriteEmptyContent) {
    EXPECT_TRUE(this->fs.WriteToFile("/empty", ""));

    auto content = this->fs.ReadFile("/empty");
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(content.value(), "");
}

// запись в файл вложенной директории
TYPED_TEST(CrudTest, WriteIntoNestedDirectory) {
    ASSERT_TRUE(this->fs.MakeDir("/dir"));
    EXPECT_TRUE(this->fs.WriteToFile("/dir/file", "payload"));
    EXPECT_EQ(this->fs.ReadFile("/dir/file").value_or(""), "payload");
}

// чтение несуществующего файла возвращает nullopt
TYPED_TEST(CrudTest, ReadMissingFileReturnsNullopt) {
    EXPECT_FALSE(this->fs.ReadFile("/no_such_file").has_value());
}

// чтение директории как файла возвращает nullopt
TYPED_TEST(CrudTest, ReadDirectoryReturnsNullopt) {
    ASSERT_TRUE(this->fs.MakeDir("/dir"));
    EXPECT_FALSE(this->fs.ReadFile("/dir").has_value());
}

// запись невозможна, если у файла нет родительской директории
TYPED_TEST(CrudTest, WriteFailsWhenParentMissing) {
    EXPECT_FALSE(this->fs.WriteToFile("/missing_dir/file", "data"));
    EXPECT_FALSE(this->fs.Exists("/missing_dir/file"));
}


// создание директории и проверка её существования
TYPED_TEST(CrudTest, MakeDirCreatesDirectory) {
    EXPECT_TRUE(this->fs.MakeDir("/dir"));
    EXPECT_TRUE(this->fs.Exists("/dir"));
}

// повторное создание той же директории запрещено
TYPED_TEST(CrudTest, MakeDirDuplicateFails) {
    ASSERT_TRUE(this->fs.MakeDir("/dir"));
    EXPECT_FALSE(this->fs.MakeDir("/dir"));
}

// нельзя создать директорию, если отсутствует родитель
TYPED_TEST(CrudTest, MakeDirFailsWhenParentMissing) {
    EXPECT_FALSE(this->fs.MakeDir("/missing/child"));
    EXPECT_FALSE(this->fs.Exists("/missing/child"));
}


// перечисление содержимого директории беспорядочно
TYPED_TEST(CrudTest, ListReturnsChildren) {
    ASSERT_TRUE(this->fs.MakeDir("/dir"));
    ASSERT_TRUE(this->fs.MakeDir("/dir/sub"));
    ASSERT_TRUE(this->fs.WriteToFile("/dir/file_a", "a"));
    ASSERT_TRUE(this->fs.WriteToFile("/dir/file_b", "b"));

    std::vector<std::string> expected = {"file_a", "file_b", "sub"};
    EXPECT_EQ(Sorted(this->fs.List("/dir")), Sorted(expected));
}

// List возвращает только прямых потомков, не внуков
TYPED_TEST(CrudTest, ListReturnsOnlyDirectChildren) {
    ASSERT_TRUE(this->fs.MakeDir("/dir"));
    ASSERT_TRUE(this->fs.MakeDir("/dir/sub"));
    ASSERT_TRUE(this->fs.WriteToFile("/dir/sub/deep", "x"));
    std::vector<std::string> expected = {"sub"};
    EXPECT_EQ(Sorted(this->fs.List("/dir")), Sorted(expected));
}

// List пустой директории возвращает пустой список
TYPED_TEST(CrudTest, ListEmptyDirectory) {
    ASSERT_TRUE(this->fs.MakeDir("/empty"));
    EXPECT_TRUE(this->fs.List("/empty").empty());
}

// List файла возвращает пустой список
TYPED_TEST(CrudTest, ListOnFileReturnsEmpty) {
    ASSERT_TRUE(this->fs.WriteToFile("/file", "data"));
    EXPECT_TRUE(this->fs.List("/file").empty());
}

// List несуществующего пути возвращает пустой список
TYPED_TEST(CrudTest, ListMissingPathReturnsEmpty) {
    EXPECT_TRUE(this->fs.List("/missing").empty());
}


// Exists различает существующие и отсутствующие узлы
TYPED_TEST(CrudTest, ExistsReflectsState) {
    EXPECT_FALSE(this->fs.Exists("/dir"));
    ASSERT_TRUE(this->fs.MakeDir("/dir"));
    EXPECT_TRUE(this->fs.Exists("/dir"));
    EXPECT_FALSE(this->fs.Exists("/dir/file"));
    ASSERT_TRUE(this->fs.WriteToFile("/dir/file", "x"));
    EXPECT_TRUE(this->fs.Exists("/dir/file"));
}

// корень существует всегда
TYPED_TEST(CrudTest, RootAlwaysExists) {
    EXPECT_TRUE(this->fs.Exists("/"));
}


// перемещение файла внутрь существующей директории сохраняет имя
TYPED_TEST(CrudTest, MoveFileIntoExistingDirectory) {
    ASSERT_TRUE(this->fs.MakeDir("/src"));
    ASSERT_TRUE(this->fs.MakeDir("/dst"));
    ASSERT_TRUE(this->fs.WriteToFile("/src/file", "data"));

    EXPECT_TRUE(this->fs.Move("/src/file", "/dst"));

    EXPECT_FALSE(this->fs.Exists("/src/file"));
    EXPECT_TRUE(this->fs.Exists("/dst/file"));
    EXPECT_EQ(this->fs.ReadFile("/dst/file").value_or(""), "data");
}

// перемещение файла в новый путь работает как переименование
TYPED_TEST(CrudTest, MoveFileWithRename) {
    ASSERT_TRUE(this->fs.MakeDir("/dir"));
    ASSERT_TRUE(this->fs.WriteToFile("/dir/old", "data"));

    EXPECT_TRUE(this->fs.Move("/dir/old", "/dir/new"));

    EXPECT_FALSE(this->fs.Exists("/dir/old"));
    EXPECT_TRUE(this->fs.Exists("/dir/new"));
    EXPECT_EQ(this->fs.ReadFile("/dir/new").value_or(""), "data");
}

// перемещение директории переносит всё поддерево
TYPED_TEST(CrudTest, MoveDirectoryMovesSubtree) {
    ASSERT_TRUE(this->fs.MakeDir("/x"));
    ASSERT_TRUE(this->fs.MakeDir("/x/y"));
    ASSERT_TRUE(this->fs.WriteToFile("/x/y/file", "content"));
    ASSERT_TRUE(this->fs.MakeDir("/z"));

    EXPECT_TRUE(this->fs.Move("/x", "/z/x"));

    EXPECT_FALSE(this->fs.Exists("/x"));
    EXPECT_FALSE(this->fs.Exists("/x/y/file"));
    EXPECT_TRUE(this->fs.Exists("/z/x"));
    EXPECT_TRUE(this->fs.Exists("/z/x/y"));
    EXPECT_TRUE(this->fs.Exists("/z/x/y/file"));
    EXPECT_EQ(this->fs.ReadFile("/z/x/y/file").value_or(""), "content");
}

// перемещение несуществующего источника невозможно
TYPED_TEST(CrudTest, MoveMissingSourceFails) {
    EXPECT_FALSE(this->fs.Move("/missing", "/dst"));
}

// перемещение при конфликте имён в назначении запрещено
TYPED_TEST(CrudTest, MoveCollisionFails) {
    ASSERT_TRUE(this->fs.MakeDir("/d1"));
    ASSERT_TRUE(this->fs.MakeDir("/d2"));
    ASSERT_TRUE(this->fs.WriteToFile("/d1/file", "a"));
    ASSERT_TRUE(this->fs.WriteToFile("/d2/file", "b"));

    EXPECT_FALSE(this->fs.Move("/d1/file", "/d2"));
    EXPECT_TRUE(this->fs.Exists("/d1/file"));
    EXPECT_EQ(this->fs.ReadFile("/d1/file").value_or(""), "a");
    EXPECT_EQ(this->fs.ReadFile("/d2/file").value_or(""), "b");
}

// нельзя переместить директорию внутрь самой себя
TYPED_TEST(CrudTest, MoveIntoOwnSubtreeFails) {
    ASSERT_TRUE(this->fs.MakeDir("/p"));
    ASSERT_TRUE(this->fs.MakeDir("/p/q"));

    EXPECT_FALSE(this->fs.Move("/p", "/p/q"));

    EXPECT_TRUE(this->fs.Exists("/p"));
    EXPECT_TRUE(this->fs.Exists("/p/q"));
}

// перемещение в несуществующую родительскую директорию запрещено
TYPED_TEST(CrudTest, MoveToMissingParentFails) {
    ASSERT_TRUE(this->fs.MakeDir("/src"));
    ASSERT_TRUE(this->fs.WriteToFile("/src/file", "x"));

    EXPECT_FALSE(this->fs.Move("/src/file", "/missing/file"));
    EXPECT_TRUE(this->fs.Exists("/src/file"));
}


// поиск по шаблону имени в поддереве
TYPED_TEST(CrudTest, FindByPattern) {
    ASSERT_TRUE(this->fs.MakeDir("/repo"));
    ASSERT_TRUE(this->fs.MakeDir("/repo/src"));
    ASSERT_TRUE(this->fs.WriteToFile("/repo/src/main.cpp", "x"));
    ASSERT_TRUE(this->fs.WriteToFile("/repo/src/util.cpp", "y"));
    ASSERT_TRUE(this->fs.WriteToFile("/repo/readme.md", "z"));

    std::vector<std::string> expected = {"/repo/src/main.cpp", "/repo/src/util.cpp"};
    EXPECT_EQ(Sorted(this->fs.Find("/repo", "*.cpp")), Sorted(expected));
}

// поиск с шаблоном "*" возвращает всё поддерево, включая стартовый узел
TYPED_TEST(CrudTest, FindAllInSubtree) {
    ASSERT_TRUE(this->fs.MakeDir("/repo"));
    ASSERT_TRUE(this->fs.MakeDir("/repo/src"));
    ASSERT_TRUE(this->fs.WriteToFile("/repo/src/main.cpp", "x"));
    ASSERT_TRUE(this->fs.WriteToFile("/repo/src/util.cpp", "y"));

    std::vector<std::string> expected = {"/repo/src", "/repo/src/main.cpp", "/repo/src/util.cpp"};
    EXPECT_EQ(Sorted(this->fs.Find("/repo/src", "*")), Sorted(expected));
}

// поиск без совпадений возвращает пустой список
TYPED_TEST(CrudTest, FindNoMatches) {
    ASSERT_TRUE(this->fs.MakeDir("/repo"));
    ASSERT_TRUE(this->fs.WriteToFile("/repo/file.txt", "x"));

    EXPECT_TRUE(this->fs.Find("/repo", "*.cpp").empty());
}

// поиск от несуществующего пути возвращает пустой список
TYPED_TEST(CrudTest, FindFromMissingPathReturnsEmpty) {
    EXPECT_TRUE(this->fs.Find("/missing", "*").empty());
}
