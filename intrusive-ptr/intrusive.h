#pragma once

#include <cstddef>  // for std::nullptr_t
#include <utility>  // for std::exchange / std::swap

class SimpleCounter {
public:
    size_t IncRef() {
        ++count_;
        return count_;
    }
    size_t DecRef() {
        --count_;
        return count_;
    }
    size_t RefCount() const {
        return count_;
    }

private:
    size_t count_ = 0;
};

struct DefaultDelete {
    template <typename T>
    static void Destroy(T* object) {
        delete object;
    }
};

template <typename Derived, typename Counter, typename Deleter>
class RefCounted {
public:
    // Increase reference counter.
    void IncRef() {
        counter_.IncRef();
    }

    // Decrease reference counter.
    // Destroy object using Deleter when the last instance dies.
    void DecRef() {
        counter_.DecRef();
        if (counter_.RefCount() == 0) {
            Deleter().Destroy(static_cast<Derived*>(this));
        }
    }

    // Get current counter value (the number of strong references).
    size_t RefCount() const {
        return counter_.RefCount();
    }

private:
    Counter counter_;
};

template <typename Derived, typename D = DefaultDelete>
using SimpleRefCounted = RefCounted<Derived, SimpleCounter, D>;

template <typename T>
class IntrusivePtr {
public:
    // Constructors
    IntrusivePtr() : observed_(nullptr){};
    IntrusivePtr(std::nullptr_t) : observed_(nullptr){};
    template <class U>
    explicit IntrusivePtr(U* ptr) {
        observed_ = ptr;
        if (observed_) {
            observed_->IncRef();
        }
    }
    explicit IntrusivePtr(T* ptr) {
        observed_ = ptr;
        if (observed_) {
            observed_->IncRef();
        }
    }

    template <typename Y>
    IntrusivePtr(const IntrusivePtr<Y>& other) {
        observed_ = other.observed_;
        if (observed_) {
            observed_->IncRef();
        }
    }

    template <typename Y>
    IntrusivePtr(IntrusivePtr<Y>&& other) noexcept {
        observed_ = std::move(other.observed_);
        other.observed_ = nullptr;
    }

    IntrusivePtr(const IntrusivePtr& other) {
        observed_ = other.observed_;
        if (observed_) {
            observed_->IncRef();
        }
    }
    IntrusivePtr(IntrusivePtr&& other) noexcept {
        observed_ = std::move(other.observed_);
        other.observed_ = nullptr;
    }

    // `operator=`-s
    IntrusivePtr& operator=(const IntrusivePtr& other) {
        /*if (observed_ == other.observed_) {
            return *this;
        }*/
        if (other.observed_) {
            other.observed_->IncRef();
        }
        if (observed_) {
            observed_->DecRef();
        }
        observed_ = other.observed_;
        return *this;
    }
    IntrusivePtr& operator=(IntrusivePtr&& other) noexcept {
        if (observed_ == other.observed_) {
            return *this;
        }
        if (observed_) {
            observed_->DecRef();
        }
        observed_ = std::move(other.observed_);
        other.observed_ = nullptr;
        return *this;
    }
    template <class U>
    IntrusivePtr& operator=(const IntrusivePtr<U>& other) {
        if (observed_ == other.observed_) {
            return *this;
        }
        if (other.observed_) {
            other.observed_->IncRef();
        }
        if (observed_) {
            observed_->DecRef();
        }
        observed_ = other.observed_;
        return *this;
    }

    template <class U>
    IntrusivePtr& operator=(IntrusivePtr<U>&& other) noexcept {
        if (observed_ == other.observed_) {
            return *this;
        }
        if (observed_) {
            observed_->DecRef();
        }
        observed_ = std::move(other.observed_);
        other.observed_ = nullptr;
        return *this;
    }

    // Destructor
    ~IntrusivePtr() {
        if (observed_) {
            observed_->DecRef();
        }
    }

    // Modifiers
    void Reset() {
        if (observed_) {
            observed_->DecRef();
            observed_ = nullptr;
        }
    }
    void Reset(T* ptr) {
        if (observed_ == ptr) {
            return;
        }
        if (observed_) {
            observed_->DecRef();
        }
        observed_ = ptr;
        if (observed_) {
            observed_->IncRef();
        }
    }
    void Swap(IntrusivePtr& other) {
        std::swap(observed_, other.observed_);
    }

    // Observers
    T* Get() const {
        return observed_;
    }
    T& operator*() const {
        return *observed_;
    }
    T* operator->() const {
        return observed_;
    }
    size_t UseCount() const {
        if (observed_) {
            return observed_->RefCount();
        }
        return 0;
    }
    explicit operator bool() const {
        if (observed_) {
            return true;
        }
        return false;
    }

    template <typename U, typename... Args>
    friend IntrusivePtr<U> MakeIntrusive(Args&&... args);

private:
    template <typename Y>
    friend class IntrusivePtr;
    T* observed_;
};

template <typename T, typename... Args>
IntrusivePtr<T> MakeIntrusive(Args&&... args) {
    IntrusivePtr<T> ans(new T(std::forward<Args>(args)...));
    // auto new_observed = new T(std::forward<Args>(args)...);
    // ans.observed_ = new_observed;
    return ans;
}
