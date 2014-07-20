import sys
import subprocess

def main():
    # parse command line options
    if len(sys.argv) == 2:
        prog = sys.argv[1]
        subprocess.call("MODE=record LD_PRELOAD=./chess.so " + prog, shell=True)
    
    
if __name__ == "__main__":
    main()
