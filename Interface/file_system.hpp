#pragma once

#include <string>
#include <optional>
#include <vector>

enum class NodeType{
    File,
    Dir
};

struct NodeInfo{
    std::string path;
    NodeType type;
    size_t size_bytes;
};

struct SystemInfo{
    size_t total_file_bytes;
    size_t total_size_bytes; // с учетом памяти реализации

    size_t file_count;
    size_t dir_count;
    size_t node_count;
};

class IFileSystem {
public:
    virtual ~IFileSystem() = default;

    virtual std::optional<std::string> ReadFile(const std::string& path) const = 0;
    virtual bool WriteToFile(const std::string& path, const std::string& data) = 0;

    virtual bool MakeDir(const std::string& path) = 0;

    virtual std::vector<std::string> List(const std::string& path) const = 0;

    virtual bool Move(const std::string& src, const std::string& dest) = 0;

    virtual std::vector<std::string> Find(const std::string& path, const std::string& pattern) const = 0;

    virtual bool Exists(const std::string& path) const = 0;

    virtual std::optional<NodeInfo> NodeState(const std::string& path) const = 0;
    virtual std::optional<SystemInfo> SystemState() const = 0; 
    virtual void Clear() = 0;
};