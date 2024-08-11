import pandas as pd
import matplotlib.pyplot as plt
import os

# Define the directory where your CSV files are located
csv_directory = '../delays'  # Change this to your directory

# List of CSV filenames to process
csv_filenames = [
    'BINANCE:BTCUSDT_delays.csv',
    'BINANCE:ETHUSDT_delays.csv'#,
   #'NVDA_delays.csv',
   #'GOOGL_delays.csv' 
]

# Iterate over each CSV file
for filename in csv_filenames:
    file_path = os.path.join(csv_directory, filename)
    
    # Check if the file exists
    if not os.path.isfile(file_path):
        print(f"File not found: {file_path}")
        continue
    
    # Read the CSV file into a DataFrame
    df = pd.read_csv(file_path, header=None, names=['Time (ns)'])
    
    # Plot the data
    plt.figure()
    plt.plot(df.index, df['Time (ns)'], marker='o', linestyle='-', color='b')
    plt.title(f"Plot of {filename}")
    plt.xlabel('Index')
    plt.ylabel('Time (seconds)')
    plt.grid(True)
    
    # Save the plot as an image
    plot_filename = os.path.splitext(filename)[0] + '_plot.png'
    plt.savefig(plot_filename)
    plt.close()
    
    print(f"Plot saved as {plot_filename}")

print("All plots generated.")
