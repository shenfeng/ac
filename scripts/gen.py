import acitem
import seg


def dump_pinyins():
    from xpinyin import Pinyin

    m = {}

    with open('pinyins', 'w') as f:
        for line in open(Pinyin.data_path):
            k, v = line.strip().split('\t')
            v = v.split(" ")[0].strip()[:-1].lower()


            # print k, v, unichr(int(k, 16))
            try:
                m[unichr(int(k, 16))] = v
                f.write('%s\t%s\n' % (unichr(int(k, 16)).encode('utf8'), v.encode('utf8')))
            except Exception as e:
                pass

                # m[unichr(int(k, 16)).encode('utf8')] = v.encode('utf8')
                # mu[unichr(int(k, 16))] = v.encode('utf8')
    return m


def gen_company_data(input, out, limit=0):
    out = open(out, 'w')
    cnt, scan = 0, 0
    trie = seg.open_dict('words.dic')
    pinyins = dump_pinyins()

    for line in open(input):
        parts = line.strip().decode('utf8').split()
        scan += 1
        if len(parts) == 6:
            id, name, full_name, ugc, ranking, city = parts
            cnt += 1
            if cnt >= limit and limit:
                break

            item = acitem.AcItem('%s|%s' % (name, id), int(ranking))
            words = seg.max_cut_for_ac(trie, name)

            py = [''.join([(pinyins.get(c, c)) for c in w]) for w in words]
            initials = [''.join([(pinyins.get(c, c))[0] for c in w]) for w in words]

            item.add_index(py)
            item.add_index(initials)

            out.write(item.to_bytes())

            # print '\t'.join(parts), name

    print cnt, scan


if __name__ == '__main__':

    gen_company_data('company', '../data/company_tiny', limit=200000)
