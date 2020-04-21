#pragma once

#include <utility>
#include <algorithm>

namespace vuk {
    // https://gist.github.com/ThePhD/8153067
    template<typename T, std::size_t n, std::size_t a = std::alignment_of<T>::value>
    class fixed_vector {
    public:
        typedef T value_type;
        typedef T& reference;
        typedef const T& const_reference;
        typedef T* pointer_type;
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        typedef pointer_type iterator;
        typedef const pointer_type const_iterator;

    private:
        typename std::aligned_storage<sizeof(T)* n, a>::type items;
        std::size_t len;

        T* ptrat(std::size_t idx) {
            return static_cast<T*>(static_cast<void*>(&items)) + idx;
        }

        const T* ptrat(std::size_t idx) const {
            return static_cast<const T*>(static_cast<const void*>(&items)) + idx;
        }

        T& refat(std::size_t idx) {
            return *ptrat(idx);
        }

        const T& refat(std::size_t idx) const {
            return *ptrat(idx);
        }

    public:
        static const std::size_t max_size() {
            return n;
        }

        fixed_vector() : len(0) {}

        fixed_vector(std::size_t capacity) : len(std::min(n, capacity)) {}

        template<std::size_t c>
        fixed_vector(const T(&arr)[c]) : len(c) {
            static_assert(c < n, "Array too large to initialize fixed_vector");
            std::copy(std::addressof(arr[0]), std::addressof(arr[c]), data());
        }

        fixed_vector(std::initializer_list<T> initializer) : len(std::min(n, initializer.size())) {
            std::copy(initializer.begin(), initializer.begin() + len, data());
        }

        bool empty() const {
            return len < 1;
        }

        bool not_empty() const {
            return len > 0;
        }

        bool full() const {
            return len >= n;
        }

        void push_back(const T& item) {
            new(ptrat(len++)) T(item);
        }

        void push_back(T&& item) {
            new(ptrat(len++)) T(std::move(item));
        }

        template<typename... Tn>
        void emplace_back(Tn&&... argn) {
            new(ptrat(len++)) T(std::forward<Tn>(argn)...);
        }

        void pop_back() {
            T& addr = refat(--len);
            addr.~T();
        }

        void clear() {
            for (; len > 0;) {
                pop_back();
            }
        }

        std::size_t size() const {
            return len;
        }

        std::size_t capacity() const {
            return n;
        }

        void resize(std::size_t sz) {
            len = std::min(sz, n);
        }

        void resize(std::size_t sz, const value_type& value) {
            auto old_len = len;
            len = std::min(sz, n);

            std::fill(begin() + old_len, begin() + len, value);
        }

        T* data() {
            return ptrat(0);
        }

        const T* data() const {
            return ptrat(0);
        }

        T& operator[](std::size_t idx) {
            return refat(idx);
        }

        const T& operator[](std::size_t idx) const {
            return refat(idx);
        }

        T& front() {
            return refat(0);
        }

        T& back() {
            return refat(len - 1);
        }

        const T& front() const {
            return refat(0);
        }

        const T& back() const {
            return refat(len - 1);
        }

        T* begin() {
            return data();
        }

        const T* cbegin() {
            return data();
        }

        const T* begin() const {
            return data();
        }

        const T* cbegin() const {
            return data();
        }

        T* end() {
            return data() + len;
        }

        const T* cend() {
            return data() + len;
        }

        const T* end() const {
            return data() + len;
        }

        const T* cend() const {
            return data() + len;
        }

        /*
        iterator insert(const_iterator pos, const T& value);
        iterator insert(const_iterator pos, T&& value);
        iterator insert( const_iterator pos, std::initializer_list<T> ilist );
        */

        template<class InputIt>
        iterator insert(const_iterator pos, InputIt first, InputIt last) {
            auto clen = std::distance(first, last);
            if (clen == 0)
                return pos;
            std::copy(pos, pos + (len - std::distance(begin(), pos)), pos + clen);
            std::copy(first, last, pos);
            len += clen;
            return pos;
        }

        bool operator==(const fixed_vector& o) const {
            return std::equal(begin(), end(), o.begin(), o.end());
        }
    };
}