import subprocess
import random
import sys

class HashTableWrapper:
    def __init__(self, pathToExe):
        self.p = subprocess.Popen(pathToExe, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
#        self.p.stdin = DebugPrintFilter(self.p.stdin)
    def __setitem__(self, key, value):
        self.p.stdin.write('insert %d %d\n' % (key, value))
    def __getitem__(self, key):
        self.p.stdin.write('lookup %d\n' % key)
        return eval(self.p.stdout.readline())
    def increment(self, key):
        self.p.stdin.write('increment %d\n' % key)
    def __delitem__(self, key):
        self.p.stdin.write('delete %d\n' % key)
    def clear(self):
        self.p.stdin.write('clear\n')
    def compact(self):
        self.p.stdin.write('compact\n')
    def run(self, test, *args):
        r = test(self, *args)
        self.p.stdin.close()
        return r, eval(self.p.stdout.read())

class DictionaryWrapper:
    def __init__(self):
        self.d = {}
    def __setitem__(self, key, value):
        self.d[key] = value
    def __getitem__(self, key):
        return self.d.get(key)
    def increment(self, key):
        self.d[key] = self.d.get(key, 0) + 1
    def __delitem__(self, key):
        if key in self.d:
            del self.d[key]
    def clear(self):
        self.d.clear()
    def compact(self):
        pass
    def run(self, test, *args):
        return test(self, *args), self.d

class DebugPrintFilter:
    def __init__(self, pipe):
        self.pipe = pipe
    def write(self, text):
        sys.stdout.write(text)
        self.pipe.write(text)
    def close(self):
        self.pipe.close()

def RandomizedTest(w, seed, keys, loops):
    random.seed(seed + 1)
    r = []
    for i in xrange(loops):
        for j in xrange(random.randint(0, len(keys))):
            w[random.choice(keys)] = random.randint(0, 0xffffffff)
        for j in xrange(random.randint(0, len(keys))):
            w.increment(random.choice(keys))
        for j in xrange(random.randint(0, len(keys))):
            del w[random.choice(keys)]
        for j in xrange(random.randint(0, len(keys))):
            r.append(w[random.choice(keys)])
        if random.randint(0, 3) == 0:
            w.clear()
        if random.randint(0, 1) == 0:
            w.compact()
    return r

if __name__ == '__main__':
    pathToExe = sys.argv[1]
    seed = int(sys.argv[2])
    keySets = [
        [0],
        range(4),
        range(10),
        range(32),
        range(100),
        [0] + [random.randint(1, 0xffffffff) for i in xrange(4)],
        [0] + [random.randint(1, 0xffffffff) for i in xrange(10)],
        [0] + [random.randint(1, 0xffffffff) for i in xrange(32)],
        [0] + [random.randint(1, 0xffffffff) for i in xrange(100)],
        [random.randint(0, 0xffffffff) for i in xrange(200)],
    ]
    for keys in keySets:
        r1 = HashTableWrapper(pathToExe).run(RandomizedTest, seed, keys, 4)
        r2 = DictionaryWrapper().run(RandomizedTest, seed, keys, 4)
        assert r1 == r2
