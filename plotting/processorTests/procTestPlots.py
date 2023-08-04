import numpy as np
import matplotlib.pyplot as plt

# Load data from file
freqAxis = np.loadtxt("freqAxis.csv", delimiter=",")
trimmedFreqAxis = np.loadtxt("trimmedFreqAxis.csv", delimiter=",")
rawSpectrum = np.loadtxt("rawSpectrum.csv", delimiter=",")
fullBaseline = np.loadtxt("fullBaseline.csv", delimiter=",")
processedBaseline = np.loadtxt("processedBaseline.csv", delimiter=",")
processedSpectrum = np.loadtxt("processedSpectrum.csv", delimiter=",")

plt.figure()
plt.hist(processedSpectrum, bins=100, label="Processed Spectrum")

plt.figure()
plt.plot(trimmedFreqAxis, processedSpectrum, label="Processed Spectrum")

plt.figure()
plt.plot(freqAxis, rawSpectrum, label="Raw Spectrum")
plt.plot(freqAxis, fullBaseline, label="Full Baseline")

plt.show()
