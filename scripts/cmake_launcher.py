import subprocess
import shelve
import os
import cPickle
import time


class CMakeBuilder:
    def __init__(self, srcPath, buildPath='build', generator=None, config='Release'):
        """Initialize the project using CMake in the given buildPath."""
        if not os.path.exists(buildPath):
            os.makedirs(buildPath)
        absSrcPath = os.path.abspath(srcPath)
        prevDir = os.getcwd()
        try:
            os.chdir(buildPath)
            args = ['-G', generator] if generator else []
            subprocess.check_call(['cmake'] + args + [absSrcPath])
        finally:
            os.chdir(prevDir)
        self.prevDefs = None
        self.buildPath = buildPath
        self.config = config
            
    def build(self, **defs):
        """If the definitions (defs) have changed, use CMake to reconfigure and build the project."""
        if self.prevDefs == None or defs != self.prevDefs:
            defines = ['-D%s=%s' % (k, str(v)) for k, v in defs.iteritems()]
            subprocess.check_output(['cmake'] + defines + [self.buildPath])
            subprocess.check_output(['cmake', '--build', self.buildPath, '--config', self.config])
            self.prevDefs = defs        


class CMakeLauncher:
    def __init__(self, cmakeBuilder, exeName, tempPath='temp', configName='Release'):
        """Open the persistent cache for results using the path 'temp\cached_results'."""
        if not os.path.exists(tempPath):
            os.makedirs(tempPath)
        self.cachedResults = shelve.open(os.path.join(tempPath, 'cached_results'))
        self.cmakeBuilder = cmakeBuilder
        self.exeName = exeName
        self.ignoreCache = False
        self.configName = configName

    def run(self, *args, **defs):        
        """First check for existing results in the persistent cache. If they don't exist, build/run the executable."""
        key = cPickle.dumps((args, defs))
        if not self.ignoreCache:
            if key in self.cachedResults:
                return self.cachedResults[key]
        self.cmakeBuilder.build(**defs)
        # It would be cool if CMake could tell us the name of the output executable.
        pathToExe = os.path.join(self.cmakeBuilder.buildPath, self.configName, self.exeName)
        time.sleep(0.2)  # In case the OS steals some CPU time refreshing the display from previous print statements.
        value = subprocess.check_output([pathToExe] + [str(a) for a in args])
        self.cachedResults[key] = value
        return value
