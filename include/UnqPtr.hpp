#pragma once

#include "Exceptions.hpp"

#include <type_traits>
#include <utility>

template <class T> class ShrdPtr;

template <class T> class UnqPtr {
    template <class> friend class UnqPtr;
    template <class> friend class ShrdPtr;

    using Deleter = void (*)(void *);

    T *view_ = nullptr;
    void *owned_ = nullptr;
    Deleter deleter_ = nullptr;

    UnqPtr(T *view, void *owned, Deleter deleter) noexcept
        : view_(view), owned_(owned), deleter_(deleter) {
    }

    template <class U> static void delete_object(void *object) {
        delete static_cast<U *>(object);
    }

    void clear() noexcept {
        view_ = nullptr;
        owned_ = nullptr;
        deleter_ = nullptr;
    }

    void destroy() noexcept {
        if (deleter_)
            deleter_(owned_);
        clear();
    }

public:
    UnqPtr() noexcept = default;

    template <class U, std::enable_if_t<std::is_convertible_v<U *, T *>, int> = 0>
    explicit UnqPtr(U *object) noexcept
        : view_(object), owned_(object), deleter_(object ? &delete_object<U> : nullptr) {
    }

    ~UnqPtr() {
        destroy();
    }

    UnqPtr(const UnqPtr &) = delete;
    UnqPtr &operator=(const UnqPtr &) = delete;

    UnqPtr(UnqPtr &&other) noexcept
        : view_(other.view_), owned_(other.owned_), deleter_(other.deleter_) {
        other.clear();
    }

    template <class U, std::enable_if_t<std::is_convertible_v<U *, T *>, int> = 0>
    UnqPtr(UnqPtr<U> &&other) noexcept
        : view_(other.view_), owned_(other.owned_), deleter_(other.deleter_) {
        other.clear();
    }

    UnqPtr &operator=(UnqPtr &&other) noexcept {
        if (this != &other) {
            destroy();
            view_ = other.view_;
            owned_ = other.owned_;
            deleter_ = other.deleter_;
            other.clear();
        }
        return *this;
    }

    template <class U, std::enable_if_t<std::is_convertible_v<U *, T *>, int> = 0>
    UnqPtr &operator=(UnqPtr<U> &&other) noexcept {
        destroy();
        view_ = other.view_;
        owned_ = other.owned_;
        deleter_ = other.deleter_;
        other.clear();
        return *this;
    }

    T *get() const noexcept {
        return view_;
    }

    T &operator*() const {
        if (!view_)
            throw InvalidState("dereference of empty UnqPtr");
        return *view_;
    }

    T *operator->() const {
        if (!view_)
            throw InvalidState("access through empty UnqPtr");
        return view_;
    }

    explicit operator bool() const noexcept {
        return view_ != nullptr;
    }

    T *release() noexcept {
        T *result = view_;
        clear();
        return result;
    }

    void reset() noexcept {
        destroy();
    }

    template <class U, std::enable_if_t<std::is_convertible_v<U *, T *>, int> = 0>
    void reset(U *object) noexcept {
        if (view_ != object) {
            destroy();
            view_ = object;
            owned_ = object;
            deleter_ = object ? &delete_object<U> : nullptr;
        }
    }

    void swap(UnqPtr &other) noexcept {
        std::swap(view_, other.view_);
        std::swap(owned_, other.owned_);
        std::swap(deleter_, other.deleter_);
    }
};

template <class T, class... Args> UnqPtr<T> make_unq(Args &&...args) {
    return UnqPtr<T>(new T(std::forward<Args>(args)...));
}
