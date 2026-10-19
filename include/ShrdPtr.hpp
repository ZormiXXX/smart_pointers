#pragma once

#include "UnqPtr.hpp"

#include <cstddef>
#include <type_traits>
#include <utility>

template <class T> class ShrdPtr {
    template <class> friend class ShrdPtr;

    UnqPtr<T> *master_ = nullptr;
    std::size_t *reference_count_ = nullptr;

    void release() noexcept {
        if (reference_count_ && --*reference_count_ == 0) {
            delete master_;
            delete reference_count_;
        }
        master_ = nullptr;
        reference_count_ = nullptr;
    }

    template <class U> void initialize(UnqPtr<U> &&owner) {
        if (!owner)
            return;

        auto *count = new std::size_t(1);
        try {
            auto *master = new UnqPtr<T>(std::move(owner));
            master_ = master;
            reference_count_ = count;
        } catch (...) {
            delete count;
            throw;
        }
    }

    template <class U> struct ConversionKeeper {
        ShrdPtr<U> source;
        explicit ConversionKeeper(const ShrdPtr<U> &pointer) : source(pointer) {
        }
    };

public:
    ShrdPtr() noexcept = default;

    template <class U, std::enable_if_t<std::is_convertible_v<U *, T *>, int> = 0>
    explicit ShrdPtr(UnqPtr<U> &&owner) {
        initialize(std::move(owner));
    }

    ShrdPtr(const ShrdPtr &other) noexcept
        : master_(other.master_), reference_count_(other.reference_count_) {
        if (reference_count_)
            ++*reference_count_;
    }

    template <class U, std::enable_if_t<std::is_convertible_v<U *, T *>, int> = 0>
    ShrdPtr(const ShrdPtr<U> &other) {
        if (!other)
            return;

        auto *keeper = new ConversionKeeper<U>(other);
        try {
            auto *count = new std::size_t(1);
            try {
                master_ = new UnqPtr<T>(other.get(), keeper, &delete_object<ConversionKeeper<U>>);
                reference_count_ = count;
            } catch (...) {
                delete count;
                throw;
            }
        } catch (...) {
            delete keeper;
            throw;
        }
    }

    ~ShrdPtr() {
        release();
    }

    ShrdPtr &operator=(const ShrdPtr &other) {
        if (this != &other) {
            ShrdPtr copy(other);
            swap(copy);
        }
        return *this;
    }

    ShrdPtr(ShrdPtr &&other) noexcept
        : master_(std::exchange(other.master_, nullptr)),
          reference_count_(std::exchange(other.reference_count_, nullptr)) {
    }

    ShrdPtr &operator=(ShrdPtr &&other) noexcept {
        if (this != &other) {
            release();
            master_ = std::exchange(other.master_, nullptr);
            reference_count_ = std::exchange(other.reference_count_, nullptr);
        }
        return *this;
    }

    template <class U, std::enable_if_t<std::is_convertible_v<U *, T *>, int> = 0>
    ShrdPtr &operator=(const ShrdPtr<U> &other) {
        ShrdPtr copy(other);
        swap(copy);
        return *this;
    }

    T *get() const noexcept {
        return master_ ? master_->get() : nullptr;
    }

    T &operator*() const {
        if (!get())
            throw InvalidState("dereference of empty ShrdPtr");
        return *get();
    }

    T *operator->() const {
        if (!get())
            throw InvalidState("access through empty ShrdPtr");
        return get();
    }

    std::size_t use_count() const noexcept {
        return reference_count_ ? *reference_count_ : 0;
    }

    explicit operator bool() const noexcept {
        return get() != nullptr;
    }

    void reset() noexcept {
        release();
    }

    void swap(ShrdPtr &other) noexcept {
        std::swap(master_, other.master_);
        std::swap(reference_count_, other.reference_count_);
    }

private:
    template <class U> static void delete_object(void *object) {
        delete static_cast<U *>(object);
    }
};
