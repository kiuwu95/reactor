#pragma once
#include<vector>
#include<utility>
#include<iostream>

static inline size_t align_up(size_t n, size_t align) {
    return (n + (align - 1)) & ~(align - 1);
}

class MemoryPool {
public:
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    MemoryPool(const size_t& blocks_num = 4096, const size_t& blockSize = 128) :
        freeList_(nullptr), blocks_num_perPage_(blocks_num),
        blockSize_(adjustBlocksize(blockSize)) {
    }
    ~MemoryPool() {
        for (auto& it : pages_) {
            ::operator delete[](it);
        }
    }

    void* allocate() {
        if (!freeList_) {
            expand();
        }

        Node* space = freeList_;
        freeList_ = freeList_->next;
        return space;
    }
    void deallocate(void* p) {
        if (!p) {
            return;
        }

        Node* newHead = static_cast<Node*>(p);
        newHead->next = freeList_;
        freeList_ = newHead;
    }
private:
    size_t adjustBlocksize(const size_t& size) {
        if (size < sizeof(void*)) {
            return align_up(sizeof(void*), alignof(void*));
        }
        return align_up(size, alignof(void*));
    }

    void expand() {
        size_t pageSize = blocks_num_perPage_ * blockSize_;
        char* newPage = static_cast<char*>(::operator new[](pageSize));

        for (size_t i = 0;i < blocks_num_perPage_;i++) {
            char* f = newPage + i * blockSize_;
            Node* node = reinterpret_cast<Node*>(f);

            node->next = freeList_;
            freeList_ = node;
        }

        pages_.push_back(newPage);
    }
private:
    struct Node {
        Node* next;
    };
    Node* freeList_;

    std::vector<void*>pages_;

    size_t blocks_num_perPage_;
    size_t blockSize_;
};