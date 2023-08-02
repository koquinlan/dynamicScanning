import numpy as np
import matplotlib.pyplot as plt

# Load data from file
freq = np.loadtxt("freq.csv", delimiter=",")
vis = np.loadtxt("visData/-0.001111.csv", delimiter=",")
bg = np.loadtxt("visData/bg_-0.001111.csv", delimiter=",")


# Plot the data
plt.figure()

plt.plot(freq, vis, label="Probe On")
plt.plot(freq, bg, label="Background")

plt.xlabel("Freq (MHz)")
plt.ylabel("Amplitude")

plt.ylim([0, 1.1 * np.partition(vis, -10)[-10]])  # 10th largest value

plt.grid()
plt.legend()


plt.figure()

foo = (vis - bg) / bg
plt.plot(freq, foo)

plt.xlabel("Freq (MHz)")
plt.ylabel("Amplitude")

# plt.ylim([0, 1.1 * np.partition(foo, -10)[-10]])  # 10th largest value

plt.grid()

plt.show()
