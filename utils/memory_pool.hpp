#pragma once

#include <vector>
#include <memory>
#include <cstddef>
#include <utility>

namespace utils {

template <typename T, size_t ChunkSize = 1024>
class MemoryPool {
    static_assert(ChunkSize > 0, "ChunkSize must be strictly greater than 0");
private:
    union Node {
        alignas(T) std::byte storage[sizeof(T)];        // Если блок занят: хранилище для типа T, строго учитывающее выравнивание для типа T
        Node* next;                                     // Если блок свободен: указатель на следующий свободный блок
    };

    Node* free_head_ = nullptr;                         // Указатель на голову Free List свободных блоков
    std::vector<std::unique_ptr<Node[]>> chunks_;       // Хранилище чанков, владеет чанком, и очищает чанки в деструкторе 

    size_t total_allocated_bytes_ = 0;                  // Память под сами блоки (заполненные + пустые), не учитывая данные файлов
    size_t active_objects_ = 0;                         // Количество активных файлов и директорий

    void AllocateChunk() {                              // Выделение системным аллокатором (new) одного чанка памяти

        auto new_chunk = std::make_unique<Node[]>(ChunkSize);  // Выделяем чанк из ChunkSize блоков

        for (size_t i = 0; i < ChunkSize - 1; ++i) {            // Связываем свободные блоки нового чанка в список
            new_chunk[i].next = &new_chunk[i+1];
        }

        new_chunk[ChunkSize-1].next = free_head_;               // Последний блок нового чанка ссылается на старую голову списка
        free_head_ = &new_chunk[0];                             // Обновляем головку списка свободных блоков

        chunks_.push_back(std::move(new_chunk));
        total_allocated_bytes_ += ChunkSize * sizeof(Node);
    }                              
public:

    MemoryPool() = default;
    ~MemoryPool() = default;

   
    MemoryPool(const MemoryPool&) = delete;             // Запрет на копирование пула
    MemoryPool& operator=(const MemoryPool&) = delete;

    MemoryPool(MemoryPool&& other) noexcept             // Перемещение пула разрешено
        : free_head_(std::exchange(ohter.free_head_, nullptr)),
          chunks_(std::move(other.chunks_)),
          total_allocated_bytes_(std::exchange(other.total_allocated_bytes_,0)),
          active_objects_(std::exchange(other.active_objects_,0))
    {}

    MemoryPool& operator=(MemoryPool&& other) noexcept {
        if (this != &other) {
            chunks_.clear();
            free_head_ = std::exchange(other.free_head_, nullptr);
            chunks_    = std::move(other.chunks_);
            total_allocated_bytes_ = std::exchange(other.total_allocated_bytes_,0);
            active_objects_ = std::exchange(other.active_objects_,0);
        }
        return *this;
    }

    template <typename... Args>                         // Выделение памяти из пулка и создание объекта типа T
    T* Allocate(Args&&.. args) {
        if (!free_head_) {
            AllocateChunk();
        }

        Node* free_node = free_head_;                   // Берём первый свободный блок из Free List
        free_head_ = free_head_->next;

        ++active_objects_;
        return new (&free_node->storage) T(std::forward<Args>(args)...);    
    }

    void Deallocate(T* ptr) {                           // Удаление объекта по указателю и возврат памяти в пул
        if (!ptr) return;

        ptr->~T();                                      // Явно вызываем деструктор хранимого объекта (чтобы динамически типы корректно почистили память)

        Node* node = reinterpret_cast<Node*>(ptr);      // Превращаем очищенный блок в голову FreeList
        node->next = free_head_;
        free_head_ = node;

        --active_objects_;
    }                           

    size_t GetTotalAllocatedBytes() const {            
        return total_allocated_bytes_;
    }            
    size_t GetActiveObjectsCount()  const {
        return active_objects_;
    }             
};

} // namespace utils