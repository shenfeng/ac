#ifndef _ACINDEX_H_
#define _ACINDEX_H_

#include "acitem.h"
#include "stdlib.h"
#include "stdio.h"
#include <algorithm>
#include "string.h"
#include "hit_queue.h"
#include "hash_set.hpp"
#include <sys/time.h>           // gettimeofday
#include <dirent.h>
#include "unordered_map"
#include "logger.hpp"

struct AcIndexItem {
    int index;
    int check;                   // check
    int show;                   // return to client
    float score;
public:
    AcIndexItem() : index(0), check(0), show(0), score(-100) {
    }

    bool operator<(const AcIndexItem &rhs) const {
        return this->score < rhs.score;
    }

    int key() const {
        return this->show;
    }
};

struct Item {
    std::string data;
    float score;
    std::string highlighted;

    Item(const char *data, float score, const std::string &h) :
            data(data), score(score), highlighted(h) {
    }
};

struct AcResult {
    size_t hits;
    std::vector<Item> items;

    AcResult() : hits(0) {
    }
};

struct AcRequest {
    const std::string q;
    const size_t limit;
    const size_t offset;
    // const bool highlight;

    AcRequest(const std::string q, size_t limit, size_t offset) :
            q(q), limit(limit), offset(offset) {
    }
};

class Watch {
private:
    double last;

    double milliTime() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000 + tv.tv_usec / 1000.0;
    }

public:
    Watch() {
        last = this->milliTime();
    }

    double Tick() {
        return this->milliTime() - this->last;
    }
};


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


class AcIndex {
    std::string name;
    Buffer data;
    std::vector<AcIndexItem> indexes;
    const Pinyins &pinyins;

    std::string highlight(const std::string &origin, const std::string &query);

    void replaceAll(std::string &str, const std::string &from, const std::string &to) {
        if (from.empty())
            return;
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        }
    }

    std::pair<std::string, bool> queryRewrite(std::string input) {
        std::string result;
        result.reserve(input.size() * 2);

        replaceAll(input, "&middot;", "·");
        replaceAll(input, "&amp;", "&");
        replaceAll(input, "&shy;", "");
        replaceAll(input, "&gt;", ">");
        replaceAll(input, "&lt;", "<");

        
        bool is_all = true; // 全是汉字，需要判断is substring
        const char *q = input.data();
        while (*q) {
            auto r = pinyins.Get(q);
            if (r.first != NULL) {
                result.append(r.first);
            } else if (r.second == 1) {
                is_all = false;
                if (!std::isspace(*q) && *q != '\'') {
                    result.push_back(std::tolower(*q));
                }
            } else {
                is_all = false;
                result.append(q, r.second);
            }
            q += r.second;
        }
        return std::make_pair(result, is_all);
    }

public:
    AcIndex(const Pinyins &pinyins) : pinyins(pinyins) {
    }

    std::string GetName() const {
        return this->name;
    }

    int Open(std::string path);

    void Search(const AcRequest &req, AcResult &resp);
};


#endif /* _ACINDEX_H_ */
