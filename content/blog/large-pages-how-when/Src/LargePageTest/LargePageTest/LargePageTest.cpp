#define NOMINMAX

#include <iostream>
#include <windows.h>
#include <algorithm>
#include <random>
#include <functional>
#include <thread>

std::string GetErrorAsString(DWORD errorMessageID)
{
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);

    //Free the buffer.
    LocalFree(messageBuffer);

    return message;
}

auto GetNormalPageMinimum()
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwPageSize;
}

std::size_t Allocate(SIZE_T pageSize, DWORD allocationType, char* largeBuffer)
{
    std::size_t gb = 1024ull * 1024 * 1024;
    std::size_t  N_PAGES_TO_ALLOC = 1 * gb / pageSize;
    std::size_t buffersize = pageSize * N_PAGES_TO_ALLOC;
    std::cout << "Allocating " << buffersize << " bytes" << std::endl;
    largeBuffer = reinterpret_cast<char*>(VirtualAlloc(NULL, buffersize, allocationType, PAGE_READWRITE));
    if (!largeBuffer)
    {
        std::cout << "VirtualAlloc failed, error: " << GetErrorAsString(GetLastError());
        return 0;
    }

    return buffersize;
}

auto GetRandomPair(std::uint64_t min, std::uint64_t max)
{
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<std::uint64_t> distribution(min, max);
    return std::make_pair(rng, distribution);
}

void GenerateRandomMemoryAccesses(char* largeBuffer, std::size_t bufferSize)
{
    std::cout << "Generating random numbers ...";
    auto [valRng, valDistribution] = GetRandomPair(std::numeric_limits<std::uint8_t>::min(), std::numeric_limits<std::uint8_t>::max());
    auto [indexRng, indexDistribution] = GetRandomPair(0ull, bufferSize - 1);

    std::size_t numChanges = 0;
    auto curTime = std::chrono::high_resolution_clock::now();
    while (true)
    {
        auto index = indexDistribution(indexRng);
        auto val = valDistribution(valRng);
        largeBuffer[index] = val;
        ++numChanges;
        auto timeDif = std::chrono::high_resolution_clock::now() - curTime;
        if (timeDif > std::chrono::seconds(1))
        {
            auto thousandChangesPerSecond = numChanges / (1000.0f * std::chrono::duration_cast<std::chrono::milliseconds>(timeDif).count());
            std::cout << "ThousandChangesPerSecond: " << thousandChangesPerSecond << std::endl;
            numChanges = 0;
            curTime = std::chrono::high_resolution_clock::now();
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: LargePageTest [Enable/Disable]";
        return -1;
    }

    bool disableLargePage = argv[1] == "Disable";

    DWORD allocationType{};
    SIZE_T pageSize{};
    if (!disableLargePage)
    {
        pageSize = GetLargePageMinimum();
        std::cout << "Large page minimum: " << pageSize << std::endl;

        std::cout << "Allocating using Large Pages, page size: " << pageSize << std::endl;
        allocationType = MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES;
    }
    else
    {
        pageSize = GetNormalPageMinimum();

        std::cout << "Allocating using Normal-sized Pages page size: " << pageSize << std::endl;
        allocationType = MEM_RESERVE | MEM_COMMIT;
    }

    char* largeBuffer = nullptr;
    auto bufferSize = Allocate(pageSize, allocationType, largeBuffer);
    if (bufferSize == 0)
    {
        return -1;
    }

    GenerateRandomMemoryAccesses(largeBuffer, bufferSize);
    std::cout << "Done!" << std::endl;
}
