#include "smart_pointers.hpp"

#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

struct Base {
    virtual ~Base() = default;
    virtual int value() const = 0;
};

struct Item : Base {
    static inline int alive = 0;
    int data;

    explicit Item(int value) : data(value) {
        ++alive;
    }
    Item(const Item &other) : data(other.data) {
        ++alive;
    }
    ~Item() override {
        --alive;
    }
    int value() const override {
        return data;
    }
};

struct OtherBase {
    virtual ~OtherBase() = default;
    int marker = 5;
};

struct MultiDerived : OtherBase, Base {
    int value() const override {
        return 99;
    }
};

static void functional_tests() {
    {
        DynamicArray<int> values(3);
        values[0] = 4;
        values[1] = 8;
        values[2] = 12;
        assert(values.GetSize() == 3);
        assert(values.Get(0) == 4 && values.Get(2) == 12);

        bool caught = false;
        try {
            (void)values.Get(3);
        } catch (const IndexOutOfRange&) {
            caught = true;
        }
        assert(caught);
    }

    {
        auto owner = make_unq<Item>(42);
        assert(owner->value() == 42);

        ShrdPtr<Item> first(std::move(owner));
        assert(!owner);
        assert(first.use_count() == 1);
        {
            auto second = first;
            assert(second.use_count() == 2);
        }
        assert(first.use_count() == 1);

        assert(first->value() == 42);
    }
    assert(Item::alive == 0);

    {
        auto derived = make_unq<MultiDerived>();
        UnqPtr<Base> base(std::move(derived));
        assert(base->value() == 99);
        assert(dynamic_cast<MultiDerived *>(base.get()) != nullptr);
    }

    {
        ShrdPtr<MultiDerived> derived(make_unq<MultiDerived>());
        ShrdPtr<Base> base(derived);
        derived.reset();
        assert(base->value() == 99);
    }

    {
        MemorySpan<int> span(3);
        span.at(0) = 10;
        span.at(1) = 20;
        auto begin = span.Locate(0);
        auto next = begin + 1;
        assert(*begin == 10 && *next == 20 && next - begin == 1);

        auto valueCopy = span.Get(1);
        auto sharedCopy = span.Copy(2);
        assert(*valueCopy == 20 && *sharedCopy == 0);
    }

    bool caught = false;
    try {
        MemorySpan<int> span(2);
        (void)span.Locate(2);
    } catch (const IndexOutOfRange &) {
        caught = true;
    }
    assert(caught);
    assert(Item::alive == 0);
    std::cout << "Functional tests: OK\n";
}

template <class Function> static double measure(std::size_t count, Function &&function) {
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

static void benchmark() {
    std::ofstream output("benchmark.csv");
    if (!output) {
        std::cerr << "Cannot create benchmark.csv\n";
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
        std::cout << count << " objects: raw=" << raw << " ms, UnqPtr=" << unique
                  << " ms, ShrdPtr=" << shared << " ms\n"
                  << "  estimated useful memory: raw=" << raw_bytes << " B, UnqPtr=" << unique_bytes
                  << " B, ShrdPtr=" << shared_bytes << " B\n";
    }
    std::cout << "Benchmark saved to benchmark.csv\n";
}

static void show_header() {
    std::cout << "\n"
                 "+--------------------------------------------------+\n"
                 "|             HW1: SMART POINTERS                 |\n"
                 "|       UnqPtr / ShrdPtr / MemorySpan / MsPtr      |\n"
                 "+--------------------------------------------------+\n";
}

static void show_menu() {
    std::cout << "\n"
                 "+--------------------------------------------------+\n"
                 "|  1  Demonstrate UnqPtr                           |\n"
                 "|  2  Demonstrate ShrdPtr                          |\n"
                 "|  3  Demonstrate MemorySpan and MsPtr             |\n"
                 "|  4  Run functional tests                         |\n"
                 "|  5  Run benchmark                                |\n"
                 "|  0  Exit                                         |\n"
                 "+--------------------------------------------------+\n"
                 "Select an option: ";
}

static void demonstrate_unique() {
    auto source = make_unq<int>(73);
    std::cout << "Created UnqPtr, value = " << *source << '\n';
    auto destination = std::move(source);
    std::cout << "After move: source is " << (source ? "not empty" : "empty")
              << ", destination value = " << *destination << '\n';
    std::cout << "The object is deleted automatically when destination leaves scope.\n";
}

static void demonstrate_shared() {
    auto owner = make_unq<int>(128);
    ShrdPtr<int> first(std::move(owner));
    std::cout << "Moved the unique owner into the first descriptor.\n"
              << "owner is " << (owner ? "not empty" : "empty")
              << ", descriptor count = " << first.use_count() << '\n';
    {
        auto second = first;
        std::cout << "Copied the descriptor: count = " << first.use_count() << ", both read value "
                  << *second << '\n';
    }
    std::cout << "After the copy leaves scope: count = " << first.use_count() << '\n';
}

static void demonstrate_memory_span() {
    MemorySpan<int> values(4);
    for (std::size_t i = 0; i < values.size(); ++i)
        values.at(i) = static_cast<int>((i + 1) * 10);

    auto first = values.Locate(0);
    auto third = first + 2;
    std::cout << "MsPtr visits values " << *first << " and " << *third
              << "; distance = " << third - first << '\n';

    try {
        (void)(third + 2);
    } catch (const InvalidState &error) {
        std::cout << "Out-of-range arithmetic was blocked: " << error.what() << '\n';
    }
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        const std::string_view mode(argv[1]);
        if (mode == "--test") {
            functional_tests();
            return 0;
        }
        if (mode == "--benchmark") {
            benchmark();
            return 0;
        }
        if (mode == "--help") {
            std::cout << "Usage: hw1 [--test|--benchmark|--help]\n";
            return 0;
        }
        std::cerr << "Unknown option: " << mode << '\n';
        return 2;
    }

    show_header();
    while (true) {
        show_menu();
        int option = -1;
        if (!(std::cin >> option)) {
            if (std::cin.eof())
                break;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Please enter a menu number.\n";
            continue;
        }

        switch (option) {
        case 1:
            demonstrate_unique();
            break;
        case 2:
            demonstrate_shared();
            break;
        case 3:
            demonstrate_memory_span();
            break;
        case 4:
            functional_tests();
            break;
        case 5:
            benchmark();
            break;
        case 0:
            std::cout << "Goodbye.\n";
            return 0;
        default:
            std::cout << "Unknown option. Select a number from the menu.\n";
        }
    }
    return 0;
}
