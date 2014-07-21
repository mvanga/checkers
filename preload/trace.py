import sys
import subprocess
import itertools

def my_print(line):
	if line[0][0] == '#':
		line[0] = line[0][1:]
	if line[1] == '0':
		return ['start', '...', 'trylock()']
	elif line[1] == '1':
		return ['lock(' + line[3] + ')', '...', 'unlock(' + line[3] + ')']
	elif line[1] == '2':
		return ['...']
	elif line[1] == '3':
		return ['EXIT']

def main():
	# parse command line options
	if len(sys.argv) == 3:
		prog = sys.argv[1]
		fname = sys.argv[2]

		with open(fname) as f:
			l = [a.split() for a in f.read().strip().split('\n')]

		for line in l:
			for x in my_print(line):
				for i in xrange(15*int(line[0])):
					print ' ',
				print '[' + line[0] + ']: ' + x

		#col_width = max(len(word) for row in data for word in row) + 2  # padding
		#for line in l:
		#	addr = line[2]
		#	p = subprocess.Popen('addr2line -s -e ' + prog + ' ' + addr, shell=True)
		#	p.wait()
		


if __name__ == "__main__":
	main()
