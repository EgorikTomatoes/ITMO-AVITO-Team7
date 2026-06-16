#pragma once

#include "../Interface/file_system.hpp"
#include "../utils/memory_pool.hpp"
#include "../utils/path_utils.hpp"

#include <memory>
#include <string>
#include <optional>
#include <string_view>
#include <vector>
#include <utility>
#include <regex>


class TreeFileSystem : public IFileSystem {
public:
    TreeFileSystem();
    ~TreeFileSystem() override;

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

    struct Node{
        NodeType type;
        std::string name;
        
        std::string data; // если File

        std::vector<Node*> children; // если Dir
        Node* parent = nullptr;

        Node(std::string node_name, NodeType node_type, Node* node_parent) 
            : name(std::move(node_name)), type(node_type), parent(node_parent) {}
    };

    utils::MemoryPool<Node> node_pool_;
    Node* root_ = nullptr;

    size_t total_file_bytes_ = 0;
    size_t file_count_ = 0;
    size_t dir_count_ = 1;

    void DeleteTree(Node* node);

    Node* GetChild(Node* cur, std::string_view name) const;
    Node* FindNode(std::string_view path) const;

    size_t GetNodeMemory(const Node* node) const;
    size_t GetSubtreeMemory(const Node* node) const;

    void FindPattern(const Node* node, const std::string& path, std::string_view pattern, std::vector<std::string>& res) const;
};

typedef TreeFileSystem Case_A;