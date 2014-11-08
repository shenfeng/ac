# encoding: utf8

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


TRIE = seg.open_dict('words.dic')
PINYINS = dump_pinyins()


def get_item(name, id, ranking):
    item = acitem.AcItem('%s|%s' % (name, id))
    words = seg.max_cut_for_ac(TRIE, name)

    py = [''.join([(PINYINS.get(c, c)) for c in w]) for w in words]
    initials = [''.join([(PINYINS.get(c, c))[0] for c in w]) for w in words]

    item.add_index(py, ranking)
    item.add_index(initials, int(ranking * 0.9))
    return item


def gen_company_data(input, out, limit=0):
    out = open(out, 'w')
    cnt, scan = 0, 0

    for line in open(input):
        parts = line.strip().decode('utf8').split()
        scan += 1
        if len(parts) == 6:
            id, name, full_name, ugc, ranking, city = parts
            cnt += 1
            if cnt >= limit and limit:
                break

            item = get_item(name, id, int(ranking))
            out.write(item.to_bytes())

            # print '\t'.join(parts), name

    print cnt, scan


if __name__ == '__main__':
    # print get_item(u'展高电子', 103584, 5225)
    # print get_item(u'展高电子', 103584, 5225).to_bytes()
    # with open('/tmp/t', 'w') as f:
    # f.write(get_item(u'展高电子', 103584, 5225).to_bytes())
    #     f.write(get_item(u'中国软件与技术服务股份有限公司', 103584, 5225).to_bytes())

    # print '\t'.join(seg.max_cut_for_ac(TRIE, u"展高电子"))

    gen_company_data('company', '../data/company_tiny', limit=200000)


# 展高电子|103584	zgdz	5225.000000
# 中谷电子|128918	zgdz	5225.000000
# 展高电子|103584	zgdz	5500.000000
# 中谷电子|128918	zgdz	5500.000000
# 智冠电子|72964	zgdz	27267.849609
# 智冠电子|72964	zgdz	28703.000000
