import numpy as np
import matplotlib.pyplot as plt

# Load data from file
freq = np.loadtxt("freq.csv", delimiter=",")
baseline = np.loadtxt("baseline.csv", delimiter=",")
runningAverage = np.loadtxt("runningAverage.csv", delimiter=",")
outliers = np.loadtxt("outliers.csv", delimiter=",").astype(int)


# Get processed spectrum
processed = (runningAverage / baseline) - 1


print(np.std(processed))
print(np.mean(processed))

# Plot the data
plt.figure()

plt.plot(freq, runningAverage, label="Raw Spectra Average")
plt.plot(freq, baseline, label="Baseline")
plt.plot(freq[outliers], runningAverage[outliers], "o", label="Outliers", color="red")

plt.xlabel("Freq (MHz)")
plt.ylabel("Amplitude")

plt.ylim([0, 1.1 * np.max(baseline)])

plt.grid()
plt.legend()

plt.figure()
plt.plot(freq, processed, label="Raw Spectra Average")
plt.plot(freq[outliers], processed[outliers], "o", label="Outliers", color="red")

plt.show()
