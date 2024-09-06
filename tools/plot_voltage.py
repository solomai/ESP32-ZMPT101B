# Script is used for visualizing voltage sensor data fed to the microcontroller board.
# It can be used for debugging purposes but is not a mandatory tool.
# The data is read from a file, such as the provided example sampled_voltage.txt.
# To generate data for this file, you need to manually copy it from the output ( terminal, monitor ect )
# when the component is built with the DEBUG_EXTRA_INFO define enabled (found in the component's header file).

import matplotlib.pyplot as plt
import os
import numpy as np

def parse_data(file_path):
    """
    Parses the input data file to extract sampling frequency, number of samples, and data values.
    
    :param file_path: Path to the input file.
    :return: Tuple containing sampling frequency, number of samples, and list of data values.
    """
    with open(file_path, 'r') as file:
        # Read lines from the file
        lines = file.readlines()

    # Extract sampling frequency and number of samples from the first two lines
    sampling_freq = int(lines[0].split(': ')[1])
    sampled_expectation = int(lines[1].split(': ')[1])

    # Flatten the list of voltage values from the subsequent lines
    data_values = [float(value) for line in lines[2:] if line.strip() for value in line.split()]
    sampled = len(data_values)

    if sampled_expectation != sampled:
        print("WARNING: Data mismatch detected")

    return sampling_freq, sampled, data_values

def moving_average(data, window_size):
    """
    Applies a moving average filter to the data.
    
    :param data: List of data values.
    :param window_size: Size of the moving window.
    :return: Filtered data using moving average.
    """
    return [sum(data[i:i + window_size]) / window_size for i in range(len(data) - window_size + 1)]

def median_filter(data, window_size):
    """
    Applies a median filter to the data.
    
    :param data: List of data values.
    :param window_size: Size of the moving window.
    :return: Filtered data using median filter.
    """
    return [np.median(data[i:i + window_size]) for i in range(len(data) - window_size + 1)]

def plot_data(sampling_freq, data_values, moving_avg_values, median_values):
    """
    Plots the data values against time on a graph.
    
    :param sampling_freq: The sampling frequency in Hz.
    :param data_values: List of original voltage values in volts.
    :param moving_avg_values: List of filtered voltage values using moving average.
    :param median_values: List of filtered voltage values using median filter.
    """
    # Calculate the time for each sample in milliseconds
    time_values = [i * (1000 / sampling_freq) for i in range(len(data_values))]
    time_values_filtered = time_values[:len(moving_avg_values)]
    time_values_median = time_values[:len(median_values)]

    # Calculate statistics
    min_voltage = min(median_values)
    max_voltage = max(median_values)
    avg_voltage = (min_voltage + max_voltage) / 2

    # Create the plot
    fig, ax = plt.subplots(figsize=(10, 6))

    # Plot original data
    original_line, = ax.plot(time_values, data_values, linestyle='-', color='b', linewidth=1, label='Original Data')

    # Plot moving average data
    moving_avg_line, = ax.plot(time_values_filtered, moving_avg_values, linestyle='-', color='g', linewidth=1, label='Moving Average')

    # Plot median filter data
    median_line, = ax.plot(time_values_median, median_values, linestyle='-', color='r', linewidth=1, label='Median Filter')

    # Add horizontal lines for min, max, and average
    ax.axhline(y=min_voltage, color='r', linestyle='--', linewidth=0.75)
    ax.axhline(y=max_voltage, color='r', linestyle='--', linewidth=0.75)
    ax.axhline(y=avg_voltage, color='r', linestyle='--', linewidth=0.75)

    # Set plot title and labels
    ax.set_title(f'Voltage vs Time\nDiscretization: {sampling_freq}Hz | Data Points: {len(data_values)}')
    ax.set_xlabel('Time (ms)')
    ax.set_ylabel('Voltage (V)')

    # Add labels for horizontal lines
    ax.text(0, min_voltage, f'Min: {min_voltage:.2f}', color='r', fontsize=10, verticalalignment='bottom')
    ax.text(0, max_voltage, f'Max: {max_voltage:.2f}', color='r', fontsize=10, verticalalignment='top')
    ax.text(0, avg_voltage, f'Avg: {avg_voltage:.2f}', color='r', fontsize=10, verticalalignment='center')

    # Set Y-axis limits for the voltage range
    buffer = 0.1  # Buffer to avoid clipping at the edges
    ax.set_ylim(min_voltage - buffer, max_voltage + buffer)

    # Display the plot
    plt.grid(True)
    plt.legend()
    
    def onpick(event):
        # Check if the event is associated with a line
        if isinstance(event.artist, plt.Line2D):
            line = event.artist
            # Toggle the thickness of the line
            if line.get_linewidth() == 1:
                line.set_linewidth(3)
            else:
                line.set_linewidth(1)
            plt.draw()

    # Make lines pickable
    original_line.set_picker(True)
    moving_avg_line.set_picker(True)
    median_line.set_picker(True)
    
    fig.canvas.mpl_connect('pick_event', onpick)
    
    plt.show()

def main():
    """
    Main function to run the program. It reads data from a file and plots it.
    """
    # File path containing the data
    script_dir = os.path.dirname(os.path.abspath(__file__))
    file_path = os.path.join(script_dir, 'sampled_voltage.txt') # Update this path to your data file location

    print("Current folder: " + os.getcwd())
    print("Sampling file: " + file_path)
    
    # Parse the data from the file
    sampling_freq, sampled, data_values = parse_data(file_path)

    # Apply filters
    window_size = 10  # Example window size; you can adjust this
    moving_avg_values = moving_average(data_values, window_size)
    median_values = median_filter(data_values, window_size)

    # Plot the data on a graph
    plot_data(sampling_freq, data_values, moving_avg_values, median_values)

if __name__ == "__main__":
    main()
