package ac

import (
	"log"
	"testing"
)

const (
	//	TESTFILE = "data/company_tiny"
	TESTFILE = "data/company_tiny"
)

func TestDecode(t *testing.T) {
	it, err := NewReader(TESTFILE)
	if err != nil {
		log.Fatal(err)
	}

	for {
		ai, f := it.Next()
		if f {
			break
		}
		_ = ai
		// log.Println(ai)
	}
}

func BenchmarkDecode(b *testing.B) {
	it, err := NewReader(TESTFILE)
	if err != nil {
		log.Fatal(err)
	}

	for i := 0; i < b.N; i++ {
		ai, f := it.Next()
		if f {
			it.Reset()
		}
		_ = ai
	}
}

func BenchmarkSearchIndex(b *testing.B) {
	ir, err := NewAcIndex(TESTFILE)
	if err != nil {
		log.Fatal(err)
	}
	py, _ := LoadPinyins("scripts/pinyins")
	ir.pinyin = py

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		re := ir.Search("z", 10, 0)
		_ = re
	}
}

func TestSearchIndex(t *testing.T) {
	ir, err := NewAcIndex(TESTFILE)

	if err != nil {
		log.Fatal(err)
	}

	py, _ := LoadPinyins("scripts/pinyins")
	ir.pinyin = py

	re := ir.Search("z", 10, 0)
	log.Println(re)
}

func TestLoadPinyins(t *testing.T) {
	m, err := LoadPinyins("scripts/pinyins")
	if err != nil {
		log.Fatal(err)
	}
	if string(m[[]rune("世界")[0]]) != "shi" {
		log.Fatal("error")
	}
}
