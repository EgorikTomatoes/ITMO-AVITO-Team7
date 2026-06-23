#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <cstddef>
#include <regex>

namespace utils {

class PathUtils {
public:
    PathUtils() = delete;

    static bool IsRootPath(std::string_view path) {
        return path == "/";
    }

    /*
        Контракт файловой системы: поддерживаем только абсолютные пути (начинаются с корня /), не поддерживаем относительные
            1)В силу того, что отсутствует понятие CWD (current working directory), у нас нет смысла поддерживать относительность
            2)Однако данная реализация корректно поддерживает скрытые файлы
    */
    static bool IsValidPath(std::string_view path) {                                
        if (path.empty() || path.front() != '/') return false;              // пустой или не начинается с / => не валидный

        for (size_t i = 1; i < path.length(); ++i) {
            if (path[i] == '/' && path[i-1] == '/') return false;           // запрет на двойные слеши

            else if (path[i] == '.' && path[i-1] == '/') {                  // запрет на относительные пути
                if (i+1 == path.length() || path[i+1] == '/')  return false;
                if (path[i+1] == '.' && (i+2 == path.length() || path[i+2] == '/')) return false;
            }  
        }
        return true;
    }                                  

    static std::optional<std::string_view> GetParentPath(std::string_view path) {     // Извлекает имя родительской директории
        if (!IsValidPath(path) || IsRootPath(path)) return std::nullopt;    

        if (path.back() == '/') path.remove_suffix(1);        
        
        size_t last_slash_pos = path.find_last_of('/');
        if (last_slash_pos == 0) return std::string_view("/");

        return path.substr(0, last_slash_pos);

    }
    static std::optional<std::string_view> GetFileName(std::string_view path) {       // Извлекает имя самого узла
        if (!IsValidPath(path)) return std::nullopt;
        if (IsRootPath(path)) return std::string_view("/");

        if (path.back() == '/') path.remove_suffix(1);
        size_t last_slash_pos = path.find_last_of('/');
        return path.substr(last_slash_pos + 1);
    }        

    static std::optional<std::vector<std::string_view>> Split(std::string_view path) { // Разбивает путь на токены для обхода дерева
        if (!IsValidPath(path)) return std::nullopt;
        if (IsRootPath(path)) return std::vector<std::string_view>{};
        if (path.back() == '/') path.remove_suffix(1);

        std::vector<std::string_view> result;
        size_t start = 1;
        size_t end = path.find('/',start);

        while (end != std::string_view::npos) {
            result.push_back(path.substr(start, end - start));
            start = end + 1;
            end = path.find('/', start);
        }

        if (start < path.length())  result.push_back(path.substr(start));

        return result;
    } 

    static std::string Join(std::string_view parent_path, std::string_view child_name) {       // Путь для нового файла при перемещении
        if (parent_path.empty()) return std::string(child_name);
        if (child_name.empty()) return std::string(parent_path);

        if (IsRootPath(parent_path)) return std::string(parent_path) + std::string(child_name);
        return std::string(parent_path) + "/" + std::string(child_name);
    }     

    /*
        Контракт файловой системы: поддерживаем стандартные glob-паттерны, которые используются в проводниках Windows/MacOs/Linux
    Данный метод нужен для эквивалентой трансляции glob-паттерна в регулярное выражение (в СТАНДАРТЕ C++ нет поддержки glob-паттерно, но есть <regex>)
    */
    static std::string GlobToRegex(std::string_view glob_pattern) {
        std::string regex_pattern;
        for (size_t i = 0; i < glob_pattern.length(); ++i) {
            char c = glob_pattern[i];
            if (c == '*') {
                regex_pattern += ".*";
            }
            else if (c == '?') {
                regex_pattern += ".";
            }
            else if (c == '!' && i > 0 && glob_pattern[i-1] == '[') {
                regex_pattern += "^";
            }
            else if (std::string(".+^$()|{}\\").find(c) != std::string::npos) {
                regex_pattern += "\\";
                regex_pattern += c;
            }
            else {
                regex_pattern += c;
            }
        }

        return regex_pattern;
    }

    static bool MatchPattern(std::string_view name, std::string_view glob_pattern) {
        try {
            std::regex rx(GlobToRegex(glob_pattern));
            return std::regex_match(std::string(name), rx);
        }
        catch (...) {
            return false;
        }
    }
};
} // namespace utils