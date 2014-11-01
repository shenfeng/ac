package main

import (
	"bytes"
	"log"
	"sort"
)

const (
	Discount = 0.95
)

type AcIndexItem struct {
	index uint32
	data  uint32
	show  uint32
	score float32
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
	hit   int
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
		// items += 1
		// println("--------------", len(ai.Indexes))
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

	// log.Println(ram, items, "--------------before--------------")

	ir.Reset()

	for {
		ai, f := ir.Next()
		if f {
			break
		}

		didx := index.append(ai.Show)
		// println(didx, string(index.data[didx:]), string(ai.Show))

		for _, it := range ai.Indexes {
			iidx := index.append(it.Index)
			sidx := didx
			if len(it.Check) > 0 {
				sidx = index.append(it.Check)
				// println("---------------------------------")
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
	// log.Println(len(index.data), len(index.indexes), "-----------after-----------")

	return index, nil
}

func (ai *AcIndex) append(d []byte) int {
	idx := len(ai.data)
	ai.data = append(ai.data, d...)
	ai.data = append(ai.data, '|')
	// ai.data = append(ai.data, 8)

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
	// log.Println(*ai)
	// return 1
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
	// i == j, f(i-1) == false, and f(j) (= f(i)) == true  =>  answer is i.
	return i
}

func one_greater(d []byte) []byte {
	for i := len(d) - 1; i >= 0; i-- {
		if d[i] != 255 {
			d[i] += 1
			break
		}
	}
	return d
}

func (ai *AcIndex) Search(q string, limit, off int) AcResult {
	bs := []byte(q)
	lower := ai.lowerBound(bs)
	up := ai.lowerBound(one_greater(bs))

	for _, d := range ai.indexes[lower:up] {
		log.Println(d, string(ai.data[d.data:]))
	}

	return AcResult{}
}

func main() {
	const (
		TESTFILE = "/tmp/ac_item_tmp"
	)
	ir, err := NewAcIndex(TESTFILE)
	if err != nil {
		log.Fatal(err)
	}

	ir.Search("网", 10, 0)
}
