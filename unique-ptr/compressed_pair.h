#pragma once

#include <type_traits>
#include <iostream>
#include <utility>

template <class T, bool FLAG, bool = (!std::is_final_v<T> && std::is_empty_v<T>)>
class CompPairBlock {};

template <class T, bool FLAG>
class CompPairBlock<T, FLAG, true> : public T {
public:
    CompPairBlock() = default;
    CompPairBlock(T&& elem) : T(std::move(elem)){};
    CompPairBlock(const T& elem) : T(elem){};
    T& Get() {
        return *this;
    }
    const T& Get() const {
        return *this;
    }
};

template <class T, bool FLAG>
class CompPairBlock<T, FLAG, false> {
public:
    CompPairBlock() = default;
    CompPairBlock(T&& elem) : elem_(std::move(elem)){};
    CompPairBlock(const T& elem) : elem_(elem){};
    T& Get() {
        return elem_;
    }
    const T& Get() const {
        return elem_;
    }

protected:
    T elem_{};
};

template <typename F, typename S>
class CompressedPair : private CompPairBlock<F, false>, CompPairBlock<S, true> {
public:
    CompressedPair() = default;
    template <class FICT1, class FICT2>
    CompressedPair(FICT1&& fict_1, FICT2&& fict_2)
        : CompPairBlock<F, false>(std::forward<FICT1>(fict_1)),
          CompPairBlock<S, true>(std::forward<FICT2>(fict_2)){};

    F& GetFirst() {
        return CompPairBlock<F, false>::Get();
    }
    const F& GetFirst() const {
        return CompPairBlock<F, false>::Get();
    }
    S& GetSecond() {
        return CompPairBlock<S, true>::Get();
    }
    const S& GetSecond() const {
        return CompPairBlock<S, true>::Get();
    }
};
