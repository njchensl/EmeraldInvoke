#pragma once
#include <algorithm>
#include <cstdlib>
#include <malloc.h>

class MemBlock final
{
public:
    MemBlock(unsigned int size, void* ptr);
    ~MemBlock();
    MemBlock(const MemBlock& other) = delete;
    MemBlock& operator=(const MemBlock& other) = delete;
    MemBlock(MemBlock&& other);
    MemBlock& operator=(MemBlock&& other);

    static MemBlock MakeEmpty(unsigned int size)
    {
        void* ptr = malloc(size);
        MemBlock* newBlock = (MemBlock*)alloca(sizeof(MemBlock));
        newBlock->m_Size = size;
        newBlock->m_Pointer = ptr;
        return std::move(*newBlock);
    }

    unsigned int GetSize() const
    {
        return m_Size;
    }

    void* Get()
    {
        return m_Pointer;
    }
private:
    unsigned int m_Size;
    void* m_Pointer;
};


