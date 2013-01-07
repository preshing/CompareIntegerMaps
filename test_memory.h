#pragma once

#if !INTEGER_MAP_USE_DLMALLOC
#error INTEGER_MAP_USE_DLMALLOC must be true to use INTEGER_MAP_EXPERIMENT(MEMORY)
#endif


//---------------------------------------------------
// TestCase for MEMORY operation
//---------------------------------------------------
extern "C"
{
    typedef struct
    {
        size_t maxfp;
        size_t fp;
        size_t used;
    } dlmalloc_stats_t;

    void  dlmalloc_stats(dlmalloc_stats_t *stats);
}

void TestBody()
{
    ResultHolder rh;

    // Determine markers
    std::vector<int> markers;
    g_Params.DefineMarkers(markers);
    rh.results.resize(markers.size());

    std::vector<size_t> keys;
    GenerateKeys(keys, markers[markers.size() - 1], 0);

    dlmalloc_stats_t stats;
    dlmalloc_stats(&stats);
    size_t memAtStart = stats.used;

    MAP_DECLARE;
    MAP_INITIALIZE();

    int i = 0;
    for (int m = 0; m < markers.size(); m++)
    {
        int limit = markers[m];
        for (; i < limit; i++)
        {
            // Insert & increment the table entry
            MAP_INCREMENT(keys[i]);
        }

        ResultHolder::Result& r = rh.results[m];
        r.marker = markers[m];

        dlmalloc_stats_t stats;
        dlmalloc_stats(&stats);
        r.nanosecs = stats.used - memAtStart;
    }

    MAP_CLEAR();

    rh.dump();
};
