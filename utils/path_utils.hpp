#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>

namespace utils {

class PathUtils {
public:
    PathUtils() = delete;

    static std::optional<std::string_view> GetParentPath(std::string_view path);      // Извлекает имя родительской директории

    static std::optional<std::string_view> GetFileName(std::string_view path);        // Извлекает имя самого узла

    static std::optional<std::vector<std::string_view>> Split(std::string_view path); // Разбивает путь на токены для обхода дерева

    static bool IsValidPath(std::string_view path);                                   // Проверка валидности пути

    static std::string Join(std::string_view parent, std::string_view child);         // Путь для нового файла (может излишне)
};

} // namespace utils