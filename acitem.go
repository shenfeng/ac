package ac

import (
	"encoding/binary"
	// "log"
	"bufio"
	"os"
	"strings"
	"syscall"
)

type IndexItem struct {
	Index []byte
	Offs  []byte
	Check []byte
}

type AcItem struct {
	Show    []byte
	Score   uint32
	Indexes []IndexItem
}

func readIndex(buf []byte) (IndexItem, int) {
	o := buf[0] + 1       // offs offset
	co := o + buf[o] + 1  // check offset
	l := co + buf[co] + 1 // consumed byte

	// log.Println("------",
	// 	string(buf[1:o]), buf[o+1:o+1+buf[o]], string(buf[co+1:buf[co]+co+1]))

	return IndexItem{
		Index: buf[1:o],
		Offs:  buf[o+1 : o+1+buf[o]],
		Check: buf[co+1 : buf[co]+co+1],
	}, int(l)
}

type ItemReader struct {
	cache []IndexItem // reuse, avoid allocation
	buf   []byte
	off   int
}

func NewReader(file string) (*ItemReader, error) {
	f, err := os.Open(file)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	fi, err := f.Stat()
	if err != nil {
		return nil, err
	}

	buf, err := syscall.Mmap(int(f.Fd()), 0, int(fi.Size()), syscall.PROT_READ, syscall.MAP_SHARED)
	if err != nil {
		return nil, err
	}

	return &ItemReader{
		cache: make([]IndexItem, 10),
		buf:   buf,
		off:   0,
	}, nil
}

func (r *ItemReader) Reset() { r.off = 0 }
func (r *ItemReader) Close() { syscall.Munmap(r.buf) }

func (r *ItemReader) Next() (AcItem, bool) {
	buf := r.buf[r.off:]
	if len(buf) == 0 {
		return AcItem{}, true
	}

	l := int(buf[0] + 1) //  index
	ai := AcItem{
		Show:  buf[1 : buf[0]+1],
		Score: binary.LittleEndian.Uint32(buf[l:]),
	}

	// log.Println(string(ai.Show), "---------------------", ai.Score)

	is := int(buf[l+4])
	if is > cap(r.cache) { // reserve more memory
		r.cache = make([]IndexItem, is)
	}

	l = l + 4 + 1 //  + score, 4 bytes, 1 byte index size

	for i := 0; i < is; i++ {
		ii, s := readIndex(buf[l:])
		r.cache[i] = ii
		// log.Println(string(ii.Index), ii.Offs, string(ii.Check))
		l += s
	}

	ai.Indexes = r.cache[:is]
	r.off += l

	return ai, false
}

type Pinyins map[rune][]byte

func LoadPinyins(file string) (Pinyins, error) {
	f, err := os.Open(file)
	if err != nil {
		return nil, err
	}
	m := make(Pinyins, 30000)
	s := bufio.NewScanner(f)
	for s.Scan() {
		parts := strings.Split(strings.Trim(s.Text(), "\n"), "\t")
		//		r, _ := utf8.DecodeRune([]byte(parts[0]))
		m[[]rune(parts[0])[0]] = []byte(parts[1])
	}
	return m, nil
}
