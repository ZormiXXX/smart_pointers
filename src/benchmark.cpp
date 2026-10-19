#include "benchmark.hpp"
#include "smart_pointers.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <utility>

namespace {

struct Item {
    int value;
    explicit Item(int value) : value(value) {}
};

template <class Function> double measure(std::size_t count, Function &&function) {
    double best = std::numeric_limits<double>::max();
    for (int trial = 0; trial < 3; ++trial) {
        const auto start = std::chrono::steady_clock::now();
        function(count);
        const auto finish = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double, std::milli>(finish - start).count();
        if (elapsed < best)
            best = elapsed;
    }
    return best;
}

} // namespace

void run_benchmark() {
    std::ofstream output("benchmark.csv");
    if (!output) {
        std::cerr << "Не удалось создать benchmark.csv\n";
        return;
    }
    output << "N,raw_ms,unq_ms,shrd_ms,raw_bytes,unq_bytes,shrd_bytes\n";

    const std::size_t sizes[] = {10, 1000, 10000, 100000, 1000000};
    for (const std::size_t count : sizes) {
        const double raw = measure(count, [](std::size_t n) {
            DynamicArray<Item *> values(static_cast<int>(n));
            for (std::size_t i = 0; i < n; ++i)
                values[static_cast<int>(i)] = new Item(static_cast<int>(i));
            for (std::size_t i = 0; i < n; ++i)
                delete values[static_cast<int>(i)];
        });

        const double unique = measure(count, [](std::size_t n) {
            DynamicArray<UnqPtr<Item>> values(static_cast<int>(n));
            for (std::size_t i = 0; i < n; ++i)
                values[static_cast<int>(i)] = make_unq<Item>(static_cast<int>(i));
        });

        const double shared = measure(count, [](std::size_t n) {
            DynamicArray<UnqPtr<Item>> owners(static_cast<int>(n));
            DynamicArray<ShrdPtr<Item>> descriptors(static_cast<int>(n));
            for (std::size_t i = 0; i < n; ++i) {
                owners[static_cast<int>(i)] = make_unq<Item>(static_cast<int>(i));
                descriptors[static_cast<int>(i)] =
                    ShrdPtr<Item>(std::move(owners[static_cast<int>(i)]));
            }
        });

        const std::size_t raw_bytes = count * (sizeof(Item *) + sizeof(Item));
        const std::size_t unique_bytes = count * (sizeof(UnqPtr<Item>) + sizeof(Item));
        const std::size_t shared_bytes =
            count * (sizeof(UnqPtr<Item>) + sizeof(ShrdPtr<Item>) + sizeof(UnqPtr<Item>) +
                     sizeof(std::size_t) + sizeof(Item));

        output << count << ',' << raw << ',' << unique << ',' << shared << ',' << raw_bytes << ','
               << unique_bytes << ',' << shared_bytes << '\n';
        std::cout << count << " объектов, время (мс): обычные указатели — " << raw
                  << "; UnqPtr — " << unique << "; ShrdPtr — " << shared << '\n'
                  << "  оценка памяти (байт): обычные — " << raw_bytes << "; UnqPtr — "
                  << unique_bytes << "; ShrdPtr — " << shared_bytes << '\n';
    }
    std::cout << "Результаты сохранены в benchmark.csv\n";
}
