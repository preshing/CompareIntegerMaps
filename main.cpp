#include "common.h"

#if INTEGER_MAP_EXPERIMENT(INSERT)
#include "test_insert.h"
#elif INTEGER_MAP_EXPERIMENT(LOOKUP)
#include "test_lookup.h"
#elif INTEGER_MAP_EXPERIMENT(MEMORY)
#include "test_memory.h"
#endif

TestParams g_Params;


//---------------------------------------------------
// GenerateKeys
//---------------------------------------------------
void GenerateKeys(std::vector<size_t>& m_keys, int keyCount, int M)
{
    m_keys.resize(keyCount);

#if INTEGER_MAP_KEY_GENERATION(LINEAR)
    for (int i = 0; i < keyCount; i++)
    {
        m_keys[i] = i;
    }

#elif INTEGER_MAP_KEY_GENERATION(RANDOM_SEQUENCE_OF_UNIQUE)
    RandomSequenceOfUnique rsu(g_Params.seed + M, g_Params.seed + M);
    for (int i = 0; i < keyCount; i++)
    {
        m_keys[i] = rsu.next();
    }

#elif INTEGER_MAP_KEY_GENERATION(SORTED_ADDRESSES) || INTEGER_MAP_KEY_GENERATION(SHUFFLED_ADDRESSES)
    // Inputs are simulated memory addresses
    size_t ptr = (g_Params.random.integer() % 0xfff0 + 0x10) * 0x10000;
    for (int i = 0; i < keyCount; i++)
    {
        m_keys[i] = ptr;
        ptr += g_Params.random.integer() % INTEGER_MAP_MAX_ADDRESS_BLOCK_SIZE;
        ptr = (ptr | 15) + 1;
    }

#if INTEGER_MAP_KEY_GENERATION(SHUFFLED_ADDRESSES)
    // Shuffle the addresses
    for (int i = 0; i < keyCount; i++)
    {
        int swap = i + g_Params.random.integer() % (keyCount - i);
        size_t temp = m_keys[i];
        m_keys[i] = m_keys[swap];
        m_keys[swap] = temp;
    }
#endif // INTEGER_MAP_KEY_GENERATION(SHUFFLED_ADDRESSES)

#else
    #error No such INTEGER_MAP_KEY_GENERATION type.
#endif
}


//---------------------------------------------------
// DefineMarkers
//---------------------------------------------------
void TestParams::DefineMarkers(std::vector<int>& markers)
{
    markers.push_back(9);
    int prevLimit = 9;
    for (int m = 0;; m++)
    {
        int limit = (int) (0.5f + powf(10, (float) m / this->granularity));
        if (limit > prevLimit)
        {
            markers.push_back(limit);
            prevLimit = limit;
            if (limit >= this->keyCount)
                break;
        }
    }
}


//---------------------------------------------------
// ResultHolder
//---------------------------------------------------
void ResultHolder::dump()
{
    printf("{\n");
    printf("    'INTEGER_MAP_CACHE_STOMPER_ENABLED': %d,\n", INTEGER_MAP_CACHE_STOMPER_ENABLED);
    printf("    'INTEGER_MAP_TWEAK_PRIORITY_AFFINITY': %d,\n", INTEGER_MAP_TWEAK_PRIORITY_AFFINITY);
    printf("    'INTEGER_MAP_USE_DLMALLOC': %d,\n", INTEGER_MAP_USE_DLMALLOC);
    printf("    'INTEGER_MAP_TIMING_METHOD': '%s',\n", INTEGER_MAP_TIMING_METHOD_STR);
    printf("    'INTEGER_MAP_EXPERIMENT': '%s',\n", INTEGER_MAP_EXPERIMENT_STR);
    printf("    'INTEGER_MAP_CONTAINER': '%s',\n", INTEGER_MAP_CONTAINER_STR);
    printf("    'INTEGER_MAP_KEY_GENERATION': '%s',\n", INTEGER_MAP_KEY_GENERATION_STR);
    printf("    'INTEGER_MAP_MAX_ADDRESS_BLOCK_SIZE': %d,\n", INTEGER_MAP_MAX_ADDRESS_BLOCK_SIZE);
    printf("    'seed': %d,\n", g_Params.seed);
    printf("    'operationsPerGroup': %d,\n", g_Params.operationsPerGroup);
    printf("    'keyCount': %d,\n", g_Params.keyCount);
    printf("    'granularity': %d,\n", g_Params.granularity);
    printf("    'stompBytes': %d,\n", g_Params.stompBytes);
    printf("    'results': [\n");
    for (int m = 0; m < results.size(); m++)
    {
        printf("        (%d, %f),\n", results[m].marker, results[m].nanosecs);
    }
    printf("    ],\n");
    printf("}\n");
}


//---------------------------------------------------
// main
//---------------------------------------------------
int main(int argc, const char* argv[])
{
    if (argc != 6)
    {
        fputs("Expected 5 arguments\n", stderr);
        return 1;
    }

#if INTEGER_MAP_TWEAK_PRIORITY_AFFINITY
    SetThreadAffinityMask(GetCurrentThread(), 1);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#endif

    Timer::Initialize();

    g_Params.seed = atoi(argv[1]);
    g_Params.operationsPerGroup = atoi(argv[2]);
    g_Params.keyCount = atoi(argv[3]);
    g_Params.granularity = atoi(argv[4]);
    g_Params.stompBytes = atoi(argv[5]);
    g_Params.random.reseed(g_Params.seed);

    TestBody();
    return 0;
}

