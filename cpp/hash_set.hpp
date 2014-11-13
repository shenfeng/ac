#ifndef _HASH_SET_H_
#define _HASH_SET_H_

// class IndexStr {
// private:
//     char const *mPtr;
// public:
//     IndexStr() : mPtr(NULL) {
//     }

//     IndexStr(char const *p) : mPtr(p) {
//     }

//     unsigned int hash_code() const {
//         char const *p = mPtr;
//         unsigned long hash = 5381;
//         while (1) {
//             int c = *p;
//             if (c == 0 || c == '|') { // \0 or | is the end sign
//                 break;
//             }
//             hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
//             ++p;
//         }
//         return hash;
//     }

//     bool is_unset() const {
//         return mPtr == NULL;
//     }

//     bool equals(const IndexStr &o) const {
//         char const *p1 = mPtr;
//         char const *p2 = o.mPtr;

//         while (*p1 == *p2) {
//             if (*p1 == 0 || *p1 == '|') {
//                 return true;
//             }
//             ++p1;
//             ++p2;
//         }
//         return false;
//     }
// };

// #define JUMP_(key, num_probes)    ( num_probes )
#define JUMP_(key, num_probes)    ( 1 )

// template<class T>
// class dense_hash_set {
// public:
//     typedef T item_t;

//     dense_hash_set(size_t capacity) : _num_buckets(min_buckets(capacity)) {
//         _table = new item_t[_num_buckets];
//     }

//     ~dense_hash_set() {
//         delete[]_table;
//     }

//     // return true if inserted, else the set contains this value
//     bool insert(const item_t &value) {
//         const size_t bucket_count_minus_one = _num_buckets - 1;
//         size_t num_probes = 0;
//         size_t bucknum = value.hash_code() & bucket_count_minus_one;

//         while (1) {
//             const item_t &item = _table[bucknum];
//             if (item.is_unset()) {
//                 _table[bucknum] = value;
//                 return true;
//             } else if (value.equals(item)) {
//                 return false;
//             }
//             ++num_probes;
//             bucknum = (bucknum + JUMP_(key, num_probes)) & bucket_count_minus_one;
//         }
//     }

// private:
//     // Copy Constructor
//     dense_hash_set(const dense_hash_set &other) = delete;

//     // Copy Assignment Operator
//     dense_hash_set &operator=(const dense_hash_set &other) = delete;

//     item_t *_table;
//     size_t _num_buckets;
// };

struct bucket_pos {
    // where the object is, is ILLEGAL_BUCKET if object not found
    const size_t where;
    // 2nd where it would go if you wanted to insert it.
    // is ILLEGAL_BUCKET if it is
    const size_t insert;
    // insert on a deleted element's location
    // const bool del;
    bucket_pos(size_t w, size_t i):where(w), insert(i) { }
};

//class int_counter {
//public:
//    int_counter(size_t capacity): _size(0) {
//        _num_buckets = min_buckets(capacity);
//        _table = new int[_num_buckets];
//         c++ did not zero the array
//        std::fill(_table, _table + _num_buckets, 0);
//    }

//    size_t size() {
//        return _size;
//    }

//    void add(const int v) {
//        const size_t bucket_count_minus_one = _num_buckets - 1;
//        size_t num_probes = 0;
//        size_t bucknum = v & bucket_count_minus_one;

//        while (1) {
//            int t = _table[bucknum];
//            if (t == 0) {
//                _table[bucknum] = v;
//                _size += 1;
//                return;
//            } else if (t == v) {
//                return;
//            }
//            ++num_probes;
//            bucknum = (bucknum + JUMP_(key, num_probes)) & bucket_count_minus_one;
//        }
//    }

//private:
//    size_t _size;
//    size_t _num_buckets;
//    int *_table;

//    size_t min_buckets(size_t v) { // expected
//        v--;
//        v |= v >> 1;
//        v |= v >> 2;
//        v |= v >> 4;
//        v |= v >> 8;
//        v |= v >> 16;
//        v++;
//        return v;
//    }

//     Copy Constructor
//    int_counter(const int_counter &other) = delete;

//     Copy Assignment Operator
//    int_counter &operator=(const int_counter &other) = delete;

//};

// no resize is implemented. implement erase and contains
class dense_int_set {
public:
    dense_int_set(size_t capacity) :
        _num_deleted(0),
        _num_buckets(min_buckets(capacity)), 
                _size(0) {
        _num_buckets = min_buckets(capacity);
        _table = new int[_num_buckets];
        // c++ did not zero the array
        std::fill(_table, _table + _num_buckets, 0);
    }

    size_t size() const {
        return this->_size;
    }

    bool insert(const int v) {
        if (_num_buckets - _size - _num_deleted < _num_buckets / 8) {
            dense_int_set tmp(*this);
            std::swap(_table, tmp._table);
            _num_deleted = 0;
        }

        auto pos = find_position(v);
        if (pos.where != ILLEGAL_BUCKET) {
            return false; // already contains
        }
        _table[pos.insert] = v;
        _size += 1;
        return true;
    }

    void erase(const int v) {
        auto pos = find_position(v);
        if (pos.where != ILLEGAL_BUCKET) {
            _num_deleted += 1;
            _size -= 1;
            _table[pos.where] = -1;
        }
    }

    bool contains(const int v) const {
        return find_position(v).where != ILLEGAL_BUCKET;
    }

    ~dense_int_set() {
        delete[]_table;
    }

private:
    bucket_pos find_position(const int v) const {
        size_t num_probes = 0;
        const size_t bucket_count_minus_one = _num_buckets - 1;
        size_t bucknum = v & bucket_count_minus_one;
        size_t insert_pos = ILLEGAL_BUCKET;
        while (1) {
            if (test_empty(bucknum)) {
                if (insert_pos == ILLEGAL_BUCKET) // found no prior place to insert
                    return bucket_pos(ILLEGAL_BUCKET, bucknum);
                else
                    return bucket_pos(ILLEGAL_BUCKET, insert_pos);
            } else if (test_deleted(bucknum)) { // keep searching, but mark to insert
                if (insert_pos == ILLEGAL_BUCKET) {
                    insert_pos = bucknum;
                }
            } else if (_table[bucknum] == v) {
                return bucket_pos(bucknum, ILLEGAL_BUCKET);
            }

            ++num_probes;
            bucknum = (bucknum + JUMP_(key, num_probes)) & bucket_count_minus_one;

            assert(num_probes < _num_buckets && "Hashtable is full");
        }
    }

    bool test_empty(size_t bucknum) const {
        return _table[bucknum] == 0;
    }

    bool test_deleted(size_t bucknum) const {
        return _table[bucknum] == -1;
    }

    static const size_t ILLEGAL_BUCKET = size_t(-1);

    // copy constructor. private
    dense_int_set(const dense_int_set &other) :
            _num_deleted(0),
            _num_buckets(other._num_buckets),
            _size(0),
            _table(new int[other._num_buckets]) {
        std::fill(_table, _table + _num_buckets, 0);
        for (size_t i = 0; i < _num_buckets; i++) {
            if (!other.test_deleted(i) && !other.test_empty(i)) {
                insert(other._table[i]);
            }
        }
    }

    size_t min_buckets(size_t v) { // expected
        v += 2;
        v = int(1.33 * v); 
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;
        return v;
    }

    // Copy Assignment Operator
    dense_int_set &operator=(const dense_int_set &other) = delete;

    size_t _num_deleted;
    size_t _num_buckets;
    size_t _size;
    int *_table;
};

#undef JUMP_


#endif /* _HASH_SET_H_ */
