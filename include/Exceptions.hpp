#pragma once
#include <string>

class Exception {
protected:
    std::string message;

public:
    Exception() = default;

    explicit Exception(const std::string& text) : message(text) {}

    virtual ~Exception() = default;

    virtual const char* what() const noexcept {
        return message.c_str();
    }
};

class IndexOutOfRange : public Exception {
private:
public:
    IndexOutOfRange(int index, int size)
        : Exception("IndexOutOfRange: index=" + std::to_string(index) +
                    ", size=" + std::to_string(size)) {}
};

class InvalidArgument : public Exception {
public:
    explicit InvalidArgument(const std::string& details)
        : Exception("InvalidArgument: " + details) {}
};

class InvalidState : public Exception {
public:
    explicit InvalidState(const std::string& details)
        : Exception("InvalidState: " + details) {}
};

class InputError : public Exception {
public:
    explicit InputError(const std::string& details)
        : Exception("InputError: " + details) {}
};

class EmptyCollection : public Exception {
public:
    EmptyCollection() : Exception("EmptyCollection: collection is empty") {}
};

class ElementNotFound : public Exception {
public:
    ElementNotFound() : Exception("ElementNotFound: no matching element was found") {}
};

class NoValuePresent : public Exception {
public:
    NoValuePresent() : Exception("NoValuePresent: trying to get value from None") {}
};
