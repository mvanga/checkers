import sys
import subprocess
import itertools

def main():
    # parse command line options
    if len(sys.argv) == 2:
        prog = sys.argv[1]
        p = subprocess.Popen("MODE=record FILE=schedule.txt LD_PRELOAD=./chess.so " + prog, shell=True)
        p.wait()
        
        with open('schedule.txt') as f:
            l = f.read().split()

        for perm in itertools.permutations(l):
            print perm
            with open('schedule2.txt', 'w') as f:
                f.write("\n".join(perm))
                p = subprocess.Popen("MODE=replay FILE=schedule2.txt LD_PRELOAD=./chess.so " + prog, shell=True)
                p.wait()

if __name__ == "__main__":
    main()
