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


# Plot processed spectra with distribution for comparison
std = np.std(processedSpectrum)
mu = np.mean(processedSpectrum)

min_val = np.amin(processedSpectrum)
max_val = np.amax(processedSpectrum)

x_axis = np.arange(min_val - std, max_val + std, 0.0001)
processed_gaussian = norm.pdf(x_axis, mu, std)


f, (a1, a2) = plt.subplots(1, 2, gridspec_kw={"width_ratios": [3, 1]})


a1.plot(trimmedFreqAxis, processedSpectrum)

a1.set_xlabel("Detuning (MHz)")
a1.set_ylabel("power [a.u]")
a1.set_title("Processed Spectrum")


vert_hist = np.histogram(processedSpectrum, bins=50, density=True)
binCenters = (vert_hist[1][:-1] + vert_hist[1][1:]) / 2
a2.plot(processed_gaussian, x_axis, "r")
a2.plot(vert_hist[0], binCenters, ".g")

a2.set_ylim([-6 * std, 6 * std])

f.set_size_inches(12.5, 5.5, forward=True)
f.tight_layout()


# Plot raw spectrum and baseline
plt.figure()
plt.plot(freqAxis, rawSpectrum, label="Raw Spectrum")
plt.plot(freqAxis, fullBaseline, label="Full Baseline")

plt.show()
