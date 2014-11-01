package main

import (
	"log"
	"testing"
)

const (
	TESTFILE = "/tmp/ac_item_tmp"
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

func TestSearchIndex(t *testing.T) {
	ir, err := NewAcIndex(TESTFILE)
	if err != nil {
		log.Fatal(err)
	}

	ir.Search("网", 10, 0)

}
