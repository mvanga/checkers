import sys
import subprocess
import itertools

def my_print(prog, line):
    prefix = None
    if len(line) > 2:
        addr = line[2]
        p = subprocess.Popen('addr2line -s -f -e ' + prog + ' ' + addr, shell=True, stdout=subprocess.PIPE)
        output = p.communicate()[0].split("\n")
        fun = output[0]
        linenumber = output[1].split(":")[1]
        prefix = "===%s(),l.%s===" % (fun, linenumber)

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

def main():
	# parse command line options
	if len(sys.argv) == 3:
		prog = sys.argv[1]
		fname = sys.argv[2]

		with open(fname) as f:
			l = [a.split() for a in f.read().strip().split('\n')]

		for line in l:
			for x in my_print(prog, line):
				for i in xrange(15*int(line[0])):
					print ' ',
				print ('[' + line[0] + ']: ' if x[0] != "=" else "") + x

		#col_width = max(len(word) for row in data for word in row) + 2  # padding
		#for line in l:
		#	addr = line[2]
		#	p = subprocess.Popen('addr2line -s -e ' + prog + ' ' + addr, shell=True)
		#	p.wait()
		


if __name__ == "__main__":
	main()
