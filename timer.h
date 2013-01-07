#pragma once


namespace QPC_Timer
{
    typedef LONGLONG Tick;
    extern Tick frequency;
    extern Tick overhead;
    extern double ticksToNanosecs;

    void Initialize();

    inline Tick Sample()
    {
        LARGE_INTEGER t;
        QueryPerformanceCounter(&t);
        return t.QuadPart;
    }
}

#if INTEGER_MAP_TIMING_METHOD(RDTSC)
namespace RDTSC_Timer
{
    typedef __int64 Tick;
    extern Tick frequency;
    extern Tick overhead;
    extern double ticksToNanosecs;

    void Initialize();

    inline Tick Sample()
    {
        __asm
        {
            mov eax, 0
            cpuid
        }
        return __rdtsc();
    }
}
#endif // INTEGER_MAP_TIMING_METHOD(RDTSC)


#if INTEGER_MAP_TIMING_METHOD(QUERY_PERFORMANCE_COUNTER)
#define Timer QPC_Timer
#elif INTEGER_MAP_TIMING_METHOD(RDTSC)
#define Timer RDTSC_Timer
#endif
