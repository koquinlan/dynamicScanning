import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import os

# Get a list of all .csv files in the "data" folder
scanWIP_folder = "scanProgress"
scan_files = [f for f in os.listdir(scanWIP_folder) if f.endswith(".csv")]

# Initialize a list to store the data
scan_data = []

# Load data from each .csv file and store it in the scan_data list
for csv_file in scan_files:
    file_path = os.path.join(scanWIP_folder, csv_file)
    data = np.loadtxt(file_path, delimiter=",")

    # Assuming data is a 2D array where each row represents a power spectrum
    spectra_list = data.tolist()  # Convert the numpy array to a list of lists

    scan_data.append(spectra_list)


fig, ax = plt.subplots()
ax.set_xlabel("X-axis Label")
ax.set_ylabel("Y-axis Label")
(line,) = ax.plot([], [], lw=2)

# Initialize variables
i = 0
j = 0


# Set x-axis and y-axis limits (plot range)
x_max = max(scan_data[i][0])
y_max = 1e-4

target = 6.5e-5


# Define the update function for the animation
def update(frame):
    global i, j, x_max, y_max, target
    ax.clear()

    # Plot the data
    x = scan_data[i][0]
    y = scan_data[i][j]

    (line,) = ax.plot(x, y)
    ax.axhline(y=target, color="red", linestyle="--")
    ax.set_xlim(0, 5)
    ax.set_ylim(0, scan_data[i][j][round(len(scan_data[i][j]) / 2) + 1] * 5)

    # Set the title for each frame
    ax.set_title(f"Step {i}, Spectrum {j}")

    # Increment j and i as needed
    j += 1
    if j >= len(scan_data[i]):
        j = 1
        i += 1


# Create an animation
ani = FuncAnimation(
    fig,
    update,
    frames=sum(len(sublist) for sublist in scan_data) - len(scan_data),
    repeat=False,
    interval=0.01,
)

ani.save("scan_animation.gif")
