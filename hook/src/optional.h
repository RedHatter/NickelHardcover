#pragma once

template <typename T>
class Optional {
public:
    Optional() : has(false) {}

    Optional(const T& value) : has(true) {
        construct(value);
    }

    Optional(T&& value) : has(true) {
        construct(static_cast<T&&>(value));
    }

    Optional(const Optional& other) : has(false) {
        if (other.has) {
            construct(*other.ptr());
            has = true;
        }
    }

    Optional(Optional&& other) : has(false) {
        if (other.has) {
            construct(static_cast<T&&>(*other.ptr()));
            has = true;
        }
    }

    ~Optional() {
        reset();
    }

    Optional& operator=(const Optional& other) {
        if (this != &other) {
            reset();

            if (other.has) {
                construct(*other.ptr());
                has = true;
            }
        }

        return *this;
    }

    Optional& operator=(Optional&& other) {
        if (this != &other) {
            reset();

            if (other.has) {
                construct(static_cast<T&&>(*other.ptr()));
                has = true;
            }
        }

        return *this;
    }

    template <typename... Args>
    void emplace(Args&&... args) {
        reset();
        construct(static_cast<Args&&>(args)...);
        has = true;
    }

    void reset() {
        if (has) {
            ptr()->~T();
            has = false;
        }
    }

    bool has_value() const {
        return has;
    }

    explicit operator bool() const {
        return has;
    }

    template <typename U>
    T value_or(U&& fallback) const {
        if (has) {
            return *ptr();
        }
        return static_cast<T>(static_cast<U&&>(fallback));
    }

    T value_or_default() const {
        if (has) {
            return *ptr();
        }
        return T();
    }

    T& operator*() {
        return *ptr();
    }

    const T& operator*() const {
        return *ptr();
    }

    T* operator->() {
        return ptr();
    }

    const T* operator->() const {
        return ptr();
    }

private:
    bool has;
    alignas(T) unsigned char storage[sizeof(T)];

    T* ptr() {
        return static_cast<T*>(static_cast<void*>(storage));
    }

    const T* ptr() const {
        return static_cast<const T*>(static_cast<const void*>(storage));
    }

    template <typename... Args>
    void construct(Args&&... args) {
        ::new (static_cast<void*>(storage)) T(static_cast<Args&&>(args)...);
    }
};
