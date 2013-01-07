#---------------------------------------------------
# Perform a bunch of experiments using CompareIntegerMaps.exe, and writes the results to results.txt.
# You can filter the experiments by name by passing a regular expression as a script argument.
# For example: run_tests.py LOOKUP_0_.*
# Results are also cached in an intermediate directory, temp, so you can add new results to results.txt
# without redoing previous experiments.
#---------------------------------------------------

import cmake_launcher
import math
import os
import re
import sys
from collections import defaultdict
from pprint import pprint


#GENERATOR = 'Visual Studio 10'
IGNORE_CACHE = False


#---------------------------------------------------
#  TestLauncher
#---------------------------------------------------
class TestLauncher:
    """ Configures, builds & runs CompareIntegerMaps using the specified options. """
    
    DEFAULT_DEFS = {
        'CACHE_STOMPER_ENABLED': 0,
        'EXPERIMENT': 'INSERT',
        'CONTAINER': 'TABLE',
    }

    def __init__(self):
        cmakeBuilder = cmake_launcher.CMakeBuilder('..', generator=globals().get('GENERATOR'))
        # It would be cool to get CMake to tell us the path to the executable instead.
        self.launcher = cmake_launcher.CMakeLauncher(cmakeBuilder, 'CompareIntegerMaps.exe')

    def run(self, seed, operationsPerGroup, keyCount, granularity, stompBytes, **defs):
        args = [seed, operationsPerGroup, keyCount, granularity, stompBytes]
        mergedDefs = dict(self.DEFAULT_DEFS)
        mergedDefs.update(defs)
        fullDefs = dict([('INTEGER_MAP_' + k, v) for k, v in mergedDefs.iteritems()])
        self.launcher.ignoreCache = IGNORE_CACHE
        output = self.launcher.run(*args, **fullDefs)
        return eval(output)


#---------------------------------------------------
#  Experiment
#---------------------------------------------------
class Experiment:
    """ A group of CompareIntegerMaps runs using similar options but different seeds. """
    
    def __init__(self, testLauncher, name, seeds, *args, **kwargs):
        self.testLauncher = testLauncher
        self.name = name
        self.seeds = seeds
        self.args = args
        self.kwargs = kwargs

    def run(self, results):
        allGroups = defaultdict(list)
        for seed in xrange(self.seeds):
            print('Running %s #%d/%d...' % (self.name, seed + 1, self.seeds))
            r = self.testLauncher.run(seed, *self.args, **self.kwargs)
            for marker, units in r['results']:
                allGroups[marker].append(units)
        def medianAverage(values):
            if len(values) >= 4:
                values = sorted(values)[1:-1]
            return sum(values) / len(values)
        results[self.name] = [(marker, medianAverage(units)) for marker, units in sorted(allGroups.items())]


#---------------------------------------------------
#  main
#---------------------------------------------------
if __name__ == '__main__':
    from datetime import datetime
    start = datetime.now()

    os.chdir(os.path.split(sys.argv[0])[0])
    filter = re.compile((sys.argv + ['.*'])[1])
    if '--nocache' in sys.argv[1:]:
        IGNORE_CACHE = True
    results = {}

    testLauncher = TestLauncher()
    maxKeys = 18000000
    granularity = 200
    
    for container in ['TABLE', 'JUDY']:
        experiment = Experiment(testLauncher,
            'MEMORY_%s' % container,
            8 if container == 'JUDY' else 1, 0, maxKeys, granularity, 0,
            CONTAINER=container,
            EXPERIMENT='MEMORY')
        if filter.match(experiment.name):
            experiment.run(results)
            
        for stomp in [0, 1000, 10000]:
            experiment = Experiment(testLauncher,
                'INSERT_%d_%s' % (stomp, container),
                8, 8000, maxKeys, granularity, stomp,
                CONTAINER=container,
                EXPERIMENT='INSERT',
                CACHE_STOMPER_ENABLED=1 if stomp > 0 else 0)
            if filter.match(experiment.name):
                experiment.run(results)
                
            experiment = Experiment(testLauncher,
                'LOOKUP_%d_%s' % (stomp, container),
                8, 8000, maxKeys, granularity, stomp,
                CONTAINER=container,
                EXPERIMENT='LOOKUP',
                CACHE_STOMPER_ENABLED=1 if stomp > 0 else 0)
            if filter.match(experiment.name):
                experiment.run(results)
            
    pprint(results, open('results.txt', 'w'))
    print('Elapsed time: %s' % (datetime.now() - start))
