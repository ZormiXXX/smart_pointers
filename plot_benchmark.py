import csv
import matplotlib.pyplot as plt


with open("benchmark.csv", newline="", encoding="utf-8") as source:
    rows = list(csv.DictReader(source))

methods = ["raw", "UnqPtr", "ShrdPtr", "std::unique_ptr", "std::shared_ptr"]
sizes = sorted({int(row["N"]) for row in rows})
fig, axes = plt.subplots(2, 2, figsize=(12, 9))
plots = [
    (axes[0, 0], "create_ms", "Создание, мс"),
    (axes[0, 1], "access_ms", "Доступ, мс"),
    (axes[1, 0], "destroy_ms", "Удаление, мс"),
]

for axis, column, title in plots:
    for method in methods:
        values = {
            int(row["N"]): float(row[column])
            for row in rows
            if row["method"] == method
        }
        axis.plot(sizes, [values[size] for size in sizes], marker="o", label=method)
    axis.set_xscale("log")
    axis.set_yscale("log")
    axis.set_title(title)
    axis.set_xlabel("Количество объектов")
    axis.grid(True, which="both", alpha=0.3)

memory_axis = axes[1, 1]
for method in methods:
    values = {
        int(row["N"]): int(row["peak_usable_bytes"])
        for row in rows
        if row["method"] == method
    }
    memory_axis.plot(sizes, [values[size] for size in sizes], marker="o", label=method)
memory_axis.set_xscale("log")
memory_axis.set_yscale("log")
memory_axis.set_title("Пиковый полезный размер блоков, байт")
memory_axis.set_xlabel("Количество объектов")
memory_axis.grid(True, which="both", alpha=0.3)

for axis in axes.flat:
    axis.legend(fontsize="small")

fig.tight_layout()
fig.savefig("benchmark.png", dpi=160)
print("График сохранён в benchmark.png")
