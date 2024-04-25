#pragma once

#include "compressed_pair.h"

#include <cstddef>  // std::nullptr_t

template <class T>
struct Slug {
    Slug() = default;
    template <class U>
    Slug(const Slug<U>&){};
    void operator()(T* ptr) {
        delete ptr;
    }
};

template <class T>
struct Slug<T[]> {
    Slug() = default;
    template <class U>
    Slug(const Slug<U>&){};
    void operator()(T* ptr) {
        delete[] ptr;
    }
};

// Primary template
template <typename T, typename Deleter = Slug<T>>
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : data_(ptr, Deleter()){};
    UniquePtr(T* ptr, Deleter deleter) : data_(ptr, std::move(deleter)){};
    template <class U, class E>
    UniquePtr(UniquePtr<U, E>&& other) noexcept {
        data_.GetFirst() = std::move(other.data_.GetFirst());
        data_.GetSecond() = std::move(other.data_.GetSecond());
        other.data_.GetFirst() = nullptr;
    }
    UniquePtr(const UniquePtr& other) = delete;
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s
    template <class U, class E>
    UniquePtr& operator=(UniquePtr<U, E>&& other) noexcept {
        if (other.data_.GetFirst() == data_.GetFirst()) {
            return *this;
        }
        data_.GetSecond()(data_.GetFirst());
        data_.GetFirst() = std::move(other.data_.GetFirst());
        data_.GetSecond() = std::move(other.data_.GetSecond());
        other.data_.GetFirst() = nullptr;
        return *this;
    }

    UniquePtr& operator=(std::nullptr_t) noexcept {
        data_.GetSecond()(data_.GetFirst());
        data_.GetFirst() = nullptr;
        return *this;
    }
    UniquePtr& operator=(const UniquePtr& other) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        data_.GetSecond()(data_.GetFirst());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* ans = data_.GetFirst();
        data_.GetFirst() = nullptr;
        return ans;
    }

    void Reset(T* ptr = nullptr) {
        T* old_ptr = data_.GetFirst();
        data_.GetFirst() = ptr;
        data_.GetSecond()(old_ptr);
    }

    void Swap(UniquePtr& other) noexcept {
        std::swap(data_, other.data_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return data_.GetFirst();
    }
    Deleter& GetDeleter() {
        return data_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return data_.GetSecond();
    }
    explicit operator bool() const {
        return (data_.GetFirst() != nullptr);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    std::add_lvalue_reference_t<T> operator*() const {
        return *data_.GetFirst();
    }
    T* operator->() const {
        return data_.GetFirst();
    }

private:
    CompressedPair<T*, Deleter> data_;
    template <class U, class E>
    friend class UniquePtr;
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : data_(ptr, Deleter()){};
    UniquePtr(T* ptr, Deleter deleter) : data_(ptr, deleter){};
    template <class U, class E>
    UniquePtr(UniquePtr<U, E>&& other) noexcept {
        data_.GetFirst() = std::move(other.data_.GetFirst());
        data_.GetSecond() = std::move(other.data_.GetSecond());
        other.data_.GetFirst() = nullptr;
    }
    UniquePtr(const UniquePtr& other) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    template <class U, class E>
    UniquePtr& operator=(UniquePtr<U, E>&& other) noexcept {
        if (other.data_.GetFirst() == data_.GetFirst()) {
            return *this;
        }
        data_.GetSecond()(data_.GetFirst());
        data_.GetFirst() = std::move(other.data_.GetFirst());
        data_.GetSecond() = std::move(other.data_.GetSecond());
        other.data_.GetFirst() = nullptr;
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) noexcept {
        data_.GetSecond()(data_.GetFirst());
        data_.GetFirst() = nullptr;
        return *this;
    }
    UniquePtr& operator=(const UniquePtr& other) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        data_.GetSecond()(data_.GetFirst());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* ans = data_.GetFirst();
        data_.GetFirst() = nullptr;
        return ans;
    }

    void Reset(T* ptr = nullptr) {
        T* old_ptr = data_.GetFirst();
        data_.GetFirst() = ptr;
        data_.GetSecond()(old_ptr);
    }

    void Swap(UniquePtr& other) noexcept {
        std::swap(data_, other.data_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return data_.GetFirst();
    }
    Deleter& GetDeleter() {
        return data_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return data_.GetSecond();
    }
    explicit operator bool() const {
        return (data_.GetFirst() != nullptr);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    T& operator*() const {
        return *data_.GetFirst();
    }
    T* operator->() const {
        return data_.GetFirst();
    }
    T& operator[](size_t i) {
        return data_.GetFirst()[i];
    }
    const T& operator[](size_t i) const {
        return data_.GetFirst()[i];
    }

private:
    CompressedPair<T*, Deleter> data_;
    template <class U, class E>
    friend class UniquePtr;
};
