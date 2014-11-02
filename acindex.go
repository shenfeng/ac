package main

import (
	"bytes"
	"log"
	"sort"
)

const (
	Discount    = 0.95
	DeletedItem = -1.0
	HUGE_RESULT = 50 * 1000
)

type AcIndexItem struct {
	index uint32
	data  uint32
	show  uint32
	score float32 //  deleted if score is < 0
}

type AcIndex struct {
	data    []byte
	indexes []AcIndexItem
	Name    string
}

type Item struct {
	data  string
	score float32
}

type AcResult struct {
	hits  int
	items []Item
}

type ByAlpha AcIndex

func (a ByAlpha) Len() int { return len(a.indexes) }
func (a ByAlpha) Less(i, j int) bool {
	return bytes.Compare(a.data[a.indexes[i].index:], a.data[a.indexes[j].index:]) < 0
}
func (a ByAlpha) Swap(i, j int) {
	a.indexes[i], a.indexes[j] = a.indexes[j], a.indexes[i]
}

// abc|def\0 => abc
func getData(bs []byte) []byte {
	for i, b := range bs {
		if b == '|' || b == 0 {
			return bs[:i]
		}
	}
	return bs
}

func oneGreater(d []byte) []byte {
	for i := len(d) - 1; i >= 0; i-- {
		if d[i] != 255 {
			d[i] += 1
			break
		}
	}
	return d
}

func NewAcIndex(path string) (*AcIndex, error) {
	ir, err := NewReader(path)
	if err != nil {
		return nil, err
	}

	items, ram := 0, 0
	for { // first round, compute items, and ram and reserve memory for them
		ai, f := ir.Next()
		if f { // finished
			break
		}
		ram += len(ai.Show) + 1
		for _, it := range ai.Indexes {
			ram += len(it.Index) + 1
			items += 1 + len(it.Offs)
			if len(it.Check) > 0 {
				ram += len(it.Check) + 1
			}
		}
	}

	index := &AcIndex{ // reserve memory
		data:    make([]byte, 0, ram),
		indexes: make([]AcIndexItem, 0, items),
	}

	ir.Reset()
	for {
		ai, f := ir.Next()
		if f {
			break
		}

		didx := index.append(ai.Show)
		for _, it := range ai.Indexes {
			iidx := index.append(it.Index)
			sidx := didx
			if len(it.Check) > 0 {
				sidx = index.append(it.Check)
			}
			score := float32(ai.Score)
			index.appendIdx(iidx, didx, sidx, score)
			for _, off := range it.Offs {
				score *= Discount
				index.appendIdx(iidx+int(off), didx, sidx, score)
			}
		}
	}

	sort.Sort(ByAlpha(*index))

	return index, nil
}

func (ai *AcIndex) append(d []byte) int {
	idx := len(ai.data)
	ai.data = append(ai.data, d...)
	ai.data = append(ai.data, 0)

	return idx
}

func (ai *AcIndex) appendIdx(idx, data, show int, score float32) {
	ai.indexes = append(ai.indexes, AcIndexItem{
		index: uint32(idx),
		data:  uint32(data),
		show:  uint32(show),
		score: score,
	})
}

func (ai *AcIndex) lowerBound(q []byte) int {
	i, j := 0, len(ai.indexes)
	for i < j {
		h := i + (j-i)/2 // avoid overflow when computing h
		// i ≤ h < j
		d := ai.data[ai.indexes[h].index:]
		if bytes.Compare(d, q) < 0 {
			i = h + 1
		} else {
			j = h
		}
	}
	return i
}

func (ai *AcIndex) Search(q string, limit, offset int) AcResult {
	bs := []byte(q)
	lower := ai.lowerBound(bs)
	up := ai.lowerBound(oneGreater(bs))

	re := AcResult{items: make([]Item, 0, limit)}
	if lower == up {
		return re
	}

	totalHits := 0
	fast := up-lower > HUGE_RESULT
	howMany := limit + offset
	var ss *DenseStrHashSet
	if fast { // fast but not accurate
		howMany += 200
	} else { // accurate: including hits count is accurate
		ss = NewStrHashSet(up - lower)
	}
	pq := NewHitQueue(howMany)

	for i := lower; i < up; i++ {
		item := ai.indexes[i]

		if fast && !pq.NeedUpdate(item.score) {
			continue
		}

		if !fast && !ss.Insert(getData(ai.data[item.data:])) {
			continue
		}
		totalHits += 1
		pq.UpdateTop(item)
	}

	l := howMany
	if totalHits < l {
		l = totalHits
	}

	// First, pop all the sentinel elements (there are pq.size() - totalHits).
	for i := howMany - totalHits; i > 0; i-- {
		pq.Pop()
	}

	tmp := make([]AcIndexItem, l)
	for i := l - 1; i >= 0; i-- {
		tmp[i] = pq.Pop()
	}

	skip := 0
	unique := NewStrHashSet(l)
	for i := 0; i < l; i++ {
		d := getData(ai.data[tmp[i].data:])
		if !unique.Insert(d) {
			continue
		}
		skip += 1
		if offset >= skip {
			continue
		}

		re.items = append(re.items, Item{data: string(d), score: tmp[i].score})
		if len(re.items) >= limit {
			break
		}
	}
	re.hits = totalHits
	return re
}

func main() {
	const (
		TESTFILE = "/tmp/ac_item_tmp"
	)
	ir, err := NewAcIndex(TESTFILE)
	if err != nil {
		log.Fatal(err)
	}

	log.Println(ir.Search("网", 10, 0))
}
