#include "functional_tests.hpp"
#include "smart_pointers.hpp"

#include <exception>
#include <iostream>
#include <utility>

namespace {

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

struct TestFailure {};

#define CHECK(condition)                                                                          \
    do {                                                                                          \
        if (!(condition))                                                                         \
            throw TestFailure{};                                                                  \
    } while (false)

template <class Test>
bool run_test(const char *name, Test test) {
    try {
        test();
        std::cout << "  [УСПЕХ] " << name << '\n';
        return true;
    } catch (const TestFailure &) {
        std::cout << "  [ОШИБКА] " << name << " — проверка не пройдена\n";
    } catch (const std::exception &error) {
        std::cout << "  [ОШИБКА] " << name << " — исключение: " << error.what() << '\n';
    } catch (...) {
        std::cout << "  [ОШИБКА] " << name << " — неизвестное исключение\n";
    }
    return false;
}

}

void run_functional_tests() {
    std::cout << "\nФункциональные тесты:\n";
    int passed = 0;
    int failed = 0;
    const auto run = [&](const char *name, auto test) {
        if (run_test(name, test))
            ++passed;
        else
            ++failed;
    };

    run("DynamicArray: доступ и проверка границ", [] {
        DynamicArray<int> values(3);
        values[0] = 4;
        values[1] = 8;
        values[2] = 12;
        CHECK(values.GetSize() == 3);
        CHECK(values.Get(0) == 4 && values.Get(2) == 12);

        bool caught = false;
        try {
            (void)values.Get(3);
        } catch (const IndexOutOfRange &) {
            caught = true;
        }
        CHECK(caught);
    });

    run("UnqPtr → ShrdPtr: передача владения и счётчик", [] {
        auto owner = make_unq<Item>(42);
        CHECK(owner->value() == 42);

        ShrdPtr<Item> first(std::move(owner));
        CHECK(!owner);
        CHECK(first.use_count() == 1);
        {
            auto second = first;
            CHECK(second.use_count() == 2);
        }
        CHECK(first.use_count() == 1);
        CHECK(first->value() == 42);
        first.reset();
        CHECK(Item::alive == 0);
    });

    run("UnqPtr: преобразование производного типа к базовому", [] {
        auto derived = make_unq<MultiDerived>();
        UnqPtr<Base> base(std::move(derived));
        CHECK(base->value() == 99);
        CHECK(dynamic_cast<MultiDerived *>(base.get()) != nullptr);
    });

    run("ShrdPtr: преобразование при множественном наследовании", [] {
        ShrdPtr<MultiDerived> derived(make_unq<MultiDerived>());
        ShrdPtr<Base> base(derived);
        derived.reset();
        CHECK(base->value() == 99);
    });

    run("MemorySpan / MsPtr: арифметика и копирование значений", [] {
        MemorySpan<int> span(3);
        span.at(0) = 10;
        span.at(1) = 20;
        auto begin = span.Locate(0);
        auto next = begin + 1;
        CHECK(*begin == 10 && *next == 20 && next - begin == 1);

        auto valueCopy = span.Get(1);
        auto sharedCopy = span.Copy(2);
        CHECK(*valueCopy == 20 && *sharedCopy == 0);
    });

    run("MemorySpan: отказ при обращении за границы", [] {
        bool caught = false;
        try {
            MemorySpan<int> span(2);
            (void)span.Locate(2);
        } catch (const IndexOutOfRange &) {
            caught = true;
        }
        CHECK(caught);
    });

    run("Item: все тестовые объекты освобождены", [] { CHECK(Item::alive == 0); });

    std::cout << "Итого: " << passed << " пройдено, " << failed << " с ошибкой.\n";
}
