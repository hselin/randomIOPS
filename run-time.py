import subprocess
import datetime
import argparse
import sys

DEFAULT_RUN_TIME = -1

class MyParser(argparse.ArgumentParser): 
	def error(self, message):
		sys.stderr.write('error: %s\n' % message)
		self.print_help()
		sys.exit(2)

if __name__ == '__main__':
	parser = MyParser()
	parser.add_argument("-rt", "--runTime", help="amount of time to run", type=int, default=DEFAULT_RUN_TIME, required=False)

	args = parser.parse_args()

	print('runTime:', args.runTime)

	startTime = datetime.datetime.now()

	while True:
		if(args.runTime >= 0):
			delta = datetime.datetime.now() - startTime
			if(delta.total_seconds() >= args.runTime):
				break

		result = subprocess.run(['./randio', '/dev/sdb' , '8' , '512' , '1'], stdout=subprocess.PIPE)		
		delta = datetime.datetime.now() - startTime
		print('%s, %s' % (result.stdout.decode('utf-8'), delta.total_seconds()))
