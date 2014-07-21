import sys
import subprocess
import itertools
from random import randint
from math import factorial
import sys
import subprocess
import itertools

def my_print(prog, line):
    prefix = None
    #if len(line) > 2:
    #    addr = line[2]
    #    p = subprocess.Popen('addr2line -s -f -e ' + prog + ' ' + addr, shell=True, stdout=subprocess.PIPE)
    #    output = p.communicate()[0].split("\n")
    #    fun = output[0]
    #    linenumber = output[1].split(":")[1]
    #    prefix = "===%s(),l.%s===" % (fun, linenumber)

    if line[1] == '0':
        ret = ['start', '...', 'trylock()']
    elif line[1] == '1':
        ret = ['lock(' + line[3] + ')', '...', 'unlock(' + line[3] + ')']
    elif line[1] == '2':
        ret = ['...']
    elif line[1] == '3':
        ret = ['EXIT']
    
    if prefix:
        return [prefix] + ret
    else:
        return ret

def get_trace(prog, fname):
	# parse command line options
	ret = ''
	with open(fname) as f:
		l = [a.split() for a in f.read().strip().split('\n')]

	for line in l:
		for x in my_print(prog, line):
			for i in xrange(20*int(line[0])):
				ret += ' '
			ret += ('[' + line[0] + ']: ' if x[0] != "=" else "") + x + '\n'
	return ret
		
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
        record_cmd = "MODE=record TRACE_FILE=trace.txt REPLAY_FILE=schedule.txt LD_PRELOAD=./chess.so " + prog
        replay_cmd = "MODE=replay TRACE_FILE=trace.txt REPLAY_FILE=schedule.txt LD_PRELOAD=./chess.so " + prog
        trace_cmd = "python trace.py " + prog + " trace.txt"

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
            with open('schedule.txt', 'w') as f:
                for line in new_l:
                    f.write(" ".join(line) + "\n")
            p = subprocess.Popen(replay_cmd, shell=True)
            p.wait()
            if p.returncode != 0:
                print "Assertion failed. Bug found!!!\n"
                print get_trace(prog, 'trace.txt')
                print "\n"
                print "Continue? (Y/n): ",
                choice = raw_input()
                if not choice: continue
                if choice in ['n', 'N']: break
            count += 1

if __name__ == "__main__":
    main()
