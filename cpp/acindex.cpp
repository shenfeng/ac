#include "acindex.h"

static const char *base_name(const char *file) {
    const char *p = strrchr(file, '/');
    if (p == NULL) {
        return file;
    } else {
        return p + 1;
    }
}

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

std::string AcIndex::highlight(const std::string &origin, const std::string &query) {
    // cout << origin << "\t" << query << endl;
    std::string pinyin, initals;
    std::vector<int> pinyin_offs, hanzi_offs;
    pinyin_offs.push_back(0);
    hanzi_offs.push_back(0);
    const char *o = origin.data();

    // 诺基亚（Nokia）: 对于（，被忽略掉，但在算汉字 offset时，应该算上
    // int pending = 0;

    while (*o) {
        auto r = pinyins.Get(o);
        auto len = r.second;
        // int len = utf8_length(o);
        // bool add = true;

        if (len > 1) {
            // auto py = pinyins->get(o);
            if (r.first != NULL) {
                pinyin_offs.push_back(pinyin_offs[pinyin_offs.size() - 1] + strlen(r.first));
                pinyin += r.first; // append whole
                initals += *r.first; // apend first
            } else {
                pinyin_offs.push_back(pinyin_offs[pinyin_offs.size() - 1] + len);
                pinyin.append(o, len);
                initals += '_';
                // add = false;
                // pending += len;
            }
        } else {
            pinyin_offs.push_back(pinyin_offs[pinyin_offs.size() - 1] + 1);

            pinyin += std::tolower(*o);
            initals += std::tolower(*o);
        }

        // if (add) {
        // hanzi_offs[hanzi_offs.size() - 1] += pending;
        hanzi_offs.push_back(hanzi_offs[hanzi_offs.size() - 1] + len);
        // pending = 0;
        // }
        // pending += pending;
        o += len;
    }

    // log_info("%s, query: %s; pinyin: %s, inital: %s",  origin.data(), query.data(),
    //          pinyin.data(), initals.data());
    // std::copy(pinyin_offs.begin(), pinyin_offs.end(), std::ostream_iterator<int>(std::cout, " "));
    // std::cout << std::endl;

    // std::copy(hanzi_offs.begin(), hanzi_offs.end(), std::ostream_iterator<int>(std::cout, " "));
    // std::cout << std::endl;

    auto idx = pinyin.find(query);
    if (idx != std::string::npos) {
        int start = idx, end = idx + query.size();
        int startIdx = -1, endIdx = -1;

        for (unsigned int i = 0; i < pinyin_offs.size(); i++) {
            int offset = pinyin_offs[i];
            if (offset >= start && startIdx < 0) {
                startIdx = i;
            }
            if (offset >= end && endIdx < 0) {
                endIdx = i;
            }
        }

        start = hanzi_offs[startIdx];
        end = hanzi_offs[endIdx];

        return origin.substr(0, start) + "<u class=h>" +
                origin.substr(start, end - start) + "</u>" + origin.substr(end);
    } else {
        idx = initals.find(query);
        if (idx != std::string::npos) {
            int start = hanzi_offs[idx], end = hanzi_offs[idx + query.size()];

            return origin.substr(0, start) + "<u class=h>" +
                    origin.substr(start, end - start) + "</u>" + origin.substr(end);
        }
    }

    return origin;
}

int AcIndex::Open(std::string path) {
    Watch watch;
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

    // skip 4 bytes
    this->data = Buffer((char *) malloc(ram + 4), 4, ram);
    this->indexes.reserve(items);
    this->name = base_name(path.data());

    ir.Reset();
    while (ir.Hasremaing()) {
        AcItem ai = ir.Next();

        AcIndexItem ii;
        ii.show = data.Copy(ai.show);
        for (auto it = ai.indexes.begin(); it != ai.indexes.end(); ++it) {
            ii.index = data.Copy(it->index);
            ii.check = ii.show;
            if (it->check.Hasremaing()) {
                ii.check = data.Copy(it->check);
            }
            ii.score = it->score;
            indexes.push_back(ii);

            int idx = ii.index;
            for (int i = 0; i < it->offs.Len(); i++) {
                ii.score *= 0.95;
                ii.index = idx + it->offs[i];
                indexes.push_back(ii);
            }
        }
    }

    ir.Close();
    std::sort(indexes.begin(), indexes.end(), ByAlphabet(this->data));

    if (ram > 1024 * 1024) {
        log_info("%s, %d items, %.2fm RAM, in %.1fms",
                this->name.c_str(), items, (items * sizeof(AcIndexItem) + ram) / 1024.0 / 1024, watch.Tick());
    }

    return 1;
}

void AcIndex::Search(const AcRequest &req, AcResult &resp) {
    Watch watch;
    ByAlphabet by(this->data);
    auto rewrite = this->queryRewrite(req.q);
    auto q = rewrite.first;
    auto low = indexes.begin(), hi = indexes.end();
    bool check = rewrite.second;

    if (q.size() > 0) {
        low = std::lower_bound(indexes.begin(), indexes.end(), q.data(), by);
        hi = std::lower_bound(indexes.begin(), indexes.end(), one_greater(q).data(), by);
    } else {
        check = false;
    }

    bool accurate = hi - low < 200000;
    dense_int_set counter(accurate ? hi - low : 10);
    size_t how_many = req.limit + req.offset;
    hit_queue<AcIndexItem> pq(how_many);

    for (auto it = low; it != hi; it++) {
        if (pq.Replace(*it)) {   // save value, but with a higher score
            continue;
        }

        if (check && strstr(data.Get(it->check), req.q.data()) == NULL) {
            continue;
        }

        if (accurate) { counter.insert(it->show); }
        resp.hits += 1;
        pq.UpdateTop(*it);
    }

    // pop all the sentinel elements (there are pq.size() - totalHits).
    for (int i = how_many - resp.hits; i > 0; i--) {
        pq.Pop();
    }

    if (how_many > resp.hits) {
        how_many = resp.hits;
    }

    std::vector<AcIndexItem> tmp(how_many);
    for (int i = how_many - 1; i >= 0; i--) {
        tmp[i] = pq.Pop();
    }
    // printf("total hits %ld, fast: %d, how_many: %d\n", total_hits, fast, how_many);
    size_t skip = 0;
    for (size_t i = 0; i < how_many; i++) {
        auto it = tmp[i];

        skip += 1;
        if (req.offset >= skip) {
            continue;
        }

        auto data = this->data.Get(it.show);
        resp.items.emplace_back(data, it.score, this->highlight(data, q));
        if (resp.items.size() >= req.limit) {
            break;
        }
    }

    if (accurate) {
        resp.hits = counter.size();
    } else {
        resp.hits /= 6;         // not accurate, just estimation
    }

    log_info("%s %d/%d %s=>a%d/c%d/%s, hit %d/%d/%d, %.1fms",
            name.data(), req.limit, req.offset,
            req.q.data(), accurate, check, q.data(),
            how_many, resp.hits, hi - low,
            watch.Tick());
}
