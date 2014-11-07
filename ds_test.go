package ac

import (
	// "log"
	"testing"
)

func TestHitQueue(t *testing.T) {
	q := NewHitQueue(5)
	for i := 0; i < 10; i++ {
		//        log.Println(q.heap)
		ai := AcIndexItem{score: float32(i)}
		q.UpdateTop(ai)
	}
	for !q.Empty() {
		a := q.Pop()
		_ = a
		//		log.Println(a, q)
	}

}

func TestDenseHashSet(t *testing.T) {

	// for i := 0; i < 200; i++ {
	// 	log.Println(i, normCapacity(i))
	// }

	s := NewIntHashSet(10)

	for i := uint32(1); i < 10; i++ {
		if !s.Insert(i) {
			t.Fail()
		}
		if s.Insert(i) {
			t.Fail()
		}
	}

	sh := NewStrHashSet(10)

	if !sh.Insert([]byte("abc")) {
		t.Fail()
	}

	if sh.Insert([]byte("abc")) {
		t.Fail()
	}

	if !sh.Insert([]byte("abcd")) {
		t.Fail()
	}
}

func BenchmarkHitQueue(b *testing.B) {
	q := NewHitQueue(200)
	for i := 0; i < b.N; i++ {
		q.UpdateTop(AcIndexItem{score: float32(i)})
	}
}

func BenchmarkIntHashSet(b *testing.B) {
	s := NewIntHashSet(b.N)
	for i := 0; i < b.N; i++ {
		s.Insert(uint32(i))
	}
}

func BenchmarkStrHashSet(b *testing.B) {
	s := NewStrHashSet(b.N)
	d := []byte("this is a test str abc")

	for i := 0; i < b.N; i++ {
		idx := (i % 255) % len(d)
		//        idx := i % len(d)
		d[idx] = byte((int(d[idx]) + 1) % 255)
		s.Insert(d)
	}
}
