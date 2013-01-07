#pragma once


//---------------------------------------------------
// TestCase for INSERT operation
//---------------------------------------------------
void TestBody()
{
    ResultHolder rh;
    CacheStomper stomper(g_Params.stompBytes);

    // Determine markers
    std::vector<int> markers;
    g_Params.DefineMarkers(markers);
    std::vector<size_t> keys;
    int keySeed = 0;

    struct TimeGroup
    {
        double sum;
        int count;

        TimeGroup() : sum(0), count(0) {}
    };
    std::vector<TimeGroup> timeGroups;
    timeGroups.resize(markers.size());

    std::vector<Timer::Tick> ticks;
    ticks.resize(markers.size());

    int M = markers.size();
    int R = 0;
    int r = 0;
    for (;;)
    {
        if (r <= 0)
        {
            if (--M <= 0)
                break;
            int prevR = R;
            R = g_Params.operationsPerGroup / (M > 0 ? markers[M] - markers[M - 1] : 1) + 1;
            r = R - prevR;
            continue;
        }

        GenerateKeys(keys, markers[M], keySeed++);

        MAP_DECLARE;
        MAP_INITIALIZE();

        int i = 0;
        Timer::Tick* tick = &ticks[0];
        Timer::Tick accum = 0;
        for (int m = 0; m <= M; m++)
        {
            int limit = markers[m];
            for (; i < limit; i++)
            {
                // Insert & increment the table entry
                size_t key = keys[i];
                Timer::Tick start = Timer::Sample();
                MAP_INCREMENT(key);
                Timer::Tick end = Timer::Sample();
                accum += end - start - Timer::overhead;

                stomper.RandomStomp();
            }

            // Time measurement between each group of operations
            *tick++ = accum;
        }

        for (int m = 1; m <= M; m++)
        {
            double delta = (ticks[m] - ticks[m - 1]) * Timer::ticksToNanosecs;
            timeGroups[m].sum += delta;
            timeGroups[m].count++;
        }

        MAP_CLEAR();
        r--;
    }

    rh.results.resize(markers.size() - 1);
    for (int m = 1; m < markers.size(); m++)
    {
        ResultHolder::Result& r = rh.results[m - 1];
        r.marker = markers[m];
        int span = markers[m] - markers[m - 1];
        r.nanosecs = timeGroups[m].sum / timeGroups[m].count / span;
    }
    
    rh.dump();
};
