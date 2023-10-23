import numpy as np
import matplotlib.pyplot as plt

rawData = np.loadtxt("baseline/rawData.csv", delimiter=",")

baseline = np.loadtxt("baseline/baseline.csv", delimiter=",")
runningAverage = np.loadtxt("baseline/runningAverage.csv", delimiter=",")
freq = np.loadtxt("baseline/freq.csv", delimiter=",")
outliers = np.loadtxt("baseline/outliers.csv", delimiter=",").astype(int)


# Plot in the first subplot
plt.plot(freq, rawData, label="Raw Data")
plt.plot(freq, runningAverage, label="Running Average")
plt.plot(freq, baseline, label="Baseline")
plt.plot(freq[outliers], baseline[outliers], "o", label="Outliers", color="red")

plt.ylim([0, 1.3 * np.max(runningAverage)])

plt.legend()

plt.show()
