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
freqAxis = np.loadtxt("freqAxis.csv", delimiter=",")
trimmedFreqAxis = np.loadtxt("trimmedFreqAxis.csv", delimiter=",")
rawSpectrum = np.loadtxt("rawSpectrum.csv", delimiter=",")
fullBaseline = np.loadtxt("fullBaseline.csv", delimiter=",")

processedBaseline = np.loadtxt("processedBaseline.csv", delimiter=",")
processedSpectrum = np.loadtxt("processedSpectrum.csv", delimiter=",")

rescaledSpectrum = np.loadtxt("rescaledSpectrum.csv", delimiter=",")

combinedSpectrum = np.loadtxt("combinedSpectrum.csv", delimiter=",")


# Plot processed spectra with distribution for comparison
f, (a1, a2) = plt.subplots(1, 2, gridspec_kw={"width_ratios": [3, 1]})

a1.plot(processedSpectrum[1], processedSpectrum[0])

a1.set_xlabel("Detuning (MHz)")
a1.set_ylabel("power [a.u]")
a1.set_title("Processed Spectrum")

plotVerticalHistogram(processedSpectrum[0], a2)

f.set_size_inches(12.5, 5.5, forward=True)
f.tight_layout()


# Plot raw spectrum and baseline
plt.figure()
plt.plot(rawSpectrum[1], rawSpectrum[0], label="Raw Spectrum")
plt.plot(freqAxis, fullBaseline, label="Full Baseline")


# Plot rescaled spectrum
plt.figure()
plt.plot(rescaledSpectrum[1], rescaledSpectrum[0], label="Rescaled Spectrum")


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
