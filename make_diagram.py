import argparse
import pandas as pd
import matplotlib.pyplot as plt
import datetime

def select_columns(data):
    print("Available columns:")
    for index, column in enumerate(data.columns, start=1):
        print(f"{index}. {column}")

    selected_columns = input(
        "Enter the column numbers you want to display, separated by commas (e.g., 1,2,3) or type 'all' to select all columns: ")

    if selected_columns.strip().lower() == 'all':
        return data
    else:
        selected_indices = [
            int(x) - 1 for x in selected_columns.split(',')]  # Adjust indices
        return data.iloc[:, selected_indices]

if __name__ == "__main__":

    # 1. Parse command line arguments
    parser = argparse.ArgumentParser(
        description="Visualize data from a CSV file")
    parser.add_argument("-f", "--file_path",
                        help="Path to the CSV file", default=None)
    args = parser.parse_args()

    # If no file path is provided as an argument, ask for the file name manually
    if args.file_path is None:
        file_name = input("Enter the name of the CSV file: ")
    else:
        file_name = args.file_path

    # 2. Parse the file
    data = pd.read_csv(args.file_path)

    # 3. Convert Unix timestamp to a more user-friendly format
    data['Time'] = data['Time'].apply(
        lambda x: datetime.datetime.fromtimestamp(int(x)))

    # 4. Set the date as index
    data.set_index('Time', inplace=True)

    # 6. Select columns to display
    data = select_columns(data)

    # 5. Create a diagram using the first column (date) as the x axis and selected columns on the y axis
    fig, ax = plt.subplots(figsize=(10, 5))
    data.plot(ax=ax)
    plt.xlabel('Time')
    plt.ylabel('Values')

    # Change the date format in the title
    start_date = data.index[0].strftime('%Y/%m/%d')
    end_date = data.index[-1].strftime('%Y/%m/%d')

    if start_date == end_date:
        plt.title(f'CSV Data Visualization for {start_date}')
    else:
        plt.title(f'CSV Data Visualization for {start_date} - {end_date}')

    plt.grid(True)

    # Set the legend position to the best location based on the plotted data
    ax.legend(loc='best')

    plt.show()
