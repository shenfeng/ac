#ifndef _ACINDEX_H_
#define _ACINDEX_H_

#include "acitem.h"
#include "stdlib.h"
#include "stdio.h"
#include <algorithm>
#include "string.h"
#include "hit_queue.h"
//#include <libgen.h> // basename
#include "hash_set.hpp"

struct AcIndexItem {
    int index;
    int data;
    int show;
    float score;
public:
    bool operator<(const AcIndexItem &rhs) const {
        return this->score < rhs.score;
    }
};

struct Item {
    char *data;
    float score;
};

struct AcResult {
    size_t hits;
    std::vector<Item> items;
};

struct AcReq {
    const std::string q;
    const int limit;
    const int offset;

    AcReq(std::string q, int limit, int offset) : q(q), limit(limit), offset(offset) {
    }
};

static const char *base_name(const char *file) {
    const char *p = strrchr(file, '/');
    if (p == NULL) {
        return file;
    } else {
        return p + 1;
    }
}

struct ByAlphabet {
private:
    Buffer data;
public:
    ByAlphabet(Buffer data) : data(data) {
    }

    // std::sort
    bool operator()(const AcIndexItem &lhs, const AcIndexItem &rhs) {
        assert(lhs.index < data.End() && rhs.index < data.End());
        // by alphabet
        int r = strcmp(data.Get(lhs.index), data.Get(rhs.index));
        if (r == 0) {
            return lhs.score < rhs.score; // by score desc
        }
        return r < 0;
    }

    // std::lower_bound
    bool operator()(AcIndexItem &lhs, const char *const &rhs) {
        assert(lhs.index < data.End());
        int r = strcmp(data.Get(lhs.index), rhs);
        return r < 0;
    }
};

static std::string one_greater(const std::string &str) { // copy
    std::string cq = str;
    // one more bigger
    for (auto it = cq.rbegin(); it != cq.rend(); ++it) {
        if (*it != (char) 255) {
            *it += 1;
            break;
        }
    }
    return cq;
}

class AcIndex {
    std::string name;
    Buffer data;
    std::vector<AcIndexItem> indexes;
public:
    int Open(std::string path) {
        ItemReader ir;
        int n = ir.Open(path);
        if (n <= 0) {
            return n;
        }

        int ram = 0, items = 0;
        while (ir.Hasremaing()) {
            AcItem ai = ir.Next();
            ram += ai.show.Len() + 1;
            for (auto it = ai.indexes.begin(); it != ai.indexes.end(); ++it) {
                ram += it->index.Len() + 1;
                items += 1 + it->offs.Len();
                if (it->check.Len() > 0) {
                    ram += it->check.Len() + 1;
                }
            }
        }

        this->data = Buffer((char *) malloc(ram), 0, ram);
        this->indexes.reserve(items);
        this->name = base_name(path.data());

        if (ram > 1024 * 1024) {
            printf("load %d items, %.2fm RAM, from %s\n", items,
                    (items * sizeof(AcIndexItem) + ram) / 1024.0 / 1024,
                    this->name.c_str());
        }

        ir.Reset();
        while (ir.Hasremaing()) {
            AcItem ai = ir.Next();

            AcIndexItem ii;
            ii.data = data.Copy(ai.show);
            for (auto it = ai.indexes.begin(); it != ai.indexes.end(); ++it) {
                ii.index = data.Copy(it->index);
                ii.show = ii.data;
                if (it->check.Hasremaing()) {
                    ii.show = data.Copy(it->check);
                }
                ii.score = it->score;
                indexes.push_back(ii);

                int idx = ii.index;
                for (int i = 0; i < it->offs.Len(); i++) {
                    ii.score *= 0.95;
                    ii.index = idx + it->offs[i];
//                    printf("--------------%d %d %d\n", ii.index, it->offs[i], it->offs.Len());
                    indexes.push_back(ii);
                }
            }
        }

        ir.Close();
        std::sort(indexes.begin(), indexes.end(), ByAlphabet(this->data));
        return 1;
    }

    void Search(const AcReq &req, AcResult &resp) {
        ByAlphabet by(this->data);
        const std::string &q = req.q;
        auto low = std::lower_bound(indexes.begin(), indexes.end(), q.data(), by);
        auto hi = std::lower_bound(indexes.begin(), indexes.end(), one_greater(q).data(), by);

        bool fast = hi - low > 50000;
        int how_many = req.limit + req.offset;
        if (fast) {
            how_many = how_many * 2 + 100;
        }

        hit_queue<AcIndexItem> pq(how_many);
        // fast: check result; non-fast: check every element
        dense_hash_set<IndexStr> unique(fast ? how_many : hi - low);
        size_t total_hits = 0;

        for (auto it = low; it != hi; it++) {
            if (!fast) {
                if (!unique.insert(data.Get(it->data))) {
                    continue; // duplicate
                }
                // TODO check substr if needed
            }
            total_hits += 1;
            pq.UpdateTop(*it);
        }
        pq.PopSentinel(total_hits);
        if (how_many > total_hits) {
            how_many = total_hits;
        }

        printf("total hits %d, fast: %d, how_many: %d\n", total_hits, fast, how_many);

        AcIndexItem tmp[how_many];
        for (int i = how_many - 1; i >= 0; i--) {
            tmp[i] = pq.Pop();
        }
        int skip = 0;
        for (int i = 0; i < how_many; i++) {
            auto it = tmp[i];
            if (fast && !unique.insert(data.Get(it.data))) {
//                printf("------------------\n");
                continue; // remove duplicate
            }
            skip += 1;
            if (req.offset >= skip) {
                continue;
            }

            Item item;
            item.score = it.score;
            item.data = this->data.Get(it.data);
            resp.items.push_back(item);

            printf("%s\t%s\t%f\t%s\n",
                    data.Get(it.data), data.Get(it.index), it.score, data.Get(it.show));

            if (resp.items.size() >= req.limit) {
                break;
            }
        }
    }
};

#endif /* _ACINDEX_H_ */
