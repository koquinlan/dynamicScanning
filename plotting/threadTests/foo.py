import numpy as np
import matplotlib.pyplot as plt

baseline = np.loadtxt("baselineTEST.csv", delimiter=",")
runningAverage = np.loadtxt("runningAverageTEST.csv", delimiter=",")
rawData = np.loadtxt("rawDataTEST.csv", delimiter=",")
freq = np.loadtxt("freqTEST.csv", delimiter=",")
outliers = np.loadtxt("outliersTEST.csv", delimiter=",").astype(int)

# Create a figure with two subplots
fig, axs = plt.subplots(1, 2, figsize=(12, 6))

# Plot in the first subplot
axs[0].plot(freq, rawData, label="Raw Data")
axs[0].plot(freq, runningAverage, label="Running Average")
axs[0].plot(freq, baseline, label="Baseline")
axs[0].plot(freq[outliers], baseline[outliers], "o", label="Outliers", color="red")
axs[0].set_title("With TEST Appended")
axs[0].legend()

# Load data without "TEST" appended
baseline_no_test = np.loadtxt("baseline.csv", delimiter=",")
runningAverage_no_test = np.loadtxt("runningAverage.csv", delimiter=",")
rawData_no_test = np.loadtxt("rawData.csv", delimiter=",")
freq_no_test = np.loadtxt("freq.csv", delimiter=",")
outliers_no_test = np.loadtxt("outliers.csv", delimiter=",").astype(int)

# Plot in the second subplot
axs[1].plot(freq_no_test, rawData_no_test, label="Raw Data")
axs[1].plot(freq_no_test, runningAverage_no_test, label="Running Average")
axs[1].plot(freq_no_test, baseline_no_test, label="Baseline")
axs[1].plot(
    freq_no_test[outliers_no_test],
    runningAverage_no_test[outliers_no_test],
    "o",
    label="Outliers",
    color="red",
)
axs[1].set_title("Without TEST Appended")
axs[1].legend()


axs[0].set_ylim([0, np.max(runningAverage)])
axs[1].set_ylim([0, np.max(runningAverage)])
# Adjust layout to prevent overlap
plt.tight_layout()

plt.show()


print(runningAverage_no_test[round(len(runningAverage_no_test) / 2) - 1])
print(freq_no_test[round(len(runningAverage_no_test) / 2) - 1])
print(np.max(runningAverage_no_test))
