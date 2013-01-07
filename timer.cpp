#include "common.h"
#include <algorithm>

namespace QPC_Timer
{
    typedef LONGLONG Tick;
    Tick frequency;
    Tick overhead;
    double ticksToNanosecs;

    void Initialize()
    {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        frequency = f.QuadPart;
        overhead = 0;
        ticksToNanosecs = 1000000000.0 / frequency;
    }
}

#if INTEGER_MAP_TIMING_METHOD(RDTSC)
namespace RDTSC_Timer
{
    typedef __int64 Tick;
    Tick frequency;
    Tick overhead;
    double ticksToNanosecs;

    void Initialize()
    {
        QPC_Timer::Initialize();
        QPC_Timer::Tick limit = QPC_Timer::frequency / 10;
        QPC_Timer::Tick start = QPC_Timer::Sample();
        Tick startTsc = __rdtsc();
        while (QPC_Timer::Sample() - start < limit)
        {
        }
        Tick endTsc = __rdtsc();
        frequency = (endTsc - startTsc) * 10;
        ticksToNanosecs = 1000000000.0 / frequency;

        __asm
        {
            mov eax, 0
            cpuid
            mov eax, 0
            cpuid
        }
        // Take the median average of a bunch of CPUID timings and consider that the overhead.
        // Pretty sure I've seen a few magic fast CPUIDs, and a few slow ones.
        // Median average seems to produce the most consistent overhead measurement between runs.
        Tick timings[128];
        for (int i = 0; i < 128; i++)
        {
            startTsc = __rdtsc();
            __asm
            {
                mov eax, 0
                cpuid
            }
            endTsc = __rdtsc();
            timings[i] = endTsc - startTsc;
        }
        std::sort(timings, timings + 128);
        Tick total = 0;
        for (int i = 32; i < 96; i++)
            total += timings[i];
        overhead = total / 64;
    }
}
#endif // INTEGER_MAP_TIMING_METHOD(RDTSC)
