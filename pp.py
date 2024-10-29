import sexpdata
import sys
for l in sys.stdin.readlines():
	if l.startswith('('):
		print(sexpdata.dumps(sexpdata.loads(l), pretty_print=True))
