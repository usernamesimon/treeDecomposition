import pandas as pd
import matplotlib.pyplot as plt

data = pd.read_csv("results_libTW_graphs_MCS_max.csv")
data_min = pd.read_csv("results_libTW_graphs_MCS_min.csv")

data.rename(columns={"Width MCS":"Width MCS-max"}, inplace=True)
data["Width MCS-min"] = data_min["Width MCS"]
# clean up filename

width_columns = ["Filename", "Width Min-Degree", "Width Min-Fill-in", "Width MCS-max", "Width MCS-min"]
time_columns = ["Filename", "Time Min-Degree", "Time Min-Fill-in", "Time MCS"]

width = data[width_columns]
time = data[time_columns]
width.set_index("Filename", inplace=True)
time.set_index("Filename", inplace=True)

plt.figure()

width.plot(kind="bar", title="Calculated widths", xlabel="File", ylabel="treewidth", figsize=(20,16))
#time.plot(ax=ax2)

plt.savefig("libTW_plot.png")