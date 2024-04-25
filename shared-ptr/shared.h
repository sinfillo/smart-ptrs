#pragma once

#include "sw_fwd.h"  // Forward declaration
#include <cstddef>   // std::nullptr_t

// https://en.cppreference.com/w/cpp/memory/shared_ptr
class ControlBlockBase {
public:
    virtual void IncCounter(bool is_weak = false) {
        if (is_weak) {
            ++weak_counter_;
        } else {
            ++strong_counter_;
        }
    }
    virtual void DecCounter(bool is_weak = false) = 0;
    virtual size_t GetCounter(bool is_weak = false) {
        if (is_weak) {
            return weak_counter_;
        } else {
            return strong_counter_;
        }
    }
    virtual ~ControlBlockBase() = default;

protected:
    size_t strong_counter_ = 1;
    size_t weak_counter_ = 0;
};

template <class T>
class ControlBlockPtr : public ControlBlockBase {
public:
    ControlBlockPtr() : pointer_(nullptr){};
    ControlBlockPtr(T* pointer) : pointer_(pointer){};
    void DecCounter(bool is_weak = false) override {
        if (is_weak) {
            --weak_counter_;
        } else {
            --strong_counter_;
        }
        if (!is_weak && strong_counter_ == 0) {
            delete pointer_;
            pointer_ = nullptr;
        }
        if (strong_counter_ == 0 && weak_counter_ == 0) {
            delete this;
        }
    }

private:
    T* pointer_;
};

template <class T>
class ControlBlockBuffer : public ControlBlockBase {
public:
    template <typename... Args>
    ControlBlockBuffer(Args&&... args) {
        new (&data_) T(std::forward<Args>(args)...);
    }
    void DecCounter(bool is_weak = false) {
        if (is_weak) {
            --weak_counter_;
        } else {
            --strong_counter_;
        }

        if (!is_weak && strong_counter_ == 0) {
            reinterpret_cast<T*>(&data_)->~T();
        }
        if (strong_counter_ == 0 && weak_counter_ == 0) {
            delete this;
        }
        /*if (is_weak) {
            if (strong_counter_ == 0) {
                if (weak_counter_ == 0) {
                    //reinterpret_cast<T*>(&data_)->~T();
                    delete this;
                } else {
                    reinterpret_cast<T*>(&data_)->~T();
                }
            }
        } else {
            if (strong_counter_ == 0) {
                if (weak_counter_ == 0) {
                    reinterpret_cast<T*>(&data_)->~T();
                    delete this;
                } else {
                    reinterpret_cast<T*>(&data_)->~T();
                }
            }
        }*/
    }
    T* GetObserved() {
        return reinterpret_cast<T*>(&data_);
    }

private:
    std::aligned_storage_t<sizeof(T), alignof(T)> data_;
};

class EnableSharedFromThisBase {};

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis : public EnableSharedFromThisBase {
public:
    SharedPtr<T> SharedFromThis() {
        return SharedPtr<T>(weak_this_);
    }
    SharedPtr<const T> SharedFromThis() const {
        return SharedPtr<T>(weak_this_);
    }

    WeakPtr<T> WeakFromThis() noexcept {
        return weak_this_;
    }
    WeakPtr<const T> WeakFromThis() const noexcept {
        return weak_this_;
    }

private:
    template <class U>
    friend class SharedPtr;
    WeakPtr<T> weak_this_;
};

template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() : block_(nullptr), observed_(nullptr){};
    SharedPtr(std::nullptr_t) : block_(nullptr), observed_(nullptr){};
    explicit SharedPtr(T* ptr) : block_(new ControlBlockPtr<T>(ptr)), observed_(ptr) {
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            InitWeakThis(ptr);
        }
    };
    template <class U>
    explicit SharedPtr(U* ptr) : block_(new ControlBlockPtr<U>(ptr)), observed_(ptr) {
        if constexpr (std::is_convertible_v<U*, EnableSharedFromThisBase*>) {
            InitWeakThis(ptr);
        }
    };
    SharedPtr(const SharedPtr& other) {
        block_ = other.block_;
        observed_ = other.observed_;
        if (block_) {
            block_->IncCounter();
        }
    }
    SharedPtr(SharedPtr&& other) noexcept {
        block_ = std::move(other.block_);
        observed_ = std::move(other.observed_);
        other.block_ = nullptr;
        other.observed_ = nullptr;
    }
    template <class U>
    SharedPtr(const SharedPtr<U>& other) {
        block_ = other.block_;
        observed_ = other.observed_;
        if (block_) {
            block_->IncCounter();
        }
    }
    template <class U>
    SharedPtr(SharedPtr<U>&& other) noexcept {
        block_ = std::move(other.block_);
        observed_ = std::move(other.observed_);
        other.block_ = nullptr;
        other.observed_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        block_ = other.block_;
        observed_ = ptr;
        if (block_) {
            block_->IncCounter();
        }
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) {
        if (other.Expired()) {
            throw BadWeakPtr();
        }
        block_ = other.block_;
        observed_ = other.observed_;
        if (block_) {
            block_->IncCounter();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (block_ == other.block_) {
            return *this;
        }
        if (block_) {
            block_->DecCounter();
            /*if (block_->GetCounter() == 0 && block_->GetCounter(true) == 0) {
                delete block_;
            }*/
        }
        block_ = other.block_;
        observed_ = other.observed_;
        if (block_) {
            block_->IncCounter();
        }
        return *this;
    }
    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (block_ == other.block_) {
            return *this;
        }
        if (block_) {
            block_->DecCounter();
            /*if (block_->GetCounter() == 0 && block_->GetCounter(true) == 0) {
                delete block_;
            }*/
        }
        block_ = std::move(other.block_);
        observed_ = std::move(other.observed_);
        other.block_ = nullptr;
        other.observed_ = nullptr;
        return *this;
    }
    template <class U>
    SharedPtr& operator=(const SharedPtr<U>& other) {
        if (block_ == other.block_) {
            return *this;
        }
        if (block_) {
            block_->DecCounter();
            /*if (block_->GetCounter() == 0 && block_->GetCounter(true) == 0) {
                delete block_;
            }*/
        }
        block_ = other.block_;
        observed_ = other.observed_;
        if (block_) {
            block_->IncCounter();
        }
        return *this;
    }
    template <class U>
    SharedPtr& operator=(SharedPtr<U>&& other) noexcept {
        if (block_ == other.block_) {
            return *this;
        }
        if (block_) {
            block_->DecCounter();
            /*if (block_->GetCounter() == 0 && block_->GetCounter(true) == 0) {
                delete block_;
            }*/
        }
        block_ = std::move(other.block_);
        observed_ = std::move(other.observed_);
        other.block_ = nullptr;
        other.observed_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        // WeakPtr<T> tmp(*this);
        if (block_) {
            block_->IncCounter(true);
            block_->DecCounter();
            block_->DecCounter(true);
            /*if (block_->GetCounter() == 0 && block_->GetCounter(true) == 0) {
                delete block_;
            }*/
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_) {
            block_->IncCounter(true);
            block_->DecCounter();
            block_->DecCounter(true);
            /*if (block_->GetCounter() == 0 && block_->GetCounter(true) == 0) {
                delete block_;
            }*/
            block_ = nullptr;
            observed_ = nullptr;
        }
    }
    void Reset(T* ptr) {
        if (block_) {
            block_->IncCounter(true);
            block_->DecCounter();
            block_->DecCounter(true);
            /*if (block_->GetCounter() == 0 && block_->GetCounter(true) == 0) {
                delete block_;
            }*/
        }
        block_ = new ControlBlockPtr<T>(ptr);
        observed_ = ptr;
    }
    template <class U>
    void Reset(U* ptr) {
        if (block_) {
            block_->IncCounter(true);
            block_->DecCounter();
            block_->DecCounter(true);
            /*if (block_->GetCounter() == 0 && block_->GetCounter(true) == 0) {
                delete block_;
            }*/
        }
        block_ = new ControlBlockPtr<U>(ptr);
        observed_ = ptr;
    }
    void Swap(SharedPtr& other) {
        std::swap(block_, other.block_);
        std::swap(observed_, other.observed_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
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
        if (block_) {
            return block_->GetCounter();
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
    friend SharedPtr<U> MakeShared(Args&&... args);
    template <typename S, typename U>
    friend bool operator==(const SharedPtr<S>& left, const SharedPtr<U>& right);

private:
    template <typename Y>
    void InitWeakThis(EnableSharedFromThis<Y>* e) {
        e->weak_this_ = *this;
    }
    template <class U>
    friend class SharedPtr;
    template <class U>
    friend class WeakPtr;
    ControlBlockBase* block_;
    T* observed_;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return (left.block_ == right.block_);
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    SharedPtr<T> ans;
    auto new_block = new ControlBlockBuffer<T>(std::forward<Args>(args)...);
    ans.block_ = static_cast<ControlBlockBase*>(new_block);
    ans.observed_ = new_block->GetObserved();
    if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
        ans.InitWeakThis(ans.observed_);
    }
    return ans;
}
