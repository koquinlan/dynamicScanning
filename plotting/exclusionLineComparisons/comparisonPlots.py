import numpy as np
import matplotlib.pyplot as plt
import os

# Get a list of all .csv files in the "data" folder
metric_folder = "metrics"
metric_files = [f for f in os.listdir(metric_folder) if f.endswith(".csv")]

# Load data from each .csv file and store it in the data_arrays list
metric_arrays = []
for csv_file in metric_files:
    file_path = os.path.join(metric_folder, csv_file)
    data = np.loadtxt(file_path, delimiter=",")
    metric_arrays.append(data)


# Get a list of all .csv files in the "data" folder
data_folder = "data"
csv_files = [f for f in os.listdir(data_folder) if f.endswith(".csv")]

# Load data from each .csv file and store it in the data_arrays list
data_arrays = []
for csv_file in csv_files:
    file_path = os.path.join(data_folder, csv_file)
    data = np.loadtxt(file_path, delimiter=",")
    data_arrays.append(data)


print(metric_arrays)


# Plot each data array in data_arrays
target = 4.2e-5
plt.figure()

for i, data in enumerate(data_arrays):
    label = f"Exclusion Line {i + 1}"
    plt.plot(data[1], data[0], label=label)
plt.axhline(y=target, color="r", linestyle="-", label="Target Exclusion")

plt.xlim([-0.1, 50.1])
plt.ylim([0, 1.3 * target])

plt.legend()
plt.show()
