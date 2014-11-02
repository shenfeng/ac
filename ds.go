package main

import "bytes"

type DenseIntHashSet struct {
	table []uint32
}

func normCapacity(v int) int {
	v = int(1.3 * float64(v))
	v--
	v |= v >> 1
	v |= v >> 2
	v |= v >> 4
	v |= v >> 8
	v |= v >> 16
	v++
	// v = v * 2
	return v
}

func NewIntHashSet(v int) *DenseIntHashSet {
	v = normCapacity(v)
	return &DenseIntHashSet{
		// buckets_minus_one: v,
		table: make([]uint32, v),
	}
}

// return true if insert ok, false if the set already contains it
func (h *DenseIntHashSet) Insert(v uint32) bool {
	buckets_minus_one, num_probes := uint32(len(h.table)-1), uint32(0)

	bucknum := v & buckets_minus_one
	for {
		t := h.table[bucknum]
		if t == 0 {
			h.table[bucknum] = v
			return true
		} else if t == v {
			return false
		}

		num_probes += 1
		bucknum = (bucknum + num_probes) & buckets_minus_one
	}
}

type DenseStrHashSet struct {
	table [][]byte
	//    size int
}

func NewStrHashSet(v int) *DenseStrHashSet {
	v = normCapacity(v)
	return &DenseStrHashSet{
		table: make([][]byte, v),
	}
}

func (h *DenseStrHashSet) Insert(v []byte) bool {
	buckets_minus_one, num_probes := uint32(len(h.table)-1), uint32(0)
	hash := uint32(5381)
	for _, b := range v {
		hash = (hash << 5) + hash + uint32(b)
	}
	bucknum := hash & buckets_minus_one
	for {
		t := h.table[bucknum]
		if t == nil {
			h.table[bucknum] = v
			//            h.size += 1
			return true
		} else if bytes.Equal(t, v) {
			return false
		}

		num_probes += 1
		bucknum = (bucknum + num_probes) & buckets_minus_one
	}
}

type HitQueue struct {
	heap []AcIndexItem // heap[0] is unused
	size int
}

func NewHitQueue(size int) *HitQueue {
	h := make([]AcIndexItem, size+1)
	return &HitQueue{heap: h, size: size}
}

func (q *HitQueue) Top() AcIndexItem {
	return q.heap[1]
}

func (q *HitQueue) NeedUpdate(s float32) bool { return q.heap[1].score < s }

func (q *HitQueue) UpdateTop(a AcIndexItem) {
	if q.heap[1].score < a.score {
		q.heap[1] = a
		//		log.Println("before", a, "--------", q.heap)
		q.downHeap()
		//		log.Println("after ", a, "------", q.heap)
	}
}

func (q *HitQueue) downHeap() {
	root := 1
	left, right := root*2, root*2+1

	for left <= q.size {
		swap := root
		//            log.Println("------------", left, swap)
		if q.lessThan(left, swap) {
			swap = left
		}

		if right <= q.size && q.lessThan(right, swap) {
			swap = right
		}

		if swap != root {
			//            log.Println("swap", swap, root)
			q.heap[swap], q.heap[root] = q.heap[root], q.heap[swap]
			root = swap
			left = root << 1
			right = left + 1
		} else {
			break
		}
	}
}

func (q *HitQueue) lessThan(i, j int) bool {
	return q.heap[i].score < q.heap[j].score
}

func (q *HitQueue) Empty() bool { return q.size == 0 }

func (q *HitQueue) Pop() AcIndexItem {
	if q.size > 0 {
		r := q.heap[1]
		q.heap[1] = q.heap[q.size]
		q.size -= 1
		q.downHeap()
		return r
	} else {
		panic("Empty heap")
	}
}
