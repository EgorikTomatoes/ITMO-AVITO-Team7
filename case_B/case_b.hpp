#pragma once

#include <Interface/file_system.hpp>
#include <utils/memory_pool.hpp>
#include <utils/path_utils.hpp>

#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <unordered_map> // может быть придётся заменить для скорости на не STL-ую реализацию
#include <utility>

class TreeHashFileSystem: public IFileSystem {
public:
    TreeHashFileSystem();
    ~TreeHashFileSystem() override;

    std::optional<std::string> ReadFile(const std::string& path) const override;
    bool WriteToFile(const std::string& path, const std::string& data) override;
    bool MakeDir(const std::string& path) override;
    std::vector<std::string> List(const std::string& path) const override;
    bool Move(const std::string& src, const std::string& dest) override;
    std::vector<std::string> Find(const std::string& path, std::string pattern = "*") const override;
    bool Exists(const std::string& path) const override;

    std::optional<NodeInfo> NodeState(const std::string& path) const override;
    std::optional<SystemInfo> SystemState() const override; 
    void Clear() override;

private:

    struct FsNode {
        NodeType type;
        std::string full_path;          // абсолютный путь до файла или директории
        std::string name;               // название файла или директории

        std::string data;               // Содержимое файла, если это файл

        std::vector<FsNode*> children;  // сырые указатели, так как памятью управляет pool
        FsNode* parent = nullptr;       // родитель в дереве файловой системы

        FsNode(NodeType node_type, std::string node_full_path, std::string node_name, FsNode* node_parent)
            : type(node_type), full_path(std::move(node_full_path)), name(std::move(node_name)), parent(node_parent)
        {}
    };

    utils::MemoryPool<FsNode> node_pool_;
    std::unordered_map<std::string_view, FsNode*> path_index_;
    FsNode* root_ = nullptr;

    size_t total_file_bytes = 0;
    size_t file_count = 0;
    size_t dir_count = 0;

    void InitRoot();
    void DestroyTree(FsNode* node);
    void UpdatePathsRecursive(FsNode* node, std::string_view new_parent_path);
};

typedef TreeHashFileSystem Case_B;