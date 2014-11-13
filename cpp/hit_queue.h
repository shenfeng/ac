#ifndef _HIT_QUEUE_H_
#define _HIT_QUEUE_H_

#include "hash_set.hpp"

template<class T>
class hit_queue { // a min heap
public:
    typedef T item_t;

    // since size is relative small
    // we can give more space to dense_int_set
    hit_queue(int size) : size(size), keys((size + 100) * 2) {
        heap = new item_t[size + 1]; // heap[0] is not used
    }

    void UpdateTop(const item_t &t) {
        if (this->heap[1] < t) {
            keys.erase(this->heap[1].key());
            keys.insert(t.key());

            this->heap[1] = t;
            this->downHeap(1);
        }
        // printf("%.2f\n", t.score);
        // print_heap();
    }

    bool Replace(const item_t &t) {
        if (!keys.contains(t.key())) {
            return false;
        }
        for (int i = 0; i <= size; i++) {
            if (heap[i].key() == t.key()) {
                if (heap[i] < t) {
                    // printf("--replace-- %d, from %.2f, to %.2f\n", i, heap[i].score, t.score);
                    heap[i] = t;
                    this->downHeap(i);
                    // print_heap();
                }
                return true;
            }
        }
        assert(0 && "ERROR, hashtable says it contains, but actually, it does not");
        return false;
    }

    void downHeap(int root) {
        // int root = 1;
        int left = root * 2;
        int right = left + 1;
        for (; left <= this->size;) {
            int swap = root;
            if (this->heap[left] < this->heap[swap]) {
                swap = left;
            }

            if (right <= this->size && heap[right] < heap[swap]) {
                swap = right;
            }

            if (swap != root) {
                swap_data(swap, root);
                root = swap;
                left = root * 2;
                right = left + 1;
            } else {
                break;
            }
        }
    }

    T Top() {
        return heap[1];
    }

    // no delete on keys. After pop, No insert is done
    T Pop() {
        T t = heap[1];
        heap[1] = heap[size];
        this->size -= 1;
        this->downHeap(1);
        return t;
    }

    ~hit_queue() {
        delete[]heap;
    }

private:
    int size;
    item_t *heap;
    // std::unordered_set<int> keys;
    dense_int_set keys;

    // Copy Constructor
    hit_queue(const hit_queue &other) = delete;

    // Copy Assignment Operator
    hit_queue &operator=(const hit_queue &other) = delete;

    void swap_data(int i, int j) {
        const T t = heap[i];
        heap[i] = heap[j];
        heap[j] = t;
    }

    void print_heap() {
        printf("%d: ", size);
        for (int i = 1; i <= size; i++) {
            printf("%d:%7.2f, ", i, heap[i].score);
        }
        printf("\n\n");
    }
};

#endif /* _HIT_QUEUE_H_ */
