#include <case_b.hpp>
#include <algorithm>

TreeHashFileSystem::TreeHashFileSystem() {
    InitRoot();
}

TreeHashFileSystem::~TreeHashFileSystem() {
    Clear();
}

void TreeHashFileSystem::InitRoot() {
    root_ = node_pool_.Allocate(NodeType::Dir, "/", "/", nullptr);
    path_index_[root_->full_path] = root_;
    dir_count = 1;
}

void TreeHashFileSystem::DestroyTree(FsNode* node) {
    if (!node) return;
    for (FsNode* child : node->children) {
        DestroyTree(child);
    }
    node_pool_.Deallocate(node); 
}

void TreeHashFileSystem::Clear() {
    path_index_.clear();
    DestroyTree(root_);
    
    total_file_bytes = 0;
    file_count = 0;
    dir_count = 0;
    
    InitRoot(); 
}

void TreeHashFileSystem::UpdatePathsRecursive(FsNode* node, std::string_view new_parent_path) {
    path_index_.erase(node->full_path);
    node->full_path = utils::PathUtils::Join(new_parent_path, node->name);
    path_index_[node->full_path] = node;
    for (FsNode* child : node->children) {
        UpdatePathsRecursive(child, node->full_path);
    }
}


bool TreeHashFileSystem::Exists(const std::string& path) const {
    return path_index_.find(path) != path_index_.end();
}

std::optional<std::string> TreeHashFileSystem::ReadFile(const std::string& path) const {
    if (!utils::PathUtils::IsValidPath(path)) return std::nullopt;

    auto it = path_index_.find(path);
    if (it == path_index_.end()) return std::nullopt;

    auto fs_node_ptr = it->second;
    if (fs_node_ptr->type != NodeType::File) return std::nullopt;
    return fs_node_ptr->data;
}

bool TreeHashFileSystem::WriteToFile(const std::string& path, const std::string& data) {
    if (!utils::PathUtils::IsValidPath(path) || utils::PathUtils::IsRootPath(path)) return false;

    // Файл существует в индексе путей
    if (auto it = path_index_.find(path); it != path_index_.end()) {                                  
        FsNode* fs_node_ptr = it->second;
        if (fs_node_ptr->type != NodeType::File) return false;

        total_file_bytes -= fs_node_ptr->data.size();
        fs_node_ptr->data = data;
        total_file_bytes += fs_node_ptr->data.size();
        return true;
    }

    // Файла не существует в индексе путей   
    if (auto parent_path = utils::PathUtils::GetParentPath(path); parent_path.has_value())  {
        auto parent_it = path_index_.find(parent_path.value());
        if (parent_it == path_index_.end()) return false;
        FsNode* parent_fs_node_ptr = parent_it->second;
        if (parent_fs_node_ptr->type != NodeType::Dir) return false;

        std::string file_name(utils::PathUtils::GetFileName(path).value());
        FsNode* new_node = node_pool_.Allocate(NodeType::File, path, std::move(file_name), parent_fs_node_ptr);

        new_node->data = data;
        parent_fs_node_ptr->children.push_back(new_node);
        total_file_bytes += data.size();
        path_index_[new_node->full_path] = new_node;
        ++file_count;

        return true;
    }  
    
    return false;
}

bool TreeHashFileSystem::MakeDir(const std::string& path) {
    if (!utils::PathUtils::IsValidPath(path) || utils::PathUtils::IsRootPath(path)) return false;
    if (path_index_.find(path) != path_index_.end()) return false;

    if (auto parent_path = utils::PathUtils::GetParentPath(path); parent_path.has_value()) {
        auto parent_it = path_index_.find(parent_path.value());
        if (parent_it == path_index_.end()) return false;
        FsNode* parent_fs_node_ptr = parent_it->second;
        if (parent_fs_node_ptr->type != NodeType::Dir) return false;

        std::string dir_name(utils::PathUtils::GetFileName(path).value());
        FsNode* new_node = node_pool_.Allocate(NodeType::Dir, path, std::move(dir_name), parent_fs_node_ptr);

        parent_fs_node_ptr->children.push_back(new_node);
        path_index_[new_node->full_path] = new_node;
        ++dir_count;

        return true;
    }
    return false;
}

std::vector<std::string> TreeHashFileSystem::List(const std::string& path) const {
    if (auto it = path_index_.find(path); it != path_index_.end()) {
        FsNode* fs_node_ptr = it->second;
        if (fs_node_ptr->type != NodeType::Dir) return {};

        std::vector<std::string> answer;
        answer.reserve(fs_node_ptr->children.size());
        for (FsNode* child : fs_node_ptr->children) {
            answer.push_back(child->name);
        }
        return answer;
    }

    return {};
}


std::vector<std::string> TreeHashFileSystem::Find(const std::string& path, std::string pattern) const {
    // TODO: implement BFS/DFS find with regex
    return {};
}

std::optional<NodeInfo> TreeHashFileSystem::NodeState(const std::string& path) const {
    // TODO
    return std::nullopt;
}


std::optional<SystemInfo> TreeHashFileSystem::SystemState() const {
    SystemInfo info;
    info.file_count = file_count;
    info.dir_count = dir_count;
    info.node_count = node_pool_.GetActiveObjectsCount();
    info.total_file_bytes = total_file_bytes;

    // Эвристический подсчет оверхеда хэш-таблицы
    size_t map_overhead = (path_index_.bucket_count() * sizeof(void*)) + 
                          (path_index_.size() * sizeof(std::pair<std::string_view, FsNode*>));

    info.total_size_bytes = total_file_bytes + node_pool_.GetTotalAllocatedBytes() + map_overhead;
    return info;
}

bool TreeHashFileSystem::Move(const std::string& src, const std::string& dest) {
    // TODO:
}
