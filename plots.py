import pandas as pd
import matplotlib.pyplot as plt

data = pd.read_csv("results.csv")
# clean up filename
data["Filename"] = data["Filename"].str.extract(r"^ER_(10+_0.[257][50])_.*", expand=False)
# group by different graph parameters found
grouped = data.groupby("Filename")


width_columns = ["Width Min-Degree", "Width Min-Fill-in", "Width MCS"]
time_columns = ["Time Min-Degree", "Time Min-Fill-in", "Time MCS"]

fig, ax = plt.subplots(2,grouped.ngroups, figsize=(24,20))
plt.suptitle("Result of Elimination Ordering benchmark", fontsize="xx-large")
plt.text(0.5, 0.96, "Each plot is named after the aggregated Erlos-Renyi graphs (e.g. 1000_0.25: N = 1000, p = 0.25)",
          fontsize="x-large", transform=fig.transFigure, horizontalalignment="center")
keys = grouped.groups.keys()

i = 0
for key in keys:
    ax_width = grouped.get_group(key)[width_columns].plot.box(ax=ax[0][i])
    ax_width.set_xticks(ax_width.get_xticks(), ax_width.get_xticklabels(), rotation=45, ha='right')
    ax_width.set_title(key)
    ax_time = grouped.get_group(key)[time_columns].plot.box(ax=ax[1][i])
    ax_time.set_xticks(ax_time.get_xticks(), ax_time.get_xticklabels(), rotation=45, ha='right')
    ax_time.set_title(key)
    i = i+1

plt.savefig("plot.png")