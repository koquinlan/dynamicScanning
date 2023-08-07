import numpy as np
import matplotlib.pyplot as plt

# to produce the normal pdf for plots
from scipy.stats import norm


# Load data from file
freqAxis = np.loadtxt("freqAxis.csv", delimiter=",")
trimmedFreqAxis = np.loadtxt("trimmedFreqAxis.csv", delimiter=",")
rawSpectrum = np.loadtxt("rawSpectrum.csv", delimiter=",")
fullBaseline = np.loadtxt("fullBaseline.csv", delimiter=",")

processedBaseline = np.loadtxt("processedBaseline.csv", delimiter=",")
processedSpectrum = np.loadtxt("processedSpectrum.csv", delimiter=",")

rescaledSpectrum = np.loadtxt("rescaledSpectrum.csv", delimiter=",")


# Plot processed spectra with distribution for comparison
std = np.std(processedSpectrum[0])
mu = np.mean(processedSpectrum[0])

minVal = np.amin(processedSpectrum[0])
maxVal = np.amax(processedSpectrum[0])

xAxis = np.arange(minVal - std, maxVal + std, 0.0001)
processedGaussian = norm.pdf(xAxis, mu, std)


f, (a1, a2) = plt.subplots(1, 2, gridspec_kw={"width_ratios": [3, 1]})


a1.plot(processedSpectrum[1], processedSpectrum[0])

a1.set_xlabel("Detuning (MHz)")
a1.set_ylabel("power [a.u]")
a1.set_title("Processed Spectrum")


vertHist = np.histogram(processedSpectrum[0], bins=50, density=True)
binCenters = (vertHist[1][:-1] + vertHist[1][1:]) / 2
a2.plot(processedGaussian, xAxis, "r")
a2.plot(vertHist[0], binCenters, ".g")

a2.set_ylim([-6 * std, 6 * std])

f.set_size_inches(12.5, 5.5, forward=True)
f.tight_layout()


# Plot raw spectrum and baseline
plt.figure()
plt.plot(rawSpectrum[1], rawSpectrum[0], label="Raw Spectrum")
plt.plot(freqAxis, fullBaseline, label="Full Baseline")

plt.show()
