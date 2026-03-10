#ifndef POOL_ALLOCATOR_H
#define POOL_ALLOCATOR_H

#include <cstddef>
#include <new>
#include <memory>
#include <iostream>

template<typename T, std::size_t ChunkSize = 64>
class PoolMapAllocator
{
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using difference_type = std::ptrdiff_t;
    using size_type = std::size_t;

    PoolMapAllocator() : pool_(std::make_shared<PoolImpl>()) {}

    PoolMapAllocator(const PoolMapAllocator& other) noexcept = default;

    template<typename U>
    PoolMapAllocator(const PoolMapAllocator<U, ChunkSize>&) noexcept
        : pool_(std::make_shared<PoolImpl>()) {}

    ~PoolMapAllocator() = default;

    template<typename U>
    struct rebind
    {
        using other = PoolMapAllocator<U, ChunkSize>;
    };

    pointer allocate(size_type numObjects)
    {
        if (numObjects != 1)
        {
            return static_cast<pointer>(::operator new(numObjects * sizeof(T)));
        }

        if (pool_->free_list == nullptr)
            pool_->allocate_new_block();

        void* result = pool_->free_list;
        pool_->free_list = *static_cast<void**>(pool_->free_list);
        ++pool_->total_allocated;
        return static_cast<pointer>(result);
    }

    void deallocate(pointer p, size_type numObjects) noexcept
    {
        if (p == nullptr) return;
        if (numObjects != 1)
        {
            ::operator delete(p);
            return;
        }

        *reinterpret_cast<void**>(p) = pool_->free_list;
        pool_->free_list = p;
        --pool_->total_allocated;
    }

    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::false_type;
    using propagate_on_container_swap = std::false_type;

    bool operator==(const PoolMapAllocator& other) const noexcept
    {
        return pool_ == other.pool_;
    }

    bool operator!=(const PoolMapAllocator& other) const noexcept
    {
        return !(*this == other);
    }

    size_type getAllocations() const
    {
        return pool_->total_allocated;
    }

private:
    struct PoolImpl
    {
        struct BlockList
        {
            void* ptr;
            BlockList* next;
            BlockList(void* p, BlockList* n) : ptr(p), next(n) {}
        };

        BlockList* blocks = nullptr;
        void* free_list = nullptr;
        std::size_t total_allocated = 0;

        ~PoolImpl()
        {
            free_blocks();
        }

        void push_front(void* block)
        {
            blocks = new BlockList(block, blocks);
        }

        void free_blocks()
        {
            while (blocks)
            {
                void* to_free = blocks->ptr;
                BlockList* next = blocks->next;
                ::operator delete(to_free);
                delete blocks;
                blocks = next;
            }
        }

        void allocate_new_block()
        {
            void* new_block = ::operator new(ChunkSize * sizeof(T));
            push_front(new_block);

            char* start = static_cast<char*>(new_block);
            for (std::size_t i = 0; i < ChunkSize; ++i)
            {
                void* cell = start + i * sizeof(T);
                *static_cast<void**>(cell) = free_list;
                free_list = cell;
            }
        }
    };

    std::shared_ptr<PoolImpl> pool_;
};


template<std::size_t ChunkSize>
class PoolMapAllocator<void, ChunkSize>
{
public:
    using pointer = void*;
    using const_pointer = const void*;
    using value_type = void;

    template<typename U>
    struct rebind
    {
        using other = PoolMapAllocator<U, ChunkSize>;
    };
};

#endif