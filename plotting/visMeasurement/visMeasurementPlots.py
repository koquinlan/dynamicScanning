import os
import re
import numpy as np
import matplotlib.pyplot as plt


def extract_number(filename):
    # Extract all decimal and negative numbers from the filename
    matches = re.findall(r"-?\d+\.\d+|-?\d+", filename)
    return float(matches[0]) if matches else None


def sort_files_by_number(file_list):
    return sorted(file_list, key=extract_number)


####  COLLECT PROBE AND BACKGROUND DATA FROM CSV  ####
dataFolder = "visData"

### PROBE DATA ###
probeDataFiles = [
    f for f in os.listdir(dataFolder) if f.endswith(".csv") and not f.startswith("bg_")
]
probeDataFiles = sort_files_by_number(probeDataFiles)
# probeDataFiles = np.concatenate((probeDataFiles[60:71], probeDataFiles[130:141]))

probeData = []
for probe in probeDataFiles:
    probePath = os.path.join(dataFolder, probe)
    probeData.append(np.loadtxt(probePath, delimiter=","))

probeData = np.vstack(probeData)  # Combine arrays into a single numpy array

### BACKGROUND DATA ###
bgDataFiles = [
    f for f in os.listdir(dataFolder) if f.startswith("bg_") and f.endswith(".csv")
]
bgDataFiles = sort_files_by_number(bgDataFiles)
# bgDataFiles = np.concatenate((bgDataFiles[60:71], bgDataFiles[130:141]))

bgData = []
for bg in bgDataFiles:
    bgPath = os.path.join(dataFolder, bg)
    bgData.append(np.loadtxt(bgPath, delimiter=","))

bgData = np.vstack(bgData)  # Combine arrays into a single numpy array

### OTHER DATA ###
probeFreqs = np.array([extract_number(filename) for filename in probeDataFiles])
freq = np.loadtxt("visFreq.csv", delimiter=",")
trueProbeFreqs = np.loadtxt("trueProbeFreqs.csv", delimiter=",")
visCurve = np.loadtxt("visCurve.csv", delimiter=",")

# trueProbeFreqs = np.concatenate((trueProbeFreqs[60:71], trueProbeFreqs[130:141]))
# visCurve = np.concatenate((visCurve[60:71], visCurve[130:141]))


####  PLOT PROBE AND BACKGROUND DATA  ####
plt.figure()

for probe, probeFreq in zip(probeData, probeFreqs):
    plt.plot(freq, probe)
    plt.plot(probeFreq, 0, "o", color="red")

plt.ylim(
    [0, 1.1 * np.partition(probeData[round(len(probeData) / 2)], -10)[-10]]
)  # 10th largest value


plt.figure()
plt.plot(trueProbeFreqs, visCurve, "o", color="red")
for bg, probe in zip(bgData, probeData):
    plt.plot(freq, (probe - bg) / bg)

# for probeFreq in probeFreqs:
#     plt.axvline(x=probeFreq, color="lightgray", linestyle="--", linewidth=1)

plt.show()
