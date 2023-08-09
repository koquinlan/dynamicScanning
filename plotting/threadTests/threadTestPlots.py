import numpy as np
import matplotlib.pyplot as plt

# to produce the normal pdf for plots
from scipy.stats import norm


def plotVerticalHistogram(data, axis, ylim=6):
    std = np.std(data)
    mu = np.mean(data)

    minVal = np.amin(data)
    maxVal = np.amax(data)

    xAxis = np.arange(minVal - std, maxVal + std, 0.0001)
    processedGaussian = norm.pdf(xAxis, mu, std)

    vertHist = np.histogram(data, bins=50, density=True)
    binCenters = (vertHist[1][:-1] + vertHist[1][1:]) / 2

    axis.plot(processedGaussian, xAxis, "r")
    axis.plot(vertHist[0], binCenters, ".g")
    axis.set_ylim([-ylim * std, ylim * std])

    axis.legend(["std = {std:.3f} \n mu = {mu:.3f}".format(std=std, mu=mu)])


# Load data from file
freq = np.loadtxt("freq.csv", delimiter=",")
outliers = np.loadtxt("outliers.csv", delimiter=",").astype(int)

baseline = np.loadtxt("baseline.csv", delimiter=",")
runningAverage = np.loadtxt("runningAverage.csv", delimiter=",")

rawSpectrum = np.loadtxt("rawSpectrum.csv", delimiter=",")
processedSpectrum = np.loadtxt("processedSpectrum.csv", delimiter=",")
# processedBaseline = np.loadtxt("processedBaseline.csv", delimiter=",")

combinedSpectrum = np.loadtxt("combinedSpectrum.csv", delimiter=",")


# Get processed spectrum
processed = (runningAverage / baseline) - 1

# Plot the data
plt.figure()

plt.plot(rawSpectrum[1], rawSpectrum[0], label="Raw data")

plt.plot(freq, runningAverage, label="Raw Spectra Average")
plt.plot(freq, baseline, label="Baseline")
plt.plot(freq[outliers], runningAverage[outliers], "o", label="Outliers", color="red")

plt.xlabel("Freq (MHz)")
plt.ylabel("Amplitude")

plt.ylim([0, 1.3 * np.max(baseline)])

plt.grid()
plt.legend()

plt.figure()
plt.plot(freq, processed, label="Proc w/o Residuals")
plt.plot(processedSpectrum[1], processedSpectrum[0], label="Proc data")
# plt.plot(freq, processedData, label="Proc w/ Residuals")
# plt.plot(freq, processedBaseline - 1, label="Residuals", color="red")
# plt.plot(freq[outliers], processed[outliers], "o", label="Outliers", color="red")


# Plot combined spectrum with distribution for comparison
f = plt.figure()

plt.subplot(2, 3, 1)
plt.plot(combinedSpectrum[1], combinedSpectrum[0])
plt.subplot(2, 3, 4)
plt.plot(combinedSpectrum[1], combinedSpectrum[2], "y")

plt.subplot(1, 3, 2)
plt.plot(combinedSpectrum[1], combinedSpectrum[0] / combinedSpectrum[2])
plt.ylim([-7.5, 7.5])

plt.subplot(1, 6, 5)
plotVerticalHistogram(combinedSpectrum[0] / combinedSpectrum[2], plt.gca(), 7.5)


f.set_size_inches(12.5, 7, forward=True)
f.tight_layout()


plt.show()
