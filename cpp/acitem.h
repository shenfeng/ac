#ifndef _ACITEM_H_
#define _ACITEM_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cctype> // isspace

#include <sys/mman.h>       //  mmap
#include <fcntl.h>          // open

#include <vector>
#include <string>

class Buffer {
    unsigned char *p;
    int end;
    int off;
public:
    Buffer(unsigned char *p, int off, int len) : p(p), off(off), end(len + off) {
    }

    Buffer() {
    }

    char Char() {
        return p[off++];
    }

    unsigned char *Get(int off) {
        return p + off;
    }

    char operator[](int i) {
        return p[off + 1];
    }

    int Len() {
        return end - off;
    }

    bool Hasremaing() {
        return off < end;
    }

    int Int() {
        int n = *(int *) (p + off);
        off += 4;
        return n;
    }

    Buffer Next(int n) {
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
    Buffer offs;
    Buffer check;

};

struct AcItem {
    Buffer show;
    int score;
    std::vector<IndexItem> *indexes;
};

class ItemReader {
private:
    size_t size;
    unsigned char *p;
    Buffer buffer;

    std::vector<IndexItem> cache;

public:
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
        this->p = (unsigned char *) mmap(NULL, this->size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (this->p == MAP_FAILED) {
            perror(path.c_str());
            return -1;
        }

        this->buffer = Buffer(this->p, 0, this->size);
    }

    AcItem Next() {
        AcItem ai;
        ai.show = this->buffer.PrefixRead();
        ai.score = this->buffer.Int();
        this->cache.empty();

        int l = this->buffer.Char();
        for (int i = 0; i < l; i++) {
            IndexItem it;
            it.index = this->buffer.PrefixRead();
            it.offs = this->buffer.PrefixRead();
            it.check = this->buffer.PrefixRead();
            this->cache.push_back(it);
        }
        ai.indexes = &this->cache;
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

#endif /* _ACITEM_H_ */
