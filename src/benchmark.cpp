#include "benchmark.hpp"
#include "smart_pointers.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <new>
#include <utility>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#elif defined(__GLIBC__)
#include <malloc.h>
#endif

namespace {

struct alignas(std::max_align_t) AllocationHeader {
    std::size_t accounted_size;
};

std::atomic<std::size_t> current_bytes{0};
std::atomic<std::size_t> peak_bytes{0};
std::atomic<std::size_t> total_allocation_calls{0};
volatile std::size_t access_sink = 0;

std::size_t usable_payload_size(const AllocationHeader *header, std::size_t requested_size) {
#if defined(__APPLE__)
    const std::size_t block_size = malloc_size(header);
    return block_size >= sizeof(AllocationHeader) ? block_size - sizeof(AllocationHeader)
                                                   : requested_size;
#elif defined(__GLIBC__)
    const std::size_t block_size = malloc_usable_size(const_cast<AllocationHeader *>(header));
    return block_size >= sizeof(AllocationHeader) ? block_size - sizeof(AllocationHeader)
                                                   : requested_size;
#else
    return requested_size;
#endif
}

void record_allocation(std::size_t size) noexcept {
    const std::size_t now = current_bytes.fetch_add(size, std::memory_order_relaxed) + size;
    total_allocation_calls.fetch_add(1, std::memory_order_relaxed);
    std::size_t peak = peak_bytes.load(std::memory_order_relaxed);
    while (peak < now &&
           !peak_bytes.compare_exchange_weak(peak, now, std::memory_order_relaxed)) {
    }
}

void reset_peak_to_current() noexcept {
    peak_bytes.store(current_bytes.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

struct Item {
    std::size_t value;
    explicit Item(std::size_t value) : value(value) {}
};

struct Result {
    double create_ms = 0;
    double access_ms = 0;
    double destroy_ms = 0;
    std::size_t peak_usable_bytes = 0;
    std::size_t allocation_calls = 0;
};

using Clock = std::chrono::steady_clock;

double milliseconds(Clock::time_point start, Clock::time_point finish) {
    return std::chrono::duration<double, std::milli>(finish - start).count();
}

template <class Pointer, class Factory, class Releaser>
Result measure_pointer_array(std::size_t count, std::size_t repetitions, Factory make,
                             Releaser release) {
    Result total;
    for (std::size_t repetition = 0; repetition < repetitions; ++repetition) {
        const std::size_t baseline = current_bytes.load(std::memory_order_relaxed);
        const std::size_t allocation_baseline =
            total_allocation_calls.load(std::memory_order_relaxed);
        reset_peak_to_current();
        Clock::time_point destroy_start;
        Clock::time_point destroy_finish;
        {
            const auto create_start = Clock::now();
            DynamicArray<Pointer> values(static_cast<int>(count));
            for (std::size_t i = 0; i < count; ++i)
                values[static_cast<int>(i)] = make(i);
            const auto create_finish = Clock::now();
            total.create_ms += milliseconds(create_start, create_finish);

            const auto peak = peak_bytes.load(std::memory_order_relaxed);
            if (peak >= baseline)
                total.peak_usable_bytes = std::max(total.peak_usable_bytes, peak - baseline);
            total.allocation_calls = std::max(
                total.allocation_calls,
                total_allocation_calls.load(std::memory_order_relaxed) - allocation_baseline);

            const auto access_start = Clock::now();
            std::size_t checksum = 0;
            for (std::size_t i = 0; i < count; ++i)
                checksum += values[static_cast<int>(i)]->value;
            access_sink = checksum;
            const auto access_finish = Clock::now();
            total.access_ms += milliseconds(access_start, access_finish);

            destroy_start = Clock::now();
            for (std::size_t i = 0; i < count; ++i)
                release(values[static_cast<int>(i)]);
        }
        destroy_finish = Clock::now();
        total.destroy_ms += milliseconds(destroy_start, destroy_finish);
    }

    total.create_ms /= static_cast<double>(repetitions);
    total.access_ms /= static_cast<double>(repetitions);
    total.destroy_ms /= static_cast<double>(repetitions);
    return total;
}

Result measure_custom_shared(std::size_t count, std::size_t repetitions) {
    Result total;
    for (std::size_t repetition = 0; repetition < repetitions; ++repetition) {
        const std::size_t baseline = current_bytes.load(std::memory_order_relaxed);
        const std::size_t allocation_baseline =
            total_allocation_calls.load(std::memory_order_relaxed);
        reset_peak_to_current();
        Clock::time_point destroy_start;
        Clock::time_point destroy_finish;
        {
            const auto create_start = Clock::now();
            DynamicArray<ShrdPtr<Item>> descriptors(static_cast<int>(count));
            for (std::size_t i = 0; i < count; ++i) {
                descriptors[static_cast<int>(i)] =
                    ShrdPtr<Item>(make_unq<Item>(i));
            }
            const auto create_finish = Clock::now();
            total.create_ms += milliseconds(create_start, create_finish);

            const auto peak = peak_bytes.load(std::memory_order_relaxed);
            if (peak >= baseline)
                total.peak_usable_bytes = std::max(total.peak_usable_bytes, peak - baseline);
            total.allocation_calls = std::max(
                total.allocation_calls,
                total_allocation_calls.load(std::memory_order_relaxed) - allocation_baseline);

            const auto access_start = Clock::now();
            std::size_t checksum = 0;
            for (std::size_t i = 0; i < count; ++i)
                checksum += descriptors[static_cast<int>(i)]->value;
            access_sink = checksum;
            const auto access_finish = Clock::now();
            total.access_ms += milliseconds(access_start, access_finish);

            destroy_start = Clock::now();
            for (std::size_t i = 0; i < count; ++i)
                descriptors[static_cast<int>(i)].reset();
        }
        destroy_finish = Clock::now();
        total.destroy_ms += milliseconds(destroy_start, destroy_finish);
    }

    total.create_ms /= static_cast<double>(repetitions);
    total.access_ms /= static_cast<double>(repetitions);
    total.destroy_ms /= static_cast<double>(repetitions);
    return total;
}

enum Method { Raw, CustomUnique, CustomShared, StdUnique, StdShared, MethodCount };
const char *method_names[MethodCount] = {"raw", "UnqPtr", "ShrdPtr", "std::unique_ptr",
                                         "std::shared_ptr"};

Result measure_method(Method method, std::size_t count, std::size_t repetitions) {
    switch (method) {
    case Raw:
        return measure_pointer_array<Item *>(count, repetitions,
                                             [](std::size_t i) { return new Item(i); },
                                             [](Item *&pointer) {
                                                 delete pointer;
                                                 pointer = nullptr;
                                             });
    case CustomUnique:
        return measure_pointer_array<UnqPtr<Item>>(
            count, repetitions, [](std::size_t i) { return make_unq<Item>(i); },
            [](UnqPtr<Item> &pointer) { pointer.reset(); });
    case CustomShared:
        return measure_custom_shared(count, repetitions);
    case StdUnique:
        return measure_pointer_array<std::unique_ptr<Item>>(
            count, repetitions, [](std::size_t i) { return std::make_unique<Item>(i); },
            [](std::unique_ptr<Item> &pointer) { pointer.reset(); });
    case StdShared:
        return measure_pointer_array<std::shared_ptr<Item>>(
            count, repetitions, [](std::size_t i) { return std::make_shared<Item>(i); },
            [](std::shared_ptr<Item> &pointer) { pointer.reset(); });
    case MethodCount:
        break;
    }
    return {};
}

std::uint32_t next_random(std::uint32_t &state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

} // namespace

void *operator new(std::size_t size) {
    if (size == 0)
        size = 1;
    if (size > static_cast<std::size_t>(-1) - sizeof(AllocationHeader))
        throw std::bad_alloc();

    auto *header = static_cast<AllocationHeader *>(std::malloc(sizeof(AllocationHeader) + size));
    if (!header)
        throw std::bad_alloc();
    header->accounted_size = usable_payload_size(header, size);
    record_allocation(header->accounted_size);
    return header + 1;
}

void operator delete(void *memory) noexcept {
    if (!memory)
        return;
    auto *header = static_cast<AllocationHeader *>(memory) - 1;
    current_bytes.fetch_sub(header->accounted_size, std::memory_order_relaxed);
    std::free(header);
}

void *operator new[](std::size_t size) {
    return ::operator new(size);
}

void operator delete[](void *memory) noexcept {
    ::operator delete(memory);
}

void operator delete(void *memory, std::size_t) noexcept {
    ::operator delete(memory);
}

void operator delete[](void *memory, std::size_t) noexcept {
    ::operator delete[](memory);
}

void run_benchmark() {
    std::ofstream output("benchmark.csv");
    if (!output) {
        std::cerr << "Не удалось создать benchmark.csv\n";
        return;
    }
    output << "N,method,create_ms,access_ms,destroy_ms,peak_usable_bytes,allocation_calls\n";

    const std::size_t sizes[] = {10, 1000, 10000, 100000, 1000000, 10000000};
    constexpr std::size_t rounds = 3;
    constexpr std::size_t target_objects = 1000000;
    for (const std::size_t count : sizes) {
        const std::size_t repetitions = std::max<std::size_t>(1, target_objects / count);
        Result results[rounds][MethodCount]{};
        std::uint32_t random_state = static_cast<std::uint32_t>(count) ^ 0x9E3779B9u;

        for (std::size_t round = 0; round < rounds; ++round) {
            Method order[MethodCount] = {Raw, CustomUnique, CustomShared, StdUnique, StdShared};
            for (int i = MethodCount - 1; i > 0; --i) {
                const int j = static_cast<int>(next_random(random_state) % (i + 1));
                std::swap(order[i], order[j]);
            }

            for (const Method method : order) {
                const Result result = measure_method(method, count, repetitions);
                results[round][method] = result;
            }
        }

        for (int method = 0; method < MethodCount; ++method) {
            double create_times[rounds];
            double access_times[rounds];
            double destroy_times[rounds];
            Result result;
            for (std::size_t round = 0; round < rounds; ++round) {
                create_times[round] = results[round][method].create_ms;
                access_times[round] = results[round][method].access_ms;
                destroy_times[round] = results[round][method].destroy_ms;
                result.peak_usable_bytes =
                    std::max(result.peak_usable_bytes, results[round][method].peak_usable_bytes);
                result.allocation_calls =
                    std::max(result.allocation_calls, results[round][method].allocation_calls);
            }
            std::sort(create_times, create_times + rounds);
            std::sort(access_times, access_times + rounds);
            std::sort(destroy_times, destroy_times + rounds);
            result.create_ms = create_times[rounds / 2];
            result.access_ms = access_times[rounds / 2];
            result.destroy_ms = destroy_times[rounds / 2];
            output << count << ',' << method_names[method] << ',' << result.create_ms << ','
                   << result.access_ms << ',' << result.destroy_ms << ','
                   << result.peak_usable_bytes << ',' << result.allocation_calls << '\n';
            std::cout << count << " объектов | " << method_names[method]
                      << " | создание " << result.create_ms << " мс, чтение "
                      << result.access_ms << " мс, удаление " << result.destroy_ms
                      << " мс | пик памяти " << result.peak_usable_bytes << " байт, выделений "
                      << result.allocation_calls << '\n';
        }
    }
    std::cout << "Результаты сохранены в benchmark.csv\n";
}
