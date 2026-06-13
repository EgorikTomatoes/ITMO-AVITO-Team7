#pragma once

#include <Interface/file_system.hpp>
#include <utils/memory_pool.hpp>
#include <utils/path_utils.hpp>

#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <unordered_map>
#include <utility>
#include <cstddef>

class FlatFileSystem: public IFileSystem {
public:
    FlatFileSystem();
    ~FlatFileSystem() override;

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
        NodeType type;              // тип ноды
        std::string full_path;      // абсолютный путь до файла или директории
        std::string name;           // название файла или директории
        std::string data;           // cодержимое файла, если это файл

        FsNode(NodeType node_type, std::string node_full_path, std::string node_name)
            : type(node_type), full_path(std::move(node_full_path)), name(std::move(node_name))
        {}
    };

    utils::MemoryPool<FsNode> node_pool_;
    std::unordered_map<std::string_view, FsNode*> path_index_;

    size_t total_file_bytes_ = 0;
    size_t dir_count_ = 0;
    size_t file_count_ = 0;

    void InitRoot();                                            // Инициализация файловой системы (инваритное состояние)

    FsNode* GetNode(std::string_view path) const;               // Ищет ноду по абсолютному пути
    FsNode* GetFile(std::string_view path) const;               // Ищет ноду по абсолютному пути + проверка на то, что это файл
    FsNode* GetDirectory(std::string_view path) const;          // Ищет ноду по абсолютному пути + проверка на то, что это директория
    FsNode* GetParentDirectory(std::string_view path) const;    // Ищет родительскую ноду

    FsNode* CreateNewNode(NodeType node_type, std::string full_path);
    size_t GetNodeNotDataMemory(const FsNode* node) const;
};

typedef FlatFileSystem Case_C;