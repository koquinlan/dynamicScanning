import os
import re
import numpy as np
import matplotlib.pyplot as plt

# foo = [
#     7.45047,
#     7.45082,
#     7.45116,
#     7.45151,
#     7.45185,
#     7.4522,
#     7.45254,
#     7.45289,
#     7.45323,
#     7.45357,
#     7.45392,
#     7.45426,
#     7.45461,
#     7.45495,
#     7.4553,
#     7.45564,
#     7.45599,
#     7.45633,
#     7.45668,
#     7.45702,
#     7.45737,
#     7.45771,
#     7.45806,
#     7.4584,
#     7.45875,
#     7.45909,
#     7.45944,
#     7.45978,
#     7.46013,
#     7.46047,
# ]

# plt.plot(foo, np.zeros(len(foo)), "o", color="red")
# plt.show()


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

bgData = []
for bg in bgDataFiles:
    bgPath = os.path.join(dataFolder, bg)
    bgData.append(np.loadtxt(bgPath, delimiter=","))

bgData = np.vstack(bgData)  # Combine arrays into a single numpy array

### OTHER DATA ###
probeFreqs = np.array([extract_number(filename) for filename in probeDataFiles])
freq = np.loadtxt("visFreq.csv", delimiter=",")


####  PLOT PROBE AND BACKGROUND DATA  ####
plt.figure()

for probe, probeFreq in zip(probeData, probeFreqs):
    plt.plot(freq, probe)
    plt.plot(probeFreq, 0, "o", color="red")

# plt.ylim([0, 1.1 * np.partition(probeData[0], -10)[-10]])  # 10th largest value


plt.figure()

for bg, probe in zip(bgData, probeData):
    plt.plot(freq, (probe - bg) / bg)

# for probeFreq in probeFreqs:
#     plt.axvline(x=probeFreq, color="lightgray", linestyle="--", linewidth=1)

plt.show()
