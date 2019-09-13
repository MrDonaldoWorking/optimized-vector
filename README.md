# vector

implementation of standard vector with optimizations

```
template <typename T>
struct vector
{
    typedef ? value_type;

    typedef ? iterator;
    typedef ? const_iterator;
    typedef ? reverse_iterator;
    typedef ? const_reverse_iterator;

    vector();
    vector(vector const&);

    template <typename InputIterator>
    vector(InputIterator first, InputIterator last);

    vector& operator=(vector const&);

    template <typename InputIterator>
    void assign(InputIterator first, InputIterator last);

    operator[]
    front
    back
    push_back
    pop_back
    data

    begin
    end
    rbegin
    rend

    empty
    size
    reserve
    capacity
    shrink_to_fit
    resize
    clear

    iterator insert(const_iterator pos, T const& val);
    iterator erase(const_iterator pos);
    iterator erase(const_iterator first, const_iterator last);
};


swap
operator==
operator!=
operator<
operator<=
operator>
operator>=
```
