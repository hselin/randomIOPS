import subprocess
import datetime
import argparse
import sys


DEFAULT_RUN_TIME = -1
DEFAULT_DEVICE_PATH = '/dev/xvdb'
DEFAULT_NUM_THREADS = '8'
DEFAULT_IO_SIZE = '512'
DEFAULT_READ_PERCENT = '1'

class MyParser(argparse.ArgumentParser): 
	def error(self, message):
		sys.stderr.write('error: %s\n' % message)
		self.print_help()
		sys.exit(2)

if __name__ == '__main__':
	parser = MyParser()
	parser.add_argument("-rt", "--runTime", help="amount of time to run", type=int, default=DEFAULT_RUN_TIME, required=False)
	parser.add_argument("-dp", "--devPath", help="device path", type=str, default=DEFAULT_DEVICE_PATH, required=False)
	parser.add_argument("-nt", "--numThreads", help="number of threads", type=str, default=DEFAULT_NUM_THREADS, required=False)
	parser.add_argument("-iz", "--ioSize", help="size of each io", type=str, default=DEFAULT_IO_SIZE, required=False)
	parser.add_argument("-rp", "--readPercent", help="percent of read IO", type=str, default=DEFAULT_READ_PERCENT, required=False)


	args = parser.parse_args()

	print('runTime:', args.runTime)
	print('devPath:', args.devPath)
	print('numThreads:', args.numThreads)
	print('ioSize:', args.ioSize)
	print('readPercent:', args.readPercent)

	startTime = datetime.datetime.now()

	while True:
		if(args.runTime >= 0):
			delta = datetime.datetime.now() - startTime
			if(delta.total_seconds() >= args.runTime):
				break

		result = subprocess.run(['./randio', args.devPath , args.numThreads, args.ioSize, args.readPercent], stdout=subprocess.PIPE)
		delta = datetime.datetime.now() - startTime
		print('%s, %s' % (result.stdout.decode('utf-8'), delta.total_seconds()))