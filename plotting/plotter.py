import numpy as np
import matplotlib.pyplot as plt

# Load data from file
freq = np.loadtxt("freq.csv", delimiter=",")
baseline = np.loadtxt("baseline.csv", delimiter=",")
runningAverage = np.loadtxt("runningAverage.csv", delimiter=",")


# Plot the data
plt.figure()

plt.plot(freq, runningAverage, label="Raw Spectra Average")
plt.plot(freq, baseline, label="Baseline")

plt.xlabel("Freq (MHz)")
plt.ylabel("Amplitude")

plt.ylim([0, 1.1 * np.max(baseline)])

plt.grid()
plt.legend()

plt.show()
