#ifndef _HIT_QUEUE_H_
#define _HIT_QUEUE_H_

#include <unordered_set>

template<class T>
class hit_queue { // a min heap
public:
    typedef T item_t;

    hit_queue(int size) : size(size), keys(size*2) {
        heap = new item_t[size + 1]; // heap[0] is not used
    }

    void UpdateTop(const item_t &t) {
        if (this->heap[1] < t) {
            keys.erase(this->heap[1].key());
            keys.insert(t.key());

            this->heap[1] = t;
            this->downHeap();
        }
    }

    bool Replace(const item_t &t) {
        if (keys.find(t.key()) == keys.end()) {
            return false;
        }
        for(int i = 0; i<=size; i++) {
            if (heap[i].key() == t.key()) {
                if (heap[i] < t) {
                    heap[i] = heap[1];
                    heap[1] = t;
                    this->downHeap();
                }
                break;
            }
        }
        return true;
    }

    void downHeap() {
        int root = 1;
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
                const T t = heap[swap];
                heap[swap] = heap[root];
                heap[root] = t;
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

    T Pop() {
        T t = heap[1];
        heap[1] = heap[size];
        this->size -= 1;
        this->downHeap();
        return t;
    }

    // pop all the sentinel elements (there are pq.size() - totalHits).
    void PopSentinel(int real_hits) {
        int how_many = this->size;
        for (int i = how_many - real_hits; i > 0; i--) {
            this->Pop();
        }
    }

    ~hit_queue() {
        delete[]heap;
    }

private:
    int size;
    item_t *heap;
    std::unordered_set<int> keys;

    // Copy Constructor
    hit_queue(const hit_queue &other) = delete;

    // Copy Assignment Operator
    hit_queue &operator=(const hit_queue &other) = delete;
};

#endif /* _HIT_QUEUE_H_ */
