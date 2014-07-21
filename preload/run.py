import sys
import subprocess
import itertools
from random import randint
from math import factorial

def random_permutation(perm):
    while True:
        for i in xrange(len(perm)):
            j = randint(0, i)
            perm[i] = perm[j]
            perm[j] = i
        yield perm

generator = {"random": random_permutation, "exhaustive": itertools.permutations}

def min_n_fact_m(n, m):
    prod = 1
    for i in xrange(2, m+1):
        if n < prod:
            return n
        prod *= i
    return min(n, prod)

gen = generator["random"]
maxrounds = 1000

def main():
    # parse command line options
    if len(sys.argv) == 2:
        prog = sys.argv[1]
        record_cmd = "MODE=record FILE=schedule.txt LD_PRELOAD=./chess.so " + prog
        replay_cmd = "MODE=replay FILE=schedule2.txt LD_PRELOAD=./chess.so " + prog

        p = subprocess.Popen(record_cmd, shell=True)
        p.wait()
        
        with open('schedule.txt') as f:
            l = [a.split() for a in f.read().strip().split('\n')]

        perm = range(len(l)) # Initial permutation (identity)

        count = 1
        limit = min_n_fact_m(maxrounds, len(l))
        for perm in gen(perm):
            if count > limit:
                break
            new_l = [l[x] for x in perm]
            print count, [x[0] for x in new_l]
            with open('schedule2.txt', 'w') as f:
                for line in new_l:
                    f.write(" ".join(line) + "\n")
            p = subprocess.Popen(replay_cmd, shell=True)
            p.wait()
            count += 1

if __name__ == "__main__":
    main()
