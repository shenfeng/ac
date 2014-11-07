package ac

import (
	"bytes"
	"encoding/json"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"path"
	"sort"
	"strconv"
	"unicode/utf8"
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
	pinyin  Pinyins
}

type Item struct {
	Data  string  `json:"data"`
	Score float32 `json:"score"`
}

type AcResult struct {
	Hits  int    `json:"hits"`
	Items []Item `json:"items"`
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

func NewAcIndex(file string) (*AcIndex, error) {
	ir, err := NewReader(file)
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
		Name:    path.Base(file),
	}

	if ram > 1024*1024 {
		log.Printf("%s items %d, ram %.2fM\n", index.Name, items, float64((items*16+ram))/1024.0/1024.0)
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

func (ai *AcIndex) queryRewrite(q string) (query []byte, key []byte, check bool) {
	query = []byte(q)
	key = query
	check = true
	if ai.pinyin != nil {
		key = make([]byte, 0, len(query)*2)
		t := query
		for len(t) > 0 {
			r, size := utf8.DecodeRune(t)
			if py, ok := ai.pinyin[r]; ok {
				key = append(key, py...)
			} else {
				check = false // do not check
				key = append(key, t[:size]...)
			}
			t = t[size:]
		}
	} else {
		key = query
		check = false
	}
	return
}

type Indexes map[string]*AcIndex

func LoadIndexes(dir string, pinyins Pinyins) (Indexes, error) {
	files, err := ioutil.ReadDir(dir)
	if err != nil {
		return nil, err
	}
	r := make(Indexes, len(files))
	ch := make(chan *AcIndex, len(files))
	cnt := 0
	for _, f := range files {
		if f.Name()[0] == '.' {
			continue
		}
		cnt += 1
		go (func(f os.FileInfo) {
			idx, err := NewAcIndex(path.Join(dir, f.Name()))
			if err == nil {
				idx.pinyin = pinyins
				ch <- idx
			} else {
				log.Println("ERROR", f, err)
			}
		})(f)
	}

	for i := 0; i < cnt; i++ {
		idx := <-ch
		r[idx.Name] = idx
	}
	return r, nil
}

func (i Indexes) Search(kind string, q string, limit, offset int) AcResult {
	if ai, ok := i[kind]; ok {
		return ai.Search(q, limit, offset)
	} else {
		return AcResult{}
	}
}

func uint(s string, d int) int {
	if i, e := strconv.Atoi(s); e == nil && i > 0 {
		return i
	} else {
		return d
	}
}

func (idx Indexes) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	switch r.URL.Path {
	case "/ac":
		if e := r.ParseForm(); e == nil {
			kind, q, limit, off := r.Form.Get("kind"), r.Form.Get("q"), r.Form.Get("limit"), r.Form.Get("offset")
			if kind != "" && q != "" {
				r := idx.Search(kind, q, uint(limit, 10), uint(off, 0))
				json, err := json.Marshal(r)
				if err == nil {
					w.Header().Add("Content-Type", "application/json")
					w.Write([]byte(json))
				} else {
					w.Write([]byte(err.Error()))
				}
			}
		}
	}
}

func (ai *AcIndex) Search(q string, limit, offset int) AcResult {
	query, key, check := ai.queryRewrite(q)

	//	log.Println([]byte(query), []byte(key), check)

	lower := ai.lowerBound(key)
	up := ai.lowerBound(oneGreater(key))

	re := AcResult{Items: make([]Item, 0, limit)}
	if lower == up {
		return re
	}

	totalHits := 0
	fast := up-lower > HUGE_RESULT
	howMany := limit + offset
	var ss *StrHashSet
	if fast { // fast, not accurate hits count
		howMany = howMany*2 + 100
	} else { // hits count is accurate
		ss = NewStrHashSet(up - lower)
	}
	pq := NewHitQueue(howMany)

	for i := lower; i < up; i++ {
		item := ai.indexes[i]
		if fast && !pq.NeedUpdate(item.score) {
			continue
		}

		if !fast {
			d := getData(ai.data[item.data:])
			if !ss.Insert(d) { // duplicate
				continue
			}
			if check && !bytes.Contains(d, query) { // accurate hits count
				continue
			}
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
		if !unique.Insert(d) || (check && !bytes.Contains(d, query)) {
			continue
		}
		skip += 1
		if offset >= skip {
			continue
		}

		re.Items = append(re.Items, Item{Data: string(d), Score: tmp[i].score})
		if len(re.Items) >= limit {
			break
		}
	}
	re.Hits = totalHits
	return re
}
