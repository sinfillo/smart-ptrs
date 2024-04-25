#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() : block_(nullptr), observed_(nullptr){};

    WeakPtr(const WeakPtr& other) {
        block_ = other.block_;
        observed_ = other.observed_;
        if (block_) {
            block_->IncCounter(true);
        }
    }
    WeakPtr(WeakPtr&& other) noexcept {
        block_ = std::move(other.block_);
        observed_ = std::move(other.observed_);
        other.block_ = nullptr;
        other.observed_ = nullptr;
    }
    template <class U>
    WeakPtr(const WeakPtr<U>& other) {
        block_ = other.block_;
        observed_ = other.observed_;
        if (block_) {
            block_->IncCounter(true);
        }
    }
    template <class U>
    WeakPtr(WeakPtr<U>&& other) noexcept {
        block_ = std::move(other.block_);
        observed_ = std::move(other.observed_);
        other.block_ = nullptr;
        other.observed_ = nullptr;
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    template <class U>
    WeakPtr(const SharedPtr<U>& other) {
        block_ = other.block_;
        observed_ = other.observed_;
        if (block_) {
            block_->IncCounter(true);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (block_ == other.block_) {
            return *this;
        }
        if (block_) {
            block_->DecCounter(true);
        }
        block_ = other.block_;
        observed_ = other.observed_;
        if (block_) {
            block_->IncCounter(true);
        }
        return *this;
    }
    WeakPtr& operator=(WeakPtr&& other) noexcept {
        if (block_ == other.block_) {
            return *this;
        }
        if (block_) {
            block_->DecCounter(true);
        }
        block_ = std::move(other.block_);
        observed_ = std::move(other.observed_);
        other.block_ = nullptr;
        other.observed_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        if (block_) {
            block_->DecCounter(true);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_) {
            block_->DecCounter(true);
        }
        block_ = nullptr;
        observed_ = nullptr;
    }
    void Swap(WeakPtr& other) {
        std::swap(block_, other.block_);
        std::swap(observed_, other.observed_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (block_) {
            return block_->GetCounter();
        }
        return 0;
    }
    bool Expired() const {
        return (UseCount() == 0);
    }
    SharedPtr<T> Lock() const {
        if (!Expired()) {
            return SharedPtr<T>(*this);
        }
        return SharedPtr<T>();
    }

private:
    template <class U>
    friend class SharedPtr;
    template <class U>
    friend class WeakPtr;
    ControlBlockBase* block_;
    T* observed_;
};
