#include "functional_tests.hpp"
#include "smart_pointers.hpp"

#include <cassert>
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

} // namespace

void run_functional_tests() {
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
        } catch (const IndexOutOfRange &) {
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
    std::cout << "Функциональные тесты пройдены.\n";
}
