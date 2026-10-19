#include "smart_pointers.hpp"
#include <cassert>
#include <iostream>
#include <limits>

struct Tracked {
    static inline int alive = 0;
    int value;
    Tracked() : value(0) {
        ++alive;
    }
    explicit Tracked(int value) : value(value) {
        ++alive;
    }
    Tracked(const Tracked &other) : value(other.value) {
        ++alive;
    }
    ~Tracked() {
        --alive;
    }
};

struct Base {
    virtual ~Base() = default;
    virtual int value() const {
        return 1;
    }
};
struct FirstBase {
    virtual ~FirstBase() = default;
};
struct Derived : FirstBase, Base {
    static inline int destroyed = 0;
    int value() const override {
        return 2;
    }
    ~Derived() override {
        ++destroyed;
    }
};

struct PlainBase {
    static inline int destroyed = 0;
};

struct PlainDerived : FirstBase, PlainBase {
    ~PlainDerived() {
        ++destroyed;
    }
};

template <class T> static void move_assign_to_self(T &value) {
    T *volatile alias = &value;
    value = std::move(*alias);
}

template <class T> static void copy_assign_to_self(T &value) {
    T *volatile alias = &value;
    value = *alias;
}

static void test_unique_lifetime() {
    assert(Tracked::alive == 0);
    {
        auto ptr = make_unq<Tracked>(10);
        assert(Tracked::alive == 1);
        auto moved = std::move(ptr);
        assert(!ptr && moved->value == 10);
        moved.reset();
        assert(Tracked::alive == 0);
    }
}

static void test_unique_empty_release_reset_and_assignment() {
    UnqPtr<Tracked> empty;
    assert(!empty && empty.get() == nullptr);
    bool caught = false;
    try {
        (void)*empty;
    } catch (const InvalidState &) {
        caught = true;
    }
    assert(caught);

    auto first = make_unq<Tracked>(1);
    auto second = make_unq<Tracked>(2);
    first = std::move(second);
    assert(!second && first->value == 2 && Tracked::alive == 1);
    move_assign_to_self(first);
    assert(first->value == 2 && Tracked::alive == 1);

    Tracked *released = first.release();
    assert(!first && Tracked::alive == 1);
    delete released;
    assert(Tracked::alive == 0);

    auto *same = new Tracked(3);
    first.reset(same);
    first.reset(same);
    assert(Tracked::alive == 1 && first->value == 3);
    first.reset();
    assert(!first && Tracked::alive == 0);
}

static void test_unique_polymorphic_delete_without_virtual_destructor() {
    PlainBase::destroyed = 0;
    {
        auto derived = make_unq<PlainDerived>();
        UnqPtr<PlainBase> base(std::move(derived));
        assert(!derived);
    }
    assert(PlainBase::destroyed == 1);
}

static void test_shared_descriptors() {
    auto owner = make_unq<Tracked>(20);
    {
        ShrdPtr<Tracked> first(std::move(owner));
        assert(!owner);
        ShrdPtr<Tracked> second(first);
        assert(first.use_count() == 2);
        assert(Tracked::alive == 1);
        assert(first.use_count() == 2 && second->value == 20);
    }
    assert(Tracked::alive == 0);
}

static void test_shared_empty_copy_move_and_reset() {
    ShrdPtr<Tracked> empty;
    ShrdPtr<Tracked> emptyCopy(empty);
    assert(!empty && !emptyCopy && empty.use_count() == 0);

    ShrdPtr<Tracked> owner(make_unq<Tracked>(31));
    assert(owner.use_count() == 1);
    ShrdPtr<Tracked> copy(owner);
    copy_assign_to_self(copy);
    assert(owner.use_count() == 2 && copy->value == 31);

    ShrdPtr<Tracked> moved(std::move(copy));
    assert(!copy && moved.use_count() == 2);
    ShrdPtr<Tracked> destination(make_unq<Tracked>(99));
    destination = std::move(moved);
    assert(!moved && destination->value == 31 && owner.use_count() == 2);

    owner.reset();
    assert(destination.use_count() == 1 && Tracked::alive == 1);
    destination.reset();
    assert(Tracked::alive == 0);
}

static void test_polymorphic_destruction() {
    auto owner = make_unq<Derived>();
    ShrdPtr<Base> descriptor(std::move(owner));
    assert(!owner);
    assert(Derived::destroyed == 0);
    descriptor.reset();
    assert(Derived::destroyed == 1);

    ShrdPtr<Derived> specific(make_unq<Derived>());
    ShrdPtr<Base> general(specific);
    specific.reset();
    assert(general->value() == 2);
    assert(dynamic_cast<Derived *>(general.get()) != nullptr);
    general.reset();
    assert(Derived::destroyed == 2);
}

static void test_shared_multiple_inheritance_adjustment() {
    ShrdPtr<Derived> derived(make_unq<Derived>());
    ShrdPtr<Base> base(derived);
    assert(base->value() == 2);
    assert(dynamic_cast<Derived *>(base.get()) != nullptr);
    derived.reset();
    assert(base->value() == 2);
    base.reset();
}

static void test_memory_span_and_bounds() {
    {
        MemorySpan<Tracked> span(8);
        auto p = span.Locate(3);
        assert(p->value == 0);
        bool caught = false;
        try {
            (void)(p + 8);
        } catch (const InvalidState &) {
            caught = true;
        }
        assert(caught);
    }
    assert(Tracked::alive == 0);
}

static void test_memory_span_zero_and_all_boundaries() {
    MemorySpan<int> empty(0);
    assert(empty.size() == 0);
    bool caught = false;
    try {
        (void)empty.Locate(0);
    } catch (const IndexOutOfRange &) {
        caught = true;
    }
    assert(caught);

    MemorySpan<int> values(2);
    values.at(0) = 7;
    values.at(1) = 9;
    auto first = values.Locate(0);
    auto last = values.Locate(1);
    assert(*(first + 0) == 7 && *(last + 0) == 9);
    assert(*(last + -1) == 7 && last - first == 1);

    caught = false;
    try {
        (void)(first + -1);
    } catch (const InvalidState &) {
        caught = true;
    }
    assert(caught);

    caught = false;
    try {
        ++last;
    } catch (const InvalidState &) {
        caught = true;
    }
    assert(caught);

    caught = false;
    try {
        (void)values.at(2);
    } catch (const IndexOutOfRange &) {
        caught = true;
    }
    assert(caught);

    MemorySpan<int> other(1);
    caught = false;
    try {
        (void)(first - other.Locate(0));
    } catch (const InvalidArgument &) {
        caught = true;
    }
    assert(caught);

    caught = false;
    try {
        MemorySpan<int> tooLarge(static_cast<std::size_t>(std::numeric_limits<int>::max()) + 1);
        (void)tooLarge;
    } catch (const InvalidArgument &) {
        caught = true;
    }
    assert(caught);
}

static void test_dynamic_array_uses_unique_array_ownership() {
    assert(Tracked::alive == 0);
    {
        DynamicArray<Tracked> values;
        for (int i = 0; i < 9; ++i)
            values.Append(Tracked(i));
        assert(values.GetSize() == 9 && values.GetCapacity() >= 9);
        assert(values[0].value == 0 && values[8].value == 8);

        DynamicArray<Tracked> copied(values);
        assert(copied.GetSize() == 9 && copied[4].value == 4);

        DynamicArray<Tracked> moved(std::move(copied));
        assert(copied.GetSize() == 0 && moved.GetSize() == 9);

        DynamicArray<Tracked> assigned;
        assigned = values;
        assert(assigned.GetSize() == 9 && assigned[8].value == 8);

        DynamicArray<Tracked> moveAssigned;
        moveAssigned = std::move(assigned);
        assert(assigned.GetSize() == 0 && moveAssigned.GetSize() == 9);
        moveAssigned.Resize(0);
        assert(moveAssigned.GetSize() == 0);
    }
    assert(Tracked::alive == 0);

    {
        DynamicArray<UnqPtr<Tracked>> owners(2);
        owners[0] = make_unq<Tracked>(101);
        owners[1] = make_unq<Tracked>(202);
        DynamicArray<UnqPtr<Tracked>> moved(std::move(owners));
        assert(owners.GetSize() == 0);
        assert(moved[0]->value == 101 && moved[1]->value == 202);
    }
    assert(Tracked::alive == 0);
}

int main() {
    test_unique_lifetime();
    test_unique_empty_release_reset_and_assignment();
    test_unique_polymorphic_delete_without_virtual_destructor();
    test_shared_descriptors();
    test_shared_empty_copy_move_and_reset();
    test_polymorphic_destruction();
    test_shared_multiple_inheritance_adjustment();
    test_memory_span_and_bounds();
    test_memory_span_zero_and_all_boundaries();
    test_dynamic_array_uses_unique_array_ownership();
    assert(Tracked::alive == 0);
    std::cout << "Memory safety and edge-case tests: OK\n";
}
