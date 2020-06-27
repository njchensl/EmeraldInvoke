#include "MemBlock.h"
#include <memory>

MemBlock::MemBlock(unsigned int size, void* ptr) : m_Size(size)
{
    m_Pointer = malloc(size);
    memcpy(m_Pointer, ptr, size);
}

MemBlock::~MemBlock()
{
    free(m_Pointer);
}

MemBlock::MemBlock(MemBlock&& other)
{
    m_Size = other.m_Size;
    m_Pointer = other.m_Pointer;
    other.m_Pointer = nullptr;
}

MemBlock& MemBlock::operator=(MemBlock&& other)
{
    if (this != &other)
    {
        free(m_Pointer);
        m_Size = other.m_Size;
        m_Pointer = other.m_Pointer;
        other.m_Pointer = nullptr;
    }

    return *this;
}
