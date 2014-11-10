#ifndef _ACITEM_H_
#define _ACITEM_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cctype> // isspace

#include <sys/mman.h>       //  mmap
#include <fcntl.h>          // open

#include <assert.h>    // assert
#include <string.h> // memcpy

#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include "utf8/checked.h"

class Buffer {
    char *p;
    int off;
    int end;
public:
    Buffer(char *p, int off, int len) : p(p), off(off), end(len + off) {
    }

    Buffer() : p(NULL), off(0), end(0) {
    }

    int Char() {
        assert(end - off >= 1);
        return ((unsigned char *) p)[off++];
    }

    int Copy(Buffer b) {
        assert(end - off >= b.Len() + 1);
        int o = this->off;
        memcpy(this->Get(), b.Get(), b.Len());
        this->off += b.Len();
        this->p[this->off++] = '\0';
        return o;
    }

    char *Get(int off) {
        assert(end > off);
        return p + off;
    }

    char *Get() {
        return p + off;
    }

    int End() {
        return this->end;
    }

    char operator[](int i) {
        assert(end - off >= 1);
        return p[off + i];
    }

    int Len() {
        return end - off;
    }

    bool Hasremaing() {
        return off < end;
    }

    int Int() {
        assert(end - off >= 4);
        int n = *(int *) (p + off);
        off += 4;
        return n;
    }

    Buffer Next(int n) {
        assert(end - off >= n);
        int b = off;
        off += n;
        return Buffer(p, b, n);
    }

    Buffer PrefixRead() {
        return this->Next(this->Char());
    }
};

struct IndexItem {
    Buffer index;
    int score;
    Buffer offs;
    Buffer check;
};

struct AcItem {
    Buffer show;
    std::vector<IndexItem> indexes;
};

class ItemReader {
private:
    size_t size;
    char *p;
    Buffer buffer;
public:
    ItemReader() : size(0), p(NULL) {
    }

    int Open(std::string path) {
        int fd = open(path.c_str(), O_RDONLY);
        if (fd < 0) {
            perror(path.c_str());
            return fd;
        }

        struct stat stat_buf;
        int rc = stat(path.c_str(), &stat_buf);
        if (rc < 0) {
            perror(path.c_str());
            return rc;
        }
        this->size = stat_buf.st_size;
        this->p = (char *) mmap(NULL, this->size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (this->p == MAP_FAILED) {
            perror(path.c_str());
            return -1;
        }

        this->buffer = Buffer(this->p, 0, this->size);
        return 1;
    }

    AcItem Next() {
        AcItem ai;
        ai.show = this->buffer.PrefixRead();

        int l = this->buffer.Char();
        for (int i = 0; i < l; i++) {
            IndexItem it;
            it.index = this->buffer.PrefixRead();
            it.score = this->buffer.Int();
            it.offs = this->buffer.PrefixRead();
            it.check = this->buffer.PrefixRead();
            ai.indexes.push_back(it);
        }

        return ai;
    }

    bool Hasremaing() {
        return this->buffer.Hasremaing();
    }

    void Close() {
        munmap(this->p, this->size);
    }

    void Reset() {
        this->buffer = Buffer(this->p, 0, this->size);
    }
};

// #include "iostream"

struct Pinyins {
    std::unordered_map<uint32_t, std::string> pinyins;

public:
    int Open(std::string path) {
        std::ifstream infile(path);
        if (!infile.is_open()) {
            return -1;
        }
        std::string line;
        // one item per line, seperate by \t
        while (std::getline(infile, line)) {
            const char *p = line.data();
            auto v = utf8::next(p, line.data() + line.size());
            if (p - line.data() > 1) {
                auto pos = line.find('\t');
                if (pos != line.npos) {
                    this->pinyins[v] = line.substr(pos + 1);
                }
            }
        }
        infile.close();
        return 1;
    }

    const std::pair<const char *, int> Get(const char *in) const {
        auto p = in;
        auto v = utf8::next(p, p + 4);
        auto it = pinyins.find(v);
        int size = p - in;
        if (it != pinyins.end()) {
            return std::make_pair(it->second.data(), size);
        } else {
            return std::make_pair((char *) NULL, size);
        }
    }
};

#endif /* _ACITEM_H_ */
