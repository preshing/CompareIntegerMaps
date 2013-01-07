This project generates benchmark data for two different data structures, then renders some graphs.

Each data structure is an associative map (aka [associative array](http://en.wikipedia.org/wiki/Associative_array)) in which both the key and value types are plain integers. One is a [Judy array](http://judy.sourceforge.net/), and the other is a custom hash table implemented in `hashtable.cpp` and `hashtable.h`.

You can view examples of the generated graphs in the accompanying blog post, [This Hash Table Is Faster Than a Judy Array](http://preshing.com/20130107/this-hash-table-is-faster-than-a-judy-array).

Code is released to the public domain, except for the Judy array implementation which is LGPL.

# Requirements

* [CMake](http://www.cmake.org/) 2.8.6 or higher
* A C++ development environment supported by CMake. I used [Visual Studio 2010 Express](http://www.microsoft.com/visualstudio/eng/products/visual-studio-2010-express).
* [Python](http://www.python.org/) 2.5 - 2.7
* [Pycairo](http://cairographics.org/pycairo/), if you wish to render the graphs

**NOTE:** This benchmark suite makes heavy use of the x86 `RDTSC` instruction, which means you are likely to get skewed results on certain CPUs unless you disable dynamic frequency scaling technologies such as Turbo Boost. See the **Benchmarking Methodology** section for more information.

Everything was implemented and tested on Windows, but it's 99% portable code, so a determined programmer should be able to get it running on other platforms with relatively minor effort. Feel free to send me pull requests with such improvements.

# How to Generate All of the Benchmark Data

All of the benchmark data gathered during the generation process, which is required for rendering the graphs, is stored in the intermediate file `results.txt`, found in the `scripts` subfolder.

This repository already includes a `results.txt` file containing a complete set of sample benchmark data. So, if you like, you could skip this entire section, and go straight to generating the graphs.

To generate all of the benchmark data from scratch, and write new `results.txt` file, execute the Python script `gather_benchmarks.py` in the `scripts` subfolder. It could take anywhere from 30 minutes to two hours to complete, depending on your machine.

To collect the cleanest possible benchmark data, you should run the script in a "quiet" CPU environment, with as few background services running as possible, such as Safe Mode with all non-essential Services stopped.

Internally, this script uses CMake to configure, generate and build a small C++ benchmarking application called `CompareIntegerMaps`. By default, CMake will use whatever project generator it detects as being available. On my system, this turned out to be Visual Studio 2010. If you need to force CMake to use a specific generator, simply uncomment the following line in `gather_benchmarks.py`, and replace the name of the generator with the one you want.

    #GENERATOR = 'Visual Studio 10'

A list of all available generators can be found my running CMake with no arguments.

# How to Generate a Subset of the Benchmark Data

If you don't want to wait 30+ minutes for all the results, you can just generate a subset of the benchmark data. The benchmark data is organized into datasets, where each dataset corresponds to a single curve on one of the graphs. The first argument to `gather_benchmarks.py` is a regular expression telling the script which datasets to generate. Here's a list of all the datasets:

    INSERT_0_JUDY
    INSERT_0_TABLE
    INSERT_1000_JUDY
    INSERT_1000_TABLE
    INSERT_10000_JUDY
    INSERT_10000_TABLE
    LOOKUP_0_JUDY
    LOOKUP_0_TABLE
    LOOKUP_1000_JUDY
    LOOKUP_1000_TABLE
    LOOKUP_10000_JUDY
    LOOKUP_10000_TABLE
    MEMORY_JUDY
    MEMORY_TABLE

So for example, if you only want to generate the first graph seen in the blog post, you could just run:

    gather_benchmarks.py INSERT_0_.*

Then, the `results.txt` file will contain only the INSERT\_0\_JUDY and INSERT\_0\_TABLE datasets.

A dataset consists of an average of the results of some number of runs of the C++ application. When you run `gather_benchmarks.py`, the results of each run are stored in an intermediate cache. The cache is automatically created in the `scripts/temp` subfolder. The next time you run `gather_benchmarks.py`, it will check for results in the cache first. This lets you interrupt the benchmark suite at any time, and continue it later. Also, once all the results are in the cache, running `gather_benchmarks.py` becomes instant.

If you want to ignore the contents of the cache, use the `--nocache` option. For example, to regenerate everything from scratch, you could either delete the entire `scripts/temp` subfolder, or use the following:

    gather_benchmarks.py .* --nocache

# How to Generate the Graphs

Make sure you have Pycairo installed, and run `render_graphs.py` in the `scripts` subfolder. This will read the `results.txt` file and output five images:

    insert.png
    lookup.png
    insert-cache-stomp.png
    lookup-cache-stomp.png
    memory.png

If you only want to generate certain graphs, specify a regular expression as the first script argument.

    make_graphs.py "(insert.png|lookup.png)"

If any datasets are missing from `results.txt`, those curves will be missing from the generated graphs.

# Verifying that the Hash Table Works Correctly

Since this project contains a custom hash table implementation, I had to make sure it worked correctly. For this, a small suite of randomized stress tests was written. The tests are built around a small C++ application called `ValidateHashTable`. If you want to run it, you must first generate the project files for `ValidateHashTable` using CMake, then build the application (possibly using CMake), then run the test suite using CTest. For example, on my system, I can open a command prompt in the `validate` folder, and do the following:

    mkdir build
    cd build
    cmake .. -G "Visual Studio 10"
    cmake --build . --config Debug
    ctest . -C Debug

This will launch 100 tests. Each test invokes the Python script `validate/test.py` using a different random seed. The script will invoke the `ValidateHashTable` application, feed a bunch of hash table commands to it via stdin, fetch the result via stdout, then compare the result to the same operations applied on a Python dictionary. The tests passes only if the exactly hash table matches the Python dictionary. There are also some random lookups performed along the way; those must match too.

# Benchmarking Methodology

This benchmark suite makes heavy use of the [x86 `RDTSC` instruction](http://en.wikipedia.org/wiki/Time_Stamp_Counter) to take very fine performance measurements. It also locks the thread of execution to a single CPU core, to avoid imprecisions caused by having different timers on each core. If your computer features dynamic frequency scaling, such as Intel Turbo Boost, you should disable it before running this benchmark suite. The option should be available somewhere in your BIOS settings. If you don't disable dynamic frequency scaling, your results are [likely to be skewed in some way](http://randomascii.wordpress.com/2011/07/29/rdtsc-in-the-age-of-sandybridge/). I ran the suite on a Core 2 Duo processor, which doesn't have dynamic frequency scaling, so there was no issue.

As mentioned above, your results will be much more precise if you run the benchmark suite in a quiet CPU environment, with as few background services running as possible, such as Safe Mode with all non-essential Services stopped.

The MSVC project settings were chosen to avoid as many known [performance pitfalls](http://preshing.com/20110711/visual-c-performance-pitfalls) as possible.
