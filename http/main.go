package main

import (
	"flag"
	"github.com/shenfeng/ac"
	"log"
	"net/http"
	_ "net/http/pprof"
)

func main() {
	var addr, dir, pinyin string
	flag.StringVar(&addr, "addr", ":7777", "Which Addr the server listens")
	flag.StringVar(&dir, "dir", "data", "Which dir to scan to find data file")
	flag.StringVar(&pinyin, "pinyin", "pinyins", "Pinyin file path")

	flag.Parse()

	pinyins, err := ac.LoadPinyins(pinyin)
	if err != nil {
		log.Fatal(err)
	}

	indexes, err := ac.LoadIndexes(dir, pinyins)
	if err != nil {
		log.Fatal(err)
	}

	log.Println("listen at", addr)

	http.Handle("/", indexes)
	log.Fatal(http.ListenAndServe(addr, nil))
}
