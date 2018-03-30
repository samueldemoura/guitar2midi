import sys
import numpy as np
import matplotlib.pyplot as plt

data = sys.stdin.readlines()

samples = []
for l in data:
	try:
		samples.append(float(l))
	except ValueError:
		pass

plot = plt.plot(samples)

plt.xscale('log')
#plt.yscale('log')

plt.show()
