#include "case_b.hpp"
#include <algorithm>
#include <regex>

TreeHashFileSystem::TreeHashFileSystem() {
    InitRoot();
}

TreeHashFileSystem::~TreeHashFileSystem() {
    DestroyTree(root_);
}

// Приватные методы

void TreeHashFileSystem::InitRoot() {
    root_ = node_pool_.Allocate(NodeType::Dir, "/", "/", nullptr);
    path_index_[root_->full_path] = root_;
    dir_count_ = 1;
}

void TreeHashFileSystem::DestroyTree(FsNode* node) {
    if (!node) return;
    for (FsNode* child : node->children) {
        DestroyTree(child);
    }
    node_pool_.Deallocate(node); 
}


TreeHashFileSystem::FsNode* TreeHashFileSystem::GetNode(std::string_view path) const {
    auto it = path_index_.find(path);
    return it != path_index_.end() ? it->second : nullptr;
}

TreeHashFileSystem::FsNode* TreeHashFileSystem::GetFile(std::string_view path) const {
    FsNode* node = GetNode(path);
    return (node != nullptr && node->type == NodeType::File) ? node : nullptr;
}

TreeHashFileSystem::FsNode* TreeHashFileSystem::GetDirectory(std::string_view path) const {
    FsNode* node = GetNode(path);
    return (node != nullptr && node->type == NodeType::Dir) ? node : nullptr;
}

TreeHashFileSystem::FsNode* TreeHashFileSystem::GetParentDirectory(std::string_view path) const {
    auto parent_path_opt = utils::PathUtils::GetParentPath(path);
    if (!parent_path_opt.has_value()) return nullptr;
    return GetDirectory(parent_path_opt.value());
}


TreeHashFileSystem::FsNode* TreeHashFileSystem::CreateNewNode(NodeType node_type, std::string full_path, FsNode* parent) {
    std::string name(utils::PathUtils::GetFileName(full_path).value_or(""));
    FsNode* new_node = node_pool_.Allocate(node_type, std::move(full_path), std::move(name), parent);
    path_index_[new_node->full_path] = new_node;
    if (parent) {
        parent->children.push_back(new_node);
    }
    return new_node;
}


void TreeHashFileSystem::UpdatePathsRecursive(FsNode* node, std::string_view new_parent_path) {
    path_index_.erase(node->full_path);
    node->full_path = utils::PathUtils::Join(new_parent_path, node->name);
    path_index_[node->full_path] = node;
    for (FsNode* child : node->children) {
        UpdatePathsRecursive(child, node->full_path);
    }
}

size_t TreeHashFileSystem::GetNodeNotDataMemory(const FsNode* node) const {
    if (!node) return 0;
    return sizeof(FsNode)
        + node->name.capacity()
        + node->full_path.capacity()
        + (node->children.capacity() * sizeof(FsNode*));
}

void TreeHashFileSystem::FindRecursive(FsNode* current_node, const std::string& pattern, std::vector<std::string>& matches) const {
    if (!current_node) return;
    if (utils::PathUtils::MatchPattern(current_node->name, pattern)) {
        matches.push_back(current_node->full_path);
    }
    
    for (auto child: current_node->children) {
        FindRecursive(child,pattern,matches);
    }
}


// API файловой системы

std::optional<std::string> TreeHashFileSystem::ReadFile(const std::string& path) const {
    if (!utils::PathUtils::IsValidPath(path)) return std::nullopt;

    FsNode* node = GetFile(path);
    if (!node) return std::nullopt;

    return node->data;
}

bool TreeHashFileSystem::WriteToFile(const std::string& path, const std::string& data) {
    if (!utils::PathUtils::IsValidPath(path) || utils::PathUtils::IsRootPath(path)) return false;

    // Файл существует в индексе путей
    if (FsNode* node = GetFile(path)) {                                  
        total_file_bytes_ -= node->data.size();
        node->data = data;
        total_file_bytes_ += node->data.size();
        return true;
    }

    // Файла не существует в индексе путей, но существует родитель
    if (FsNode* parent = GetParentDirectory(path); !GetNode(path) && parent) {
        FsNode* new_file = CreateNewNode(NodeType::File, path, parent);
        new_file->data = data;
        total_file_bytes_ += data.size();
        ++file_count_;
        return true;
    }

    return false;
}

bool TreeHashFileSystem::MakeDir(const std::string& path) {
    if (!utils::PathUtils::IsValidPath(path) || utils::PathUtils::IsRootPath(path) || GetNode(path)) return false;

    if (FsNode* parent = GetParentDirectory(path)) {
        CreateNewNode(NodeType::Dir, path, parent);
        ++dir_count_;
        return true;
    }

    return false;
}

std::vector<std::string> TreeHashFileSystem::List(const std::string& path) const {
    if (!utils::PathUtils::IsValidPath(path)) return {};

    if (FsNode* dir_node = GetDirectory(path)) {
        std::vector<std::string> answer;
        answer.reserve(dir_node->children.size());

        for (auto child: dir_node->children) {
            answer.push_back(child->name);
        }
        return answer;
    }
    return {};
}

bool TreeHashFileSystem::Move(const std::string& src, const std::string& dest) {
    if (!utils::PathUtils::IsValidPath(src) || !utils::PathUtils::IsValidPath(dest) || utils::PathUtils::IsRootPath(src)) return false;

    if (FsNode* moving_node = GetNode(src)) {
        FsNode* new_parent = nullptr;
        std::string new_name;
        // Перемещение внутрь существующей директории
        if (FsNode* dest_node = GetDirectory(dest)) {
            new_parent = dest_node;
            new_name = moving_node->name;
        }
        // Перемещение в новое место с переименованием
        else {
            new_parent = GetParentDirectory(dest);
            new_name = std::string(utils::PathUtils::GetFileName(dest).value());
        }

        if (!new_parent) return false;                  // Должен быть родитель
        for (auto child: new_parent->children) {
            if (child->name == new_name) return false;  // Не должно быть коллизий в именах
        }

        FsNode* current_ancestor = new_parent;          // Циклическая защита: нельзя переместить ноду в своё поддерево
        while (current_ancestor) {
            if (current_ancestor == moving_node) return false;
            current_ancestor = current_ancestor->parent;
        }

        FsNode* old_parent = moving_node->parent;          // Erase-Remove идиома для вектора
        auto& siblings = old_parent->children;
        siblings.erase(std::remove(siblings.begin(),siblings.end(), moving_node), siblings.end());

        moving_node->name = std::move(new_name);
        moving_node->parent = new_parent;
        new_parent->children.push_back(moving_node);
        UpdatePathsRecursive(moving_node, new_parent->full_path);
        
        return true;
    }

    return false;
}


std::vector<std::string> TreeHashFileSystem::Find(const std::string& path, std::string pattern) const {
    if (!utils::PathUtils::IsValidPath(path)) return {};

    if (FsNode* start_node = GetNode(path)) {
        std::vector<std::string> matches;
        FindRecursive(start_node, pattern, matches);
        return matches;
    }
    return {};
}

bool TreeHashFileSystem::Exists(const std::string& path) const {
    return utils::PathUtils::IsValidPath(path) && GetNode(path) != nullptr;
}

std::optional<NodeInfo> TreeHashFileSystem::NodeState(const std::string& path) const {
    if (!utils::PathUtils::IsValidPath(path)) return std::nullopt;
    if (FsNode* node = GetNode(path)) {
        NodeInfo info = {.path = node->full_path, .type = node->type, .size_bytes = GetNodeNotDataMemory(node)};
        return info;    
    }

    return std::nullopt;
}


std::optional<SystemInfo> TreeHashFileSystem::SystemState() const {
    SystemInfo info;
    info.file_count = file_count_;
    info.dir_count = dir_count_;
    info.node_count = node_pool_.GetActiveObjectsCount();

    info.total_file_bytes = total_file_bytes_;

    size_t implementation_memory = node_pool_.GetTotalAllocatedBytes();  // 1)Память на сами структуры нод (активные + свободные)

    // 2) Динамическая память строк, векторов и т.д. в активных нодах
    for (const auto& [path, node]: path_index_) {
        implementation_memory += node->name.capacity();
        implementation_memory += node->full_path.capacity();
        implementation_memory += node->children.capacity() * sizeof(FsNode*);
    }

    // 3) Эвристическтй рассчёт оверхеда хэш-таблицы
    size_t map_overhead = (path_index_.bucket_count() * sizeof(void*))                           // массив бакетов
        + (path_index_.size() * (sizeof(void*) + sizeof(std::pair<std::string_view, FsNode*>))); // активные узлы
    
    info.total_size_bytes = total_file_bytes_ + implementation_memory + map_overhead;
    return info;
}


void TreeHashFileSystem::Clear() {
    path_index_.clear();
    DestroyTree(root_);
    
    total_file_bytes_ = 0;
    file_count_ = 0;
    dir_count_ = 0;
    
    InitRoot(); 
}
