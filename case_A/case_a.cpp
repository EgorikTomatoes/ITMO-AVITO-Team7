#include "case_a.hpp"
#include <memory>
#include <optional>
#include <vector>

std::optional<std::string> TreeFileSystem::ReadFile(const std::string& path) const {
    Node* node = FindNode(path);

    if (!node || node->type != NodeType::File) return std::nullopt;
    return node->data;
}

bool TreeFileSystem::WriteToFile(const std::string& path, const std::string& data){
    if (path.empty() || path == "/" || path[0] != '/') return false;

    Node* node = FindNode(path);
    
    if (node){
        if (node->type != NodeType::File) return false;

        total_file_bytes_ -= node->data.size();
        node->data = data;
        total_file_bytes_ += node->data.size();
        return true;
    }

    Node* parent = GetParentDir(path);
    if (!parent || parent->type != NodeType::Dir) return false;

    std::vector<std::string> names = Split(path);
    if (names.empty()) return false;
    std::string name = names.back();

    auto new_node = std::make_unique<Node>(name, NodeType::File, parent);
    new_node->data = data;

    total_file_bytes_ += data.size();
    file_count_++;
    parent->children.push_back(std::move(new_node));
    
    return true;
}

bool TreeFileSystem::MakeDir(const std::string& path){
    if (path.empty() || path == "/" || path[0] != '/') return false;
    if (FindNode(path)) return false;

    Node* parent = GetParentDir(path);
    if (!parent || parent->type != NodeType::Dir) return false;

    std::vector<std::string> names = Split(path);
    if (names.empty()) return false;
    std::string new_name = names.back();

    auto new_node = std::make_unique<Node>(new_name, NodeType::Dir, parent);
    parent->children.push_back(std::move(new_node));
    dir_count_++;

    return true;
}

std::vector<std::string> TreeFileSystem::List(const std::string& path) const {
    Node* node = FindNode(path);
    if (!node || node->type != NodeType::Dir) return {};

    std::vector<std::string> ans;
    for (auto& child : node->children){
        ans.push_back(child->name);
    }
    return ans;
}

bool TreeFileSystem::Move(const std::string& src, const std::string& dest){
    if (src.empty() || dest.empty()) return false;
    if (src == "/" || dest == "/") return false;

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
        new_parent = GetParentDir(dest);
        std::vector<std::string> names = Split(dest);

        if (!new_parent || new_parent->type != NodeType::Dir || names.empty()) return false;
        new_name = names.back();
    }

    if (GetChild(new_parent, new_name)) return false;

    Node* cur = new_parent;
    while (cur){
        if (cur == node) return false; // проверка на перемещение в себя
        cur = cur->parent;
    }

    std::unique_ptr<Node> moving;

    for (auto it = old_parent->children.begin(); it != old_parent->children.end(); ++it){
        if (it->get() == node){
            moving = std::move(*it);
            old_parent->children.erase(it);
            break;
        }
    }

    if (!moving) return false;
    moving->name = new_name;
    moving->parent = new_parent;
    new_parent->children.push_back(std::move(moving));
    return true;
}


// TODO: find with regex

std::optional<NodeInfo> TreeFileSystem::NodeState(const std::string& path) const {
    if (path.empty() || path[0] != '/') return std::nullopt;

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
    info.total_size_bytes = GetSubtreeMemory(root_.get());
    info.file_count = file_count_;
    info.dir_count = dir_count_;
    info.node_count = file_count_ + dir_count_;
    return info;
}

bool TreeFileSystem::Exists(const std::string& path) const {
    return FindNode(path) != nullptr;
}

void TreeFileSystem::Clear(){
    root_ = std::make_unique<Node>("/", NodeType::Dir, nullptr);

    total_file_bytes_ = 0;
    file_count_ = 0;
    dir_count_ = 1;
}



std::vector<std::string> TreeFileSystem::Split(const std::string& str, char delimeter) const {
    std::vector<std::string> res;
    std::string cur;

    for (char ch : str){
        if (ch == delimeter){
            if (!cur.empty()){
                res.push_back(cur);
                cur.clear();
            }
        } else {
            cur += ch;
        }
    }

    if (!cur.empty()){
        res.push_back(cur);
    }

    return res;
}

TreeFileSystem::Node* TreeFileSystem::GetChild(Node* cur, const std::string& name) const{
    if (!cur || cur->type != NodeType::Dir) return nullptr;

    for (auto& child : cur->children){
        if (child->name == name){
            return child.get();
        }
    }

    return nullptr;
}

TreeFileSystem::Node* TreeFileSystem::FindNode(const std::string& path) const{
    if (path.empty() || path[0] != '/') return nullptr;
    if (path == "/") return root_.get();

    Node* cur = root_.get();
    std::vector<std::string> names = Split(path);

    for (auto& name : names){
        cur = GetChild(cur, name);
        if (!cur) return nullptr;
    }
    return cur;
}

TreeFileSystem::Node* TreeFileSystem::GetParentDir(const std::string& path){
    std::vector<std::string> names = Split(path);
    if (names.empty()) return nullptr;

    Node* cur = root_.get();

    for (size_t i = 0; i + 1 < names.size(); ++i){
        cur = GetChild(cur, names[i]);
        if (!cur || cur->type != NodeType::Dir) return nullptr;
    }
    return cur;
}

size_t TreeFileSystem::GetNodeMemory(const Node* node) const {
    if (!node) return 0;
    return sizeof(Node) 
            + node->name.capacity() 
            + node->children.capacity() * sizeof(std::unique_ptr<Node>);
}

size_t TreeFileSystem::GetSubtreeMemory(const Node* node) const {
    if (!node) return 0;
    size_t ans = GetNodeMemory(node);

    if (node->type == NodeType::File){
        ans += node->data.capacity();
    }
    for (auto& child : node->children){
        ans += GetSubtreeMemory(child.get());
    }

    return ans;
}