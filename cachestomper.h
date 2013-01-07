#pragma once

#include "mersennetwister.h"


//---------------------------------------------------
// CacheStomper ENABLED
//---------------------------------------------------
class CacheStomper
{
private:
    static const int kArrayLength = 0x2000000;  // 32 MB (must be power of 2)
    static const int kCacheLineSize = 64;
    static const int kStep = kCacheLineSize / sizeof(int);

    int* m_mem;
    float m_cacheLinesPerRandomStomp;
    MersenneTwister m_random;

public:
    CacheStomper(int bytesPerRandomStomp)
    {
        m_mem = new int[kArrayLength];
        m_cacheLinesPerRandomStomp = (float) bytesPerRandomStomp / kCacheLineSize;
        // Touch the whole array
        for (int ofs = 0; ofs < kArrayLength; ofs += kStep)
            m_mem[ofs]++;
    }

    ~CacheStomper()
    {
        delete[] m_mem;
    }

    void Seed(int seed)
    {
        m_random.reseed(seed);
    }

    void Stomp(size_t ofs, size_t length)
    {
        ofs &= kArrayLength - 1;
        if (length > kArrayLength)
            length = kArrayLength;
        while (length-- > 0)
        {
            m_mem[ofs]++;
            ofs += kStep;
            if (++ofs >= kArrayLength)
                ofs = 0;
        }
    }

    void RandomStomp()
    {
#if INTEGER_MAP_CACHE_STOMPER_ENABLED
        size_t ofs = m_random.integer() & (kArrayLength - 1);
        size_t length = (int) (m_random.expoVariate(m_cacheLinesPerRandomStomp) + 0.5f);
        Stomp(ofs, length);
#endif
    }

    void NukeMem()
    {
        for (int ofs = 0; ofs < kArrayLength; ofs += kStep)
            m_mem[ofs]++;
    }
};
