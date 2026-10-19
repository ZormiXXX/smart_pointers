import csv
import matplotlib.pyplot as plt

with open("benchmark.csv", newline="") as file:
    rows = list(csv.DictReader(file))

n = [int(row["N"]) for row in rows]
fig, (time_plot, memory_plot) = plt.subplots(2, 1, figsize=(8, 9))

for key, label in (("raw_ms", "Raw pointers"), ("unq_ms", "UnqPtr"), ("shrd_ms", "ShrdPtr")):
    time_plot.plot(n, [float(row[key]) for row in rows], marker="o", label=label)
time_plot.set_ylabel("Time, ms")
time_plot.set_title("Management time")
time_plot.legend()
time_plot.grid(True, which="both", alpha=0.3)

for key, label in (("raw_bytes", "Raw pointers"), ("unq_bytes", "UnqPtr"), ("shrd_bytes", "ShrdPtr")):
    memory_plot.plot(n, [int(row[key]) for row in rows], marker="o", label=label)
memory_plot.set_xlabel("Number of objects")
memory_plot.set_ylabel("Useful memory, bytes")
memory_plot.set_title("Estimated requested memory")
memory_plot.legend()
memory_plot.grid(True, which="both", alpha=0.3)

for axis in (time_plot, memory_plot):
    axis.set_xscale("log")
    axis.set_yscale("log")

fig.tight_layout()
fig.savefig("benchmark.png", dpi=160)
print("Benchmark graph saved to benchmark.png")
