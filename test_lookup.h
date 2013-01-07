#pragma once


//---------------------------------------------------
// TestCase for LOOKUP operation
//---------------------------------------------------
void TestBody()
{
    ResultHolder rh;
    CacheStomper stomper(g_Params.stompBytes);

    // Determine markers
    std::vector<int> markers;
    g_Params.DefineMarkers(markers);

    std::vector<size_t> keys;
    GenerateKeys(keys, markers[markers.size() - 1], 0);

    rh.results.resize(markers.size());

    MAP_DECLARE;
    MAP_INITIALIZE();

    int i = 0;
    for (int m = 0; m < markers.size(); m++)
    {
        int population = markers[m];
        for (; i < population; i++)
        {
            // Insert & increment the table entry
            MAP_INCREMENT(keys[i]);
        }

        // Make sequence of keys to get
        int mustLookup = g_Params.operationsPerGroup;
        Timer::Tick start, end;
        Timer::Tick accum = 0;
        for (int j = 0; j < mustLookup; j++)
        {
            size_t key = keys[g_Params.random.integer() % population];
            start = Timer::Sample();
            MAP_INCREMENT(key);
            end = Timer::Sample();
            accum += end - start - Timer::overhead;

            stomper.RandomStomp();
        }

        ResultHolder::Result& r = rh.results[m];
        r.marker = population;
        r.nanosecs = accum * Timer::ticksToNanosecs / mustLookup;
    }

    MAP_CLEAR();
    
    rh.dump();
};
