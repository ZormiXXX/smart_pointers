#pragma once

#include "DynamicArray.hpp"
#include "Exceptions.hpp"
#include "ShrdPtr.hpp"

#include <cstddef>
#include <compare>
#include <limits>

template <class T> class MemorySpan;

template <class T> class MsPtr {
    MemorySpan<T> *owner_ = nullptr;
    std::size_t index_ = 0;
    friend class MemorySpan<T>;

    MsPtr(MemorySpan<T> *owner, std::size_t index) : owner_(owner), index_(index) {
    }

public:
    T &operator*() const;
    T *operator->() const {
        return &operator*();
    }
    MsPtr &operator++();
    MsPtr &operator--();
    MsPtr operator++(int) {
        MsPtr previous(*this);
        ++(*this);
        return previous;
    }
    MsPtr operator--(int) {
        MsPtr previous(*this);
        --(*this);
        return previous;
    }
    MsPtr &operator+=(std::ptrdiff_t offset);
    MsPtr &operator-=(std::ptrdiff_t offset);
    MsPtr operator+(std::ptrdiff_t offset) const;
    MsPtr operator-(std::ptrdiff_t offset) const;
    T &operator[](std::ptrdiff_t offset) const;
    std::ptrdiff_t operator-(const MsPtr &other) const;

    bool operator==(const MsPtr &other) const noexcept {
        return owner_ == other.owner_ && index_ == other.index_;
    }
    bool operator!=(const MsPtr &other) const noexcept {
        return !(*this == other);
    }

    std::strong_ordering operator<=>(const MsPtr &other) const {
        if (owner_ != other.owner_)
            throw InvalidArgument("cannot order MsPtr values from different MemorySpan objects");
        return index_ <=> other.index_;
    }
};

template <class T> class MemorySpan {
    DynamicArray<T> data_;
    friend class MsPtr<T>;

    static int checked_size(std::size_t size) {
        if (size > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw InvalidArgument("MemorySpan size exceeds DynamicArray limit");
        return static_cast<int>(size);
    }

    void check(std::size_t index) const {
        if (index >= size())
            throw IndexOutOfRange(static_cast<int>(index), static_cast<int>(size()));
    }

public:
    explicit MemorySpan(std::size_t size) : data_(checked_size(size)) {
    }

    std::size_t size() const noexcept {
        return static_cast<std::size_t>(data_.GetSize());
    }

    UnqPtr<T> Get(std::size_t index) {
        check(index);
        return make_unq<T>(data_[static_cast<int>(index)]);
    }

    ShrdPtr<T> Copy(std::size_t index) {
        check(index);
        return ShrdPtr<T>(make_unq<T>(data_[static_cast<int>(index)]));
    }

    MsPtr<T> Locate(std::size_t index) {
        check(index);
        return MsPtr<T>(this, index);
    }

    MsPtr<T> Begin() {
        return MsPtr<T>(this, 0);
    }

    MsPtr<T> End() {
        return MsPtr<T>(this, size());
    }

    MsPtr<T> begin() {
        return Begin();
    }

    MsPtr<T> end() {
        return End();
    }

    T &at(std::size_t index) {
        check(index);
        return data_[static_cast<int>(index)];
    }
};

template <class T> T &MsPtr<T>::operator*() const {
    if (!owner_ || index_ >= owner_->size())
        throw InvalidState("invalid MsPtr");
    return owner_->data_[static_cast<int>(index_)];
}

template <class T> MsPtr<T> &MsPtr<T>::operator++() {
    if (!owner_ || index_ >= owner_->size())
        throw InvalidState("MsPtr increment past end");
    ++index_;
    return *this;
}

template <class T> MsPtr<T> &MsPtr<T>::operator--() {
    if (!owner_ || index_ == 0)
        throw InvalidState("MsPtr decrement before begin");
    --index_;
    return *this;
}

template <class T> MsPtr<T> &MsPtr<T>::operator+=(std::ptrdiff_t offset) {
    *this = *this + offset;
    return *this;
}

template <class T> MsPtr<T> &MsPtr<T>::operator-=(std::ptrdiff_t offset) {
    *this = *this - offset;
    return *this;
}

template <class T> MsPtr<T> MsPtr<T>::operator+(std::ptrdiff_t offset) const {
    if (!owner_ || index_ > owner_->size())
        throw InvalidState("invalid MsPtr");
    const auto current = static_cast<std::ptrdiff_t>(index_);
    const auto last = static_cast<std::ptrdiff_t>(owner_->size());
    if (offset < -current || offset > last - current)
        throw InvalidState("MsPtr arithmetic out of range");
    const auto next = current + offset;
    return MsPtr(owner_, static_cast<std::size_t>(next));
}

template <class T> MsPtr<T> MsPtr<T>::operator-(std::ptrdiff_t offset) const {
    if (offset == std::numeric_limits<std::ptrdiff_t>::min())
        throw InvalidState("MsPtr arithmetic out of range");
    return *this + (-offset);
}

template <class T> T &MsPtr<T>::operator[](std::ptrdiff_t offset) const {
    return *(*this + offset);
}

template <class T> std::ptrdiff_t MsPtr<T>::operator-(const MsPtr &other) const {
    if (owner_ != other.owner_)
        throw InvalidArgument("different MemorySpan objects");
    return static_cast<std::ptrdiff_t>(index_) - static_cast<std::ptrdiff_t>(other.index_);
}
