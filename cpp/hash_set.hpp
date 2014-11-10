#ifndef _HASH_SET_H_
#define _HASH_SET_H_

class IndexStr {
private:
    char const *mPtr;
public:
    IndexStr() : mPtr(NULL) {
    }

    IndexStr(char const *p) : mPtr(p) {
    }

    unsigned int hash_code() const {
        char const *p = mPtr;
        unsigned long hash = 5381;
        while (1) {
            int c = *p;
            if (c == 0 || c == '|') { // \0 or | is the end sign
                break;
            }
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
            ++p;
        }
        return hash;
    }

    bool is_unset() const {
        return mPtr == NULL;
    }

    bool equals(const IndexStr &o) const {
        char const *p1 = mPtr;
        char const *p2 = o.mPtr;

        while (*p1 == *p2) {
            if (*p1 == 0 || *p1 == '|') {
                return true;
            }
            ++p1;
            ++p2;
        }
        return false;
    }
};

#define JUMP_(key, num_probes)    ( num_probes )
//#define JUMP_(key, num_probes)    ( 1 )

class Int {
    int i;
public:
    Int(unsigned int i) : i(i) {
    }

    Int() : i(0) {
    }

    unsigned int hash_code() const {
        return i;
    }

    bool is_unset() const {
        return i == 0;
    }

    bool equals(const Int &o) const {
        return i == o.i;
    }
};

template<class T>
class dense_hash_set {
public:
    typedef unsigned int size_type;
    typedef T item_t;
    item_t *_table;
    size_type _num_buckets;

    dense_hash_set(size_type capacity) : _num_buckets(min_buckets(capacity)) {
        _table = new item_t[_num_buckets];
        //        _table = (item_t *) malloc(_num_buckets * sizeof(item_t));
        //std::pair<int, int> p = {0, 0};
        //std::uninitialized_fill(_table, _table + _num_buckets, p);
        //        memset(_table, 0, _num_buckets * sizeof(item_t));
    }

    ~dense_hash_set() {
        delete[]_table;
    }

    // return true if inserted, else the set contains this value
    bool insert(const item_t &value) {
        const size_type bucket_count_minus_one = _num_buckets - 1;
        size_type num_probes = 0;
        size_type bucknum = value.hash_code() & bucket_count_minus_one;

        // // give compiler a chance to unroll the loop
        // for (int i = 0; i < 3; ++i) {
        //     const item_t &item = _table[bucknum];
        //     if (item.is_unset()) {
        //         _table[bucknum] = value;
        //         return true;
        //     } else if (value.equals(item)) {
        //         return false;
        //     }
        //     ++num_probes;
        //     bucknum = (bucknum + JUMP_(key, num_probes)) & bucket_count_minus_one;
        // }

        while (1) {
            const item_t &item = _table[bucknum];
            if (item.is_unset()) {
                _table[bucknum] = value;
                return true;
            } else if (value.equals(item)) {
                return false;
            }
            ++num_probes;
            bucknum = (bucknum + JUMP_(key, num_probes)) & bucket_count_minus_one;
        }
    }

private:
    size_type min_buckets(size_type v) { // expected
        v = int(1.3 * v);
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;
        return v;
    }

    // Copy Constructor
    dense_hash_set(const dense_hash_set &other) = delete;

    // Copy Assignment Operator
    dense_hash_set &operator=(const dense_hash_set &other) = delete;
};

#undef JUMP_


#endif /* _HASH_SET_H_ */
