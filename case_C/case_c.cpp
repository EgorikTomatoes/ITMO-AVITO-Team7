#include "case_c.hpp"
#include <algorithm>


FlatFileSystem::FlatFileSystem() {
    InitRoot();
}

FlatFileSystem::~FlatFileSystem() {
    for (auto& [path, node]: path_index_) {
        node_pool_.Deallocate(node);
    }
}

// Приватные методы

void FlatFileSystem::InitRoot() {
    FsNode* root = node_pool_.Allocate(NodeType::Dir, "/", "/");
    path_index_[root->full_path] = root;
    dir_count_ = 1;
}

FlatFileSystem::FsNode* FlatFileSystem::GetNode(std::string_view path) const {
    auto it = path_index_.find(path);
    return it != path_index_.end() ? it->second : nullptr;
}

FlatFileSystem::FsNode* FlatFileSystem::GetFile(std::string_view path) const {
    FsNode* node = GetNode(path);
    return (node != nullptr && node->type == NodeType::File) ? node : nullptr;
}

FlatFileSystem::FsNode* FlatFileSystem::GetDirectory(std::string_view path) const {
    FsNode* node = GetNode(path);
    return (node != nullptr && node->type == NodeType::Dir) ? node : nullptr;
}

FlatFileSystem::FsNode* FlatFileSystem::GetParentDirectory(std::string_view path) const {
    auto parent_path_opt = utils::PathUtils::GetParentPath(path);
    if (!parent_path_opt.has_value()) return nullptr;
    return GetDirectory(parent_path_opt.value());
}

FlatFileSystem::FsNode* FlatFileSystem::CreateNewNode(NodeType node_type, std::string full_path) {
    std::string name(utils::PathUtils::GetFileName(full_path).value_or(""));
    FsNode* new_node = node_pool_.Allocate(node_type, std::move(full_path), std::move(name));
    path_index_[new_node->full_path] = new_node;

    return new_node;
}

size_t FlatFileSystem::GetNodeNotDataMemory(const FsNode* node) const {
    if (!node) return 0;
    return sizeof(FsNode) 
        + node->name.capacity()
        + node->full_path.capacity();
}

// API файловой системы


std::optional<std::string> FlatFileSystem::ReadFile(const std::string& path) const {
    if (!utils::PathUtils::IsValidPath(path)) return std::nullopt;
    
    FsNode* node = GetFile(path);
    if (!node) return std::nullopt;

    return node->data;
}


bool FlatFileSystem::WriteToFile(const std::string& path, const std::string& data) {
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
        FsNode* new_file = CreateNewNode(NodeType::File, path);
        new_file->data = data;
        total_file_bytes_ += data.size();
        ++file_count_;
        return true;
    }

    return false;
}

bool FlatFileSystem::MakeDir(const std::string& path) {
    if (!utils::PathUtils::IsValidPath(path) || utils::PathUtils::IsRootPath(path) || GetNode(path)) return false;

    if (GetParentDirectory(path)) {
        CreateNewNode(NodeType::Dir, path);
        ++dir_count_;
        return true;
    }

    return false;
}

std::vector<std::string> FlatFileSystem::List(const std::string& path) const {

}

bool FlatFileSystem::Move(const std::string& src, const std::string& dest) {

}

std::vector<std::string> FlatFileSystem::Find(const std::string& path, std::string pattern) const {

}

bool FlatFileSystem::Exists(const std::string& path) const {
    return utils::PathUtils::IsValidPath(path) && GetNode(path) != nullptr;
}

std::optional<NodeInfo> FlatFileSystem::NodeState(const std::string& path) const {
    if (!utils::PathUtils::IsValidPath(path)) return std::nullopt;
    if (FsNode* node = GetNode(path)) {
        NodeInfo info = {.path = node->full_path, .type = node->type, .size_bytes = GetNodeNotDataMemory(node)};
        return info;
    }   

    return std::nullopt;
}
std::optional<SystemInfo> FlatFileSystem::SystemState() const {
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
    }

    // 3) Эвристическтй рассчёт оверхеда хэш-таблицы
    size_t map_overhead = (path_index_.bucket_count() * sizeof(void*))                           // массив бакетов
        + (path_index_.size() * (sizeof(void*) + sizeof(std::pair<std::string_view, FsNode*>))); // активные узлы
    info.total_size_bytes = total_file_bytes_ + implementation_memory + map_overhead;
    return info;
}

void FlatFileSystem::Clear() {
    for (auto& [path, node]: path_index_) {
        node_pool_.Deallocate(node);
    }
    path_index_.clear();

    total_file_bytes_ = 0;
    file_count_ = 0;
    dir_count_ = 0;

    InitRoot();
}