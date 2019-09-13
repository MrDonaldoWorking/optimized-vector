#ifndef OPTIMIZED_VECTOR_H
#define OPTIMIZED_VECTOR_H

#include <iostream>
#include <memory>
#include <cstring>
#include <assert.h>
#include <type_traits>

#include <stdio.h>

#define sz_(x)   (static_cast<size_t *>(x))
#define cap_(x)  (static_cast<size_t *>(x) + 1)
#define link_(x) (static_cast<size_t *>(x) + 2)
#define data_(x) (static_cast<size_t *>(x) + 3)

void assign_fs(void * data, size_t x, size_t y, size_t z) {
    *sz_(data) = x;
    *cap_(data) = y;
    *link_(data) = z;
}

template <typename T>
void try_alloc(void *& data, size_t & x) {
    try {
        data = malloc(sizeof(size_t) * 3 + sizeof(T) * x);
    } catch (...) {
        free(data);
        throw;
    }
}

template <typename T>
void destroy_all(T* data, size_t size,
                 typename std::enable_if<!std::is_trivially_destructible<T>::value>::type* = nullptr)
{
    for (size_t i = size; i != 0; --i)
        data[i - 1].~T();
}

template <typename T>
void destroy_all(T*, size_t,
                 typename std::enable_if<std::is_trivially_destructible<T>::value>::type* = nullptr)
{}

template <typename TT>
void copy_construct_all(TT* dst, TT const* src, size_t size,
                        typename std::enable_if<!std::is_trivially_copyable<TT>::value>::type* = nullptr)
{
    size_t i = 0;

    try {
        for (; i != size; ++i)
            new (dst + i) TT(src[i]);
    }
    catch (...) {
        destroy_all(dst, i);
        throw;
    }
}

template <typename TT>
void copy_construct_all(TT* dst, TT const* src, size_t size,
                        typename std::enable_if<std::is_trivially_copyable<TT>::value>::type* = nullptr)
{
    if (size != 0) {
        memcpy(dst, src, size * sizeof(TT));
    }
}

template<typename T>
struct vector {
    typedef T value_type;
    typedef T *iterator;
    typedef T const *const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    vector() noexcept : big(nullptr) {};
    vector(vector const &);

    template<typename InputIterator>
    vector(InputIterator first, InputIterator last);

    vector &operator=(vector const &);

    template<typename InputIterator>
    void assign(InputIterator, InputIterator);

    T &operator[](size_t);
    const T &operator[](size_t) const;

    T &front();
    const T &front() const;

    T &back();
    const T &back() const;

    void pop_back();
    void push_back(T);

    T *data();
    const T *data() const;

    iterator begin();
    const_iterator begin() const;

    iterator end();
    const_iterator end() const;

    reverse_iterator rbegin();
    const_reverse_iterator rbegin() const;

    reverse_iterator rend();
    const_reverse_iterator rend() const;

    bool empty() const noexcept;
    size_t size() const;
    void reserve(size_t);
    size_t capacity() const;
    void shrink_to_fit();
    void resize(size_t, T);
    void clear() noexcept;

    void *alloc(size_t);

    ~vector();

    iterator insert(const_iterator pos, T val);
    iterator erase(const_iterator pos);
    iterator erase(const_iterator first, const_iterator last);

    template <typename U>
    friend void swap(vector<U> &, vector<U> &);

private:
    union {
        T small;
        // std::shared_ptr<void *> big = nullptr;
    };
    void *big = nullptr;
    // std::shared_ptr<void *> big = nullptr;

    void unique();

    void free_data() noexcept;

    bool is_small() const;
};

template<typename T>
vector<T>::vector(vector const &other) {
    if (other.empty()) {
        big = nullptr;
    } else if (other.is_small()) {
        new(&small) T(other.small);
        big = &small;
    } else {
        big = other.big;
        ++*link_(big);
    }
}

template<typename T>
template<typename InputIterator>
vector<T>::vector(InputIterator first, InputIterator last) {
    while (first != last) {
        push_back(*first++);
    }
}

template<typename T>
vector<T> &vector<T>::operator=(const vector<T> &other) {
    if (big == other.big) {
        return *this;
    }

    free_data();
    if (other.empty()) {
        big = nullptr;
    } else if (other.is_small()) {
        new(&small) T(other.small);
        big = &small;
    } else {
        big = other.big;
        ++(*link_(big));
    }
    return *this;
}

template<typename T>
vector<T>::~vector() {
    free_data();
}

template<typename T>
template<typename InputIterator>
void vector<T>::assign(InputIterator first, InputIterator second) {
    unique();
    while (first != second) {
        push_back(*first++);
    }
}

template<typename T>
T &vector<T>::operator[](size_t sz) {
    if (is_small()) {
        assert(sz == 0);
        return small;
    }
    unique();
    return (*(reinterpret_cast<T *>(data_(big)) + sz));
}

template<typename T>
const T &vector<T>::operator[](size_t sz) const {
    if (size() == 1) {
        return small;
    }
    return *(reinterpret_cast<T *>(data_(big)) + sz);
}

template<typename T>
void vector<T>::unique() {
    if (empty() || is_small()) {
        return;
    }

    size_t sz = *sz_(big);
    size_t cap = *cap_(big);
    size_t links = *link_(big);
    // printf("sz = %zu, cap = %zu, links = %zu\n", sz, cap, links);
    if (links > 1) {
        void *new_data;
        try {
            new_data = malloc(sizeof(size_t) * 3 + sizeof(T) * cap);
        } catch (...) {
            throw;
        }
        try {
            T *nd = reinterpret_cast<T *>(data_(new_data));
            T *d = reinterpret_cast<T *>(data_(big));
            copy_construct_all(nd, d, sz);
        } catch(...) {
            throw;
        }

        *link_(big) = links - 1;

        big = new_data;
        assign_fs(big, sz, cap, 1);
    }
}

template<typename T>
void vector<T>::free_data() noexcept {
    if (is_small()) {
        small.~T();
    }
    if (!is_small() && !empty() && --*link_(big) == 0) {
        destroy_all(reinterpret_cast<T *>(data_(big)), *sz_(big));
        free(big);
    }
}

template<typename T>
bool vector<T>::is_small() const {
    return big == &small;
}

template<typename T>
bool vector<T>::empty() const noexcept {
    return big == nullptr;
}

template<typename T>
T &vector<T>::back() {
    if (is_small()) {
        return small;
    }
    return (*this)[size() - 1];
}

template<typename T>
const T &vector<T>::back() const {
    if (is_small()) {
        return small;
    }
    return (*this)[size() - 1];
}

template<typename T>
T &vector<T>::front() {
    if (is_small()) {
        return small;
    }
    return (*this)[0];
}

template<typename T>
const T &vector<T>::front() const {
    if (is_small()) {
        return small;
    }
    return (*this)[0];
}

template<typename T>
void vector<T>::push_back(T x) {
    if (empty()) {
        new(&small) T(x);
        big = &small;
    } else if (is_small()) {
        void *new_data = alloc(4);
        T *mem_pos = reinterpret_cast<T *>(data_(new_data));

        try {
            new(mem_pos) T(small);
        } catch (...) {
            free(new_data);
            throw;
        }
        try {
            new(mem_pos + 1) T(x);
        } catch (...) {
            free(new_data);
            (*mem_pos).~T();
            throw;
        }

        small.~T();
        *sz_(new_data) = 2;
        big = new_data;
    } else {
        size_t cap = capacity();
        size_t sz = size();

        unique();
        if (sz + 1 >= cap) {
            reserve(sz * 2);
        }

        try {
            new(reinterpret_cast<T *>(data_(big)) + sz) T(x);
        } catch (...) {
            throw;
        }

        *sz_(big) = sz + 1;
    }
}

template<typename T>
void vector<T>::pop_back() {
    assert(!empty());

    if (is_small()) {
        assert(size() == 1);
        small.~T();
        big = nullptr;
    } else if (size() == 2) {
        T mem = (*this)[0];
        new(&small) T(mem);
        free_data();
        big = &small;
    } else {
        unique();
        (*(reinterpret_cast<T *>(data_(big)) + --*sz_(big))).~T();
    }
}

template<typename T>
T *vector<T>::data() {
    if (is_small()) {
        return static_cast<T *>(big);
    }
    return reinterpret_cast<T *>(data_(big));
}

template<typename T>
const T *vector<T>::data() const {
    if (is_small()) {
        return static_cast<T *>(big);
    }
    return reinterpret_cast<T *>(data_(big));
}

template<typename T>
typename vector<T>::iterator vector<T>::begin() {
    unique();

    if (is_small()) {
        return static_cast<T *>(big);
    }
    return reinterpret_cast<T *>(data_(big));
}

template<typename T>
typename vector<T>::const_iterator vector<T>::begin() const {
    if (is_small()) {
        return static_cast<T *>(big);
    }
    return reinterpret_cast<T *>(data_(big));
}

template<typename T>
typename vector<T>::reverse_iterator vector<T>::rbegin() {
    return std::reverse_iterator<iterator>(end());
}

template<typename T>
typename vector<T>::const_reverse_iterator vector<T>::rbegin() const {
    return std::reverse_iterator<const_iterator>(end());
}

template<typename T>
typename vector<T>::iterator vector<T>::end() {
    unique();

    if (is_small()) {
        return static_cast<T *>(big) + 1;
    }
    return reinterpret_cast<T *>(data_(big)) + *sz_(big);
}

template<typename T>
typename vector<T>::const_iterator vector<T>::end() const {
    if (is_small()) {
        return static_cast<T *>(big) + 1;
    }
    return reinterpret_cast<T *>(data_(big)) + *sz_(big);
}

template<typename T>
typename vector<T>::reverse_iterator vector<T>::rend() {
    return std::reverse_iterator<iterator>(begin());
}

template<typename T>
typename vector<T>::const_reverse_iterator vector<T>::rend() const {
    return std::reverse_iterator<const_iterator>(begin());
}

template<typename T>
size_t vector<T>::size() const {
    if (big == nullptr) {
        return 0;
    }
    if (big == &small) {
        return 1;
    }
    return *sz_(big);
}

template<typename T>
size_t vector<T>::capacity() const {
    if (empty()) {
        return 0;
    }
    if (is_small()) {
        return 1;
    }
    return *cap_(big);
}

template<typename T>
void *vector<T>::alloc(size_t new_sz) {
    void *res = nullptr;
    try_alloc<T>(res, new_sz);
    assign_fs(res, 0, new_sz, 1);
    return res;
}

template<typename T>
void vector<T>::reserve(size_t new_sz) {
    new_sz = std::max(new_sz, (size_t) 2);

    if (empty() || is_small()) {
        if (empty()) {
            try_alloc<T>(big, new_sz);
            *sz_(big) = 0;
        } else {
            try_alloc<T>(big, new_sz);
            try {
                new(reinterpret_cast<T *>(data_(big))) T(small);
            } catch (...) {
                free(big);
                // big = &small;
                throw;
            }

            small.~T();
            *sz_(big) = 1;
        }

        *cap_(big) = new_sz;
        *link_(big) = 1;

        return;
    }

    unique();
    size_t sz = *sz_(big);
    size_t cap = new_sz;
    size_t links = *link_(big);

    void *new_data = malloc(sizeof(size_t) * 3 + sizeof(T) * new_sz);
    try {
        T *nd = reinterpret_cast<T *>(data_(new_data));
        T *d = reinterpret_cast<T *>(data_(big));
        copy_construct_all(nd, d, sz);
        destroy_all(d, sz);
    } catch (...) {
        free(new_data);
        throw;
    }

    free(big);
    big = new_data;
    assign_fs(big, sz, cap, links);
}

template<typename T>
void vector<T>::resize(size_t new_sz, T x) {
    while (size() < new_sz) {
        push_back(x);
    }
    while (size() > new_sz) {
        pop_back();
    }
    assert(size() == new_sz);
}

template<typename T>
void vector<T>::shrink_to_fit() {
    reserve(size());
}

template<typename T>
void vector<T>::clear() noexcept {
    unique();
    free_data();
}

template<typename T>
typename vector<T>::iterator
vector<T>::insert(vector::const_iterator pos, T val) {
    if (empty()) {
        assert(pos == begin());
        push_back(val);
        return begin();
    }

    size_t len = pos - begin();
//    printf("len = %zu, val = %d\n", len, (int) val);
//    printf("pos     = %d\nbegin() = %d\n", pos, begin());
    unique();
    push_back(val);

    pos = begin() + len;
    auto i = end() - 1;
    //std::cout << pos << " " << end() << " " << (end() - pos) << '\n';
    for (; i > pos; --i) {
        std::iter_swap(i, i - 1);
    }

    return begin() + len;
}

template<typename T>
typename vector<T>::iterator
vector<T>::erase(vector::const_iterator first, vector::const_iterator last) {
    unique();

    size_t len = last - first;
    size_t prev_sz = *sz_(big);
    for (auto i = (iterator) last; i < end(); ++i) {
        std::iter_swap(i, i - len);
    }

    size_t new_sz = prev_sz - len;
    iterator i = begin() + new_sz;
    while (i < end()) {
        (*(i++)).~T();
    }

//    T mem = (*this)[0];
//    switch (new_sz) {
//        case 0:
//            free(big);
//            big = nullptr;
//            break;
//
//        case 1:
//            new(&small) T(mem);
//            free_data();
//            big = &small;
//            break;
//
//        default:
//            *sz_(big) = new_sz;
//            break;
//    }
    if (new_sz == 0) {
        free(big);
        big = nullptr;
    } else if (new_sz == 1) {
        T mem = (*this)[0];
        new(&small) T(mem);
        free_data();
        big = &small;
    } else {
        *sz_(big) = new_sz;
    }

    return data() + size();
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(vector::const_iterator pos) {
    return erase(pos, pos + 1);
}

template<typename T>
void swap(vector<T> &a, vector<T> &b) {
    if (a.is_small()) {
        if (b.is_small()) {
            std::swap(a.small, b.small);
        } else if (!b.empty()) {
            try {
                new(&b.small) T(a.small);
            } catch (...) {
                throw;
            }

            a.small.~T();
            a.big = b.big;
            b.big = &b.small;
        }

        return;
    } else if (!a.empty()) {
        if (b.is_small()) {
            try {
                new(&a.small) T(b.small);
            } catch (...) {
                throw;
            }

            b.small.~T();
            b.big = a.big;
            a.big = &a.small;
        } else {
            std::swap(a, b);
        }
    } else {
        std::swap(a, b);
    }
}

template<typename T>
bool operator==(const vector<T> &a, const vector<T> &b) {
    if (a.size() != b.size()) {
        return false;
    }
    return memcmp(a.data(), b.data(), sizeof(T) * a.size()) == 0;
}

template<typename T>
bool operator<(const vector<T> &a, const vector<T> &b) {
    size_t len = std::min(a.size(), b.size());
    for (size_t i = 0; i < len; ++i) {
        if (a[i] != b[i]) {
            return a[i] < b[i];
        }
    }
    return a.size() < b.size();
}

template<typename T>
bool operator<=(const vector<T> &a, const vector<T> &b) {
    return (a < b || a == b);
}

template<typename T>
bool operator>(const vector<T> &a, const vector<T> &b) {
    return !(a <= b);
}

template<typename T>
bool operator>=(const vector<T> &a, const vector<T> &b) {
    return !(a < b);
}

template<typename T>
bool operator!=(const vector<T> &a, const vector<T> &b) {
    return !(a == b);
}

#endif //OPTIMIZED_VECTOR_H