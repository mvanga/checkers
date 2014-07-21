import sys
import subprocess
import itertools

def main():
    # parse command line options
    if len(sys.argv) == 2:
        prog = sys.argv[1]
        record_cmd = "MODE=record FILE=schedule.txt LD_PRELOAD=./chess.so " + prog
        replay_cmd = "MODE=replay FILE=schedule2.txt LD_PRELOAD=./chess.so " + prog

        p = subprocess.Popen(record_cmd, shell=True)
        p.wait()
        
        with open('schedule.txt') as f:
            l = [a.split()[0] for a in f.read().strip().split('\n')]

        for perm in itertools.permutations(l):
            print perm
            with open('schedule2.txt', 'w') as f:
                f.write("\n".join(perm))
            p = subprocess.Popen(replay_cmd, shell=True)
            p.wait()

if __name__ == "__main__":
    main()
