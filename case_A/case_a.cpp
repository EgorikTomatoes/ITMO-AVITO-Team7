#include "case_a.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>
#include <regex>

using pu = utils::PathUtils;

TreeFileSystem::TreeFileSystem() {
    root_ = node_pool_.Allocate("/", NodeType::Dir, nullptr);
}

TreeFileSystem::~TreeFileSystem(){
    DeleteTree(root_);
}

void TreeFileSystem::DeleteTree(Node* node){
    if (!node) return;
    for (Node* child : node->children){
        DeleteTree(child);
    }
    node_pool_.Deallocate(node);
}

std::optional<std::string> TreeFileSystem::ReadFile(const std::string& path) const {
    if (!pu::IsValidPath(path)) return std::nullopt;
    Node* node = FindNode(path);

    if (!node || node->type != NodeType::File) return std::nullopt;
    return node->data;
}

bool TreeFileSystem::WriteToFile(const std::string& path, const std::string& data){
    if (!pu::IsValidPath(path) || pu::IsRootPath(path)) return false;

    Node* node = FindNode(path);
    
    if (node){
        if (node->type != NodeType::File) return false;

        total_file_bytes_ -= node->data.size();
        node->data = data;
        total_file_bytes_ += node->data.size();
        return true;
    }

    auto parent_path = pu::GetParentPath(path);
    if (!parent_path) return false;

    Node* parent = FindNode(*parent_path);
    if (!parent || parent->type != NodeType::Dir) return false;

    auto name = pu::GetFileName(path);
    if (!name) return false;

    Node* new_node = node_pool_.Allocate(std::string(*name), NodeType::File, parent);
    new_node->data = data;

    total_file_bytes_ += data.size();
    file_count_++;
    parent->children.push_back(new_node);
    
    return true;
}

bool TreeFileSystem::MakeDir(const std::string& path){
    if (!pu::IsValidPath(path) || pu::IsRootPath(path)) return false;
    if (FindNode(path)) return false;

    auto parent_path = pu::GetParentPath(path);
    if (!parent_path) return false;

    Node* parent = FindNode(*parent_path);
    if (!parent || parent->type != NodeType::Dir) return false;

    auto new_name = pu::GetFileName(path);
    if (!new_name) return false;

    Node* new_node = node_pool_.Allocate(std::string(*new_name), NodeType::Dir, parent);
    parent->children.push_back(new_node);
    dir_count_++;

    return true;
}

std::vector<std::string> TreeFileSystem::List(const std::string& path) const {
    if (!pu::IsValidPath(path)) return {};

    Node* node = FindNode(path);
    if (!node || node->type != NodeType::Dir) return {};

    std::vector<std::string> ans;
    ans.reserve(node->children.size());
    for (auto& child : node->children){
        ans.push_back(child->name);
    }
    return ans;
}

bool TreeFileSystem::Move(const std::string& src, const std::string& dest){
    if (!pu::IsValidPath(src) || !pu::IsValidPath(dest)) return false;
    if (pu::IsRootPath(src) || pu::IsRootPath(dest)) return false;

    Node* node = FindNode(src);
    if (!node) return false;

    Node* old_parent = node->parent;
    if (!old_parent) return false;

    Node* new_parent = nullptr;
    std::string new_name;

    Node* dest_node = FindNode(dest);

    if (dest_node){
        if (dest_node->type != NodeType::Dir){
            return false;
        }
        // добавляем в существующую папку
        new_parent = dest_node;
        new_name = node->name;
    }
    else {
        // новая папка, возможно новое имя файла
        auto parent_path = pu::GetParentPath(dest);
        auto name = pu::GetFileName(dest);
        if (!parent_path || !name) return false;

        new_parent = FindNode(*parent_path);
        if (!new_parent || new_parent->type != NodeType::Dir) return false;
        new_name = *name;
    }

    if (GetChild(new_parent, new_name)) return false;

    Node* cur = new_parent;
    while (cur){
        if (cur == node) return false; // проверка на перемещение в себя
        cur = cur->parent;
    }

    for (auto it = old_parent->children.begin(); it != old_parent->children.end(); ++it){
        if (*it== node){
            old_parent->children.erase(it);
            break;
        }
    }

    node->name = new_name;
    node->parent = new_parent;
    new_parent->children.push_back(node);
    return true;
}

std::vector<std::string> TreeFileSystem::Find(const std::string& path, std::string pattern) const {
    if (!pu::IsValidPath(path)) return {};

    Node* node = FindNode(path);
    if (!node) return {};

    std::vector<std::string> ans;
    FindPattern(node, path, pattern, ans);
    return ans;
}


std::optional<NodeInfo> TreeFileSystem::NodeState(const std::string& path) const {
    if (!pu::IsValidPath(path)) return std::nullopt;

    Node* node = FindNode(path);
    if (!node) return std::nullopt;

    NodeInfo info;
    info.path = path;
    info.type = node->type;
    info.size_bytes = GetNodeMemory(node);
    return info;
}

std::optional<SystemInfo> TreeFileSystem::SystemState() const {
    SystemInfo info;
    info.total_file_bytes = total_file_bytes_;
    info.total_size_bytes = node_pool_.GetTotalAllocatedBytes() + total_file_bytes_;
    info.file_count = file_count_;
    info.dir_count = dir_count_;
    info.node_count = file_count_ + dir_count_;
    return info;
}

bool TreeFileSystem::Exists(const std::string& path) const {
    return FindNode(path) != nullptr;
}

void TreeFileSystem::Clear(){
    DeleteTree(root_);

    root_ = node_pool_.Allocate("/", NodeType::Dir, nullptr);
    total_file_bytes_ = 0;
    file_count_ = 0;
    dir_count_ = 1;
}



TreeFileSystem::Node* TreeFileSystem::GetChild(Node* cur, std::string_view name) const{
    if (!cur || cur->type != NodeType::Dir) return nullptr;

    for (auto& child : cur->children){
        if (child->name == name){
            return child;
        }
    }

    return nullptr;
}

TreeFileSystem::Node* TreeFileSystem::FindNode(std::string_view path) const{
    if (!pu::IsValidPath(path)) return nullptr;
    if (pu::IsRootPath(path)) return root_;

    Node* cur = root_;
    auto names = pu::Split(path);
    if (!names) return nullptr;

    for (auto name : *names){
        cur = GetChild(cur, name);
        if (!cur) return nullptr;
    }
    return cur;
}

size_t TreeFileSystem::GetNodeMemory(const Node* node) const {
    if (!node) return 0;
    return sizeof(Node) 
            + node->name.capacity() 
            + node->children.capacity() * sizeof(Node*);
}

size_t TreeFileSystem::GetSubtreeMemory(const Node* node) const {
    if (!node) return 0;
    size_t ans = GetNodeMemory(node);

    if (node->type == NodeType::File){
        ans += node->data.capacity();
    }
    for (auto& child : node->children){
        ans += GetSubtreeMemory(child);
    }

    return ans;
}

void TreeFileSystem::FindPattern(const Node* node, const std::string& path, std::string_view pattern, 
                                std::vector<std::string>& res) const{
    if (!node) return;
    
    if (!utils::PathUtils::IsRootPath(path) && utils::PathUtils::MatchPattern(node->name, pattern)){
        res.push_back(path);
    }
    if (node->type != NodeType::Dir) return;

    for (auto& child : node->children){
        std::string child_path = pu::Join(path, child->name);
        FindPattern(child, child_path, pattern, res);
    }
}
