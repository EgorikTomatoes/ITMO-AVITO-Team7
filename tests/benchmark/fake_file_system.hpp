#pragma once

#include <Interface/file_system.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

// тесты без зависимости от конкретных реализаций (case_A/B/C) и utils.
class FakeFileSystem : public IFileSystem {
public:
    static constexpr std::size_t kMemoryBytes = 4242;

    mutable int read_calls = 0;
    int write_calls = 0;
    int mkdir_calls = 0;
    mutable int list_calls = 0;
    int move_calls = 0;
    mutable int find_calls = 0;

    int TotalCalls() const {
        return read_calls + write_calls + mkdir_calls + list_calls + move_calls + find_calls;
    }

    std::optional<std::string> ReadFile(const std::string&) const override {
        ++read_calls;
        return std::nullopt;
    }

    bool WriteToFile(const std::string&, const std::string&) override {
        ++write_calls;
        return true;
    }

    bool MakeDir(const std::string&) override {
        ++mkdir_calls;
        return true;
    }

    std::vector<std::string> List(const std::string&) const override {
        ++list_calls;
        return {};
    }

    bool Move(const std::string&, const std::string&) override {
        ++move_calls;
        return true;
    }

    std::vector<std::string> Find(const std::string&, std::string = "*") const override {
        ++find_calls;
        return {};
    }

    bool Exists(const std::string&) const override {
        return false;
    }

    std::optional<NodeInfo> NodeState(const std::string&) const override {
        return std::nullopt;
    }

    std::optional<SystemInfo> SystemState() const override {
        SystemInfo info{};
        info.total_size_bytes = kMemoryBytes;
        return info;
    }

    void Clear() override {}
};
