#include "case_a.hpp"
#include <execution>
#include <memory>
#include <type_traits>

std::optional<std::string> TreeFileSystem::ReadFile(const std::string& path) const {
    Node* node = FindNode(path);

    if (!node || node->type != NodeType::File) return std::nullopt;
    return node->data;
}

bool TreeFileSystem::WriteToFile(const std::string& path, const std::string& data){
    if (path.empty() || path == "/") return false;

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
    if (path.empty()) return nullptr;
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
