#pragma once

#include <string>

enum class OperationType {
    Read,
    Write,
    Mkdir,
    List,
    Move,
    Find
};

struct Operation {
    OperationType type;

    std::string path1;
    std::string path2;
    std::string data;
    std::string pattern;
};