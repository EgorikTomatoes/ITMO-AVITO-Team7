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

void FlatFileSystem::FindIterative(const FsNode* current_node, const std::string& pattern, std::vector<std::string>& matches) const {
    std::string prefix = (utils::PathUtils::IsRootPath(current_node->full_path)) ? "/" : current_node->full_path + "/";
    for (const auto& [path, node]: path_index_) {
        if ((path == current_node->full_path || path.starts_with(current_node->full_path)) && 
            utils::PathUtils::MatchPattern(node->name,pattern)) 
        {   
            matches.push_back(std::string(path));
        }
    }
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
    if (!utils::PathUtils::IsValidPath(path)) return {};
    if (FsNode* dir_node = GetDirectory(path)) {
        std::vector<std::string> answer;
        for (const auto& [node_path, node]: path_index_) {
            if (auto parent_path_opt = utils::PathUtils::GetParentPath(node_path); 
                parent_path_opt.has_value() && parent_path_opt.value() == path) 
            {
                answer.push_back(node->name);
            }
        }

        return answer;
    }

    return {};
}

bool FlatFileSystem::Move(const std::string& src, const std::string& dest) {
    if (!utils::PathUtils::IsValidPath(src) || !utils::PathUtils::IsValidPath(dest) || utils::PathUtils::IsRootPath(src)) return false;

    if (FsNode* starting_node = GetNode(src)) {
        std::string new_parent_path;
        std::string new_name;
        // Перемещение внутрь существующей директории
        if (GetDirectory(dest)) {
            new_parent_path = dest;
            new_name = starting_node->name;
        }
        // Перемещение в новое место с переименованием
        else {
            auto parent_opt = utils::PathUtils::GetParentPath(dest);
            if (!parent_opt.has_value() || !GetDirectory(parent_opt.value())) return false;
            new_parent_path = std::string(parent_opt.value());
            new_name = std::string(utils::PathUtils::GetFileName(dest).value_or(""));
        }

        std::string new_full_path = utils::PathUtils::Join(new_parent_path,new_name);
        if (GetNode(new_full_path)) return false;                        // Коллизия имён
        std::string src_prefix = src + "/";
        if (dest == src || dest.starts_with(src_prefix)) return false;   // Циклическая защита: нельзя переместить ноду в своё поддерево

        std::vector<FsNode*> nodes_to_move;             // 1) Cобирааем все узлы для перемещения
        for (const auto& [path, node]: path_index_) {
            if (path == src || path.starts_with(src_prefix)) nodes_to_move.push_back(node);
        }

        for (auto node_to_move: nodes_to_move) {        // 2) Удаляем старые ключи
            path_index_.erase(node_to_move->full_path);
        }

        for (auto node_to_move: nodes_to_move) {        // 3) Обновление пути (и имени если нужно)
            if (node_to_move == starting_node) {
                node_to_move->name = new_name;
                node_to_move->full_path = new_full_path;
            }
            else {
                node_to_move->full_path.replace(0,src.length(),new_full_path);
            }
            path_index_[node_to_move->full_path] = node_to_move;
        }

        return true;
    }

    return false;
}

std::vector<std::string> FlatFileSystem::Find(const std::string& path, std::string pattern) const {
    if (!utils::PathUtils::IsValidPath(path)) return {};

    if (FsNode* start_node = GetNode(path)) {
        std::vector<std::string> matches;
        FindIterative(start_node,pattern,matches);
        return matches;
    }

    return {};
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