import subprocess
import datetime

if __name__ == '__main__':
	startTime = datetime.datetime.now()
	while True:
		result = subprocess.run(['./randio', '/dev/xvdp' , '16' , '512' , '1'], stdout=subprocess.PIPE)
		delta = datetime.datetime.now() - startTime
		print('%s, %s' % (result.stdout.decode('utf-8'), delta.total_seconds()))
