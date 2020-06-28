#include <iostream>
#include "cinvoke.h"
#include <chrono>
#include <thread>
#include <cstdint>
#include <vector>

#include "MemBlock.h"

#define BUF_SIZE 8192

HANDLE hMapFile;
LPCTSTR pBuf;

void* InitMemMap()
{
    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        BUF_SIZE,
        "em-invoke");
    if (hMapFile == nullptr)
    {
        printf("Could not create file mapping object (%lu).\n", GetLastError());
        return nullptr;
    }

    pBuf = (LPTSTR)MapViewOfFile(hMapFile,
                                 FILE_MAP_ALL_ACCESS,
                                 0,
                                 0,
                                 BUF_SIZE);
    if (pBuf == nullptr)
    {
        printf("Could not map view of file (%lu).\n", GetLastError());

        CloseHandle(hMapFile);

        return nullptr;
    }
    return (void*)pBuf;
}

void CleanUpMemMap()
{
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
}

enum class MemStatus : int
{
    Exit = -1,
    Idle = 0,
    ServerRead = 1,
    ClientRead = 2
};

// extern "C" __declspec(dllexport) void fun(char a, short i, int* ptr)
// {
//     std::cout << a << std::endl;
//     std::cout << i << std::endl;
//     std::cout << *ptr << std::endl;
//     *ptr = 30;
// }

int main()
{
    int* ptr = (int*)InitMemMap();
    if (!ptr)
    {
        std::cerr << "Error" << std::endl;
        return 1;
    }
    CInvContext* context = cinv_context_create();
    ptr[0] = (int)MemStatus::Idle;
    
    // ptr[0] = 1;
    // uint8_t data[] = {0, 3, 'f', 'u', 'n', 0, '\0', 3, 'c', 's', 'p', 77, 10, 0, 4, 0, 0, 0, 20, 0, 0, 0};
    // memcpy(ptr + 1, data, sizeof(data));
    

    // start receiving commands
    for (;;)
    {
        MemStatus command = (MemStatus)ptr[0];
        switch (command)
        {
        case MemStatus::Idle:
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            break;
        }
        case MemStatus::Exit:
        {
            goto EXIT;
        }
        case MemStatus::ServerRead:
        {
            class MemBlockAllocator
            {
            public:
                MemBlockAllocator(unsigned int size) : m_Block(MemBlock::MakeEmpty(size)),
                                                       m_Head((uint8_t*)m_Block.Get())
                {
                }

                void* Allocate(unsigned int size)
                {
                    m_Head += size;
                    return m_Head - size;
                }

            private:
                MemBlock m_Block;
                uint8_t* m_Head;
            };
            MemBlockAllocator allocator(4096);

            // get module
            uint8_t* head = (uint8_t*)(ptr + 1);
            uint8_t moduleNameLength = *head;
            head++;
            HMODULE module;
            if (moduleNameLength == 0)
            {
                module = GetModuleHandle(nullptr);
            }
            else
            {
                char* dllName = (char*)allocator.Allocate(moduleNameLength + 1);
                memcpy(dllName, head, moduleNameLength);
                dllName[moduleNameLength] = '\0';
                head += moduleNameLength;
                module = LoadLibraryA(dllName);
            }
            // get proc address
            uint8_t procNameLength = *head;
            head++;
            char* procName = (char*)allocator.Allocate(procNameLength + 1);
            memcpy(procName, head, procNameLength);
            procName[procNameLength] = '\0';
            head += procNameLength;
            FARPROC procAddress = GetProcAddress(module, procName);
            if (!procAddress)
            {
                std::cerr << "no such proc" << std::endl;
                break;
            }
            // calling convention
            uint8_t callingConventionCode = *head;
            head++;
            cinv_callconv_t callingConvention = (cinv_callconv_t)callingConventionCode;
            // return format
            char returnFormatCode = *(char*)head;
            head++;
            char returnFormat[2] = {returnFormatCode, '\0'};
            // param format
            uint8_t numParams = *head;
            head++;
            char* paramFormat = (char*)allocator.Allocate(numParams + 1);
            memcpy(paramFormat, head, numParams);
            head += numParams;
            paramFormat[numParams] = '\0';
            // params
            void** params = (void**)allocator.Allocate(sizeof(void*) * numParams);
            uint8_t* paramData = (uint8_t*)allocator.Allocate(numParams * 8);
            uint8_t* paramDataHead = paramData;
            std::vector<MemBlock> memBlocks;
            for (uint8_t i = 0; i < numParams; i++)
            {
                char paramCode = paramFormat[i];
                switch (paramCode)
                {
#define PARAM_CODE(name, type) case name : { memcpy(paramDataHead, head, sizeof(type)); \
                        head += sizeof(type);                                           \
                        params[i] = paramDataHead;                                      \
                        break; }
                PARAM_CODE('c', char)
                PARAM_CODE('s', short)
                PARAM_CODE('i', int)
                PARAM_CODE('l', long)
                PARAM_CODE('e', long long)
                PARAM_CODE('f', float)
                PARAM_CODE('d', double)
                case 'p':
                {
                    unsigned int size = *(unsigned int*)head;
                    head += sizeof(unsigned int);
                    MemBlock dataBlock(size, head);
                    head += size;
                    void* dataPtr = dataBlock.Get();
                    memcpy(paramDataHead, &dataPtr, sizeof(void*));
                    memBlocks.push_back(std::move(dataBlock));
                    params[i] = paramDataHead;
                    break;
                }
                PARAM_CODE('2', uint16_t)
                PARAM_CODE('4', uint32_t)
                PARAM_CODE('8', uint64_t)
#undef PARAM_CODE
                }
                paramDataHead += 8;
            }
            CInvFunction* func = cinv_function_create(context, callingConvention, returnFormat, paramFormat);
            uint64_t returnVal;
            cinv_status_t status = cinv_function_invoke(context, func, (void*)procAddress, &returnVal, params);
            if (status != CINV_SUCCESS)
            {
                std::cerr << "Error" << std::endl;
            }
            cinv_function_delete(context, func);
            head = (uint8_t*)(ptr + 1);
            memcpy(head, &returnVal, 8);
            // restore pointers
            // information regarding the size of the blocks is known by the caller
            for (MemBlock& block : memBlocks)
            {
                unsigned int blockSize = block.GetSize();
                memcpy(head, block.Get(), blockSize);
                head += blockSize;
            }
            ptr[0] = (int)MemStatus::ClientRead;
            break;
        }
        case MemStatus::ClientRead:
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            break;
        }
        }
    }
EXIT:

    cinv_context_delete(context);

    CleanUpMemMap();
    return 0;
}
