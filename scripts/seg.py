# encoding: utf8

__author__ = 'feng'

# trie and seg implementation
# --------------------------------

def build_trie(words, trie=None):
    trie = trie or {}
    for word in words:
        t = trie
        for c in word:
            if c not in t:
                t[c] = {}

            t = t[c]

        t[''] = ''  # ending flag

    # print ','.join(trie.keys())

    return trie


def forward_max_match(trie, word):
    last_word_idx = 0

    for idx, w in enumerate(word):
        if w in trie:
            trie = trie[w]
            if '' in trie:
                last_word_idx = idx
        else:
            break

    return word[:last_word_idx + 1]


import string

LETTERS = set(string.ascii_letters + '&' + string.digits)


def forward_max_cut(trie, sentence):
    r = []
    buffer = ''
    while sentence:
        w = forward_max_match(trie, sentence)
        if w in LETTERS:
            buffer += w
        else:
            if buffer:
                r.append(buffer)
                buffer = ''
            r.append(w)

        sentence = sentence[len(w):]

    if buffer:
        r.append(buffer)
    return r


def append_or_merge(result, tmp):
    if not result or len(tmp) > 1:
        result.append(''.join(tmp))
    else:
        # 单字，向前合并 => [向前合] + [并] = [向前合并]
        result[len(result) - 1] = result[len(result) - 1] + tmp[0]


special_words = set([u'有限公司', u'科技', u'公司', u'有限责任', u'办事处', u'股份有限公司', u'集团',
                     u'事务所', u'实业', u'有限',
                     u'分公司', u'网络科技', u'信息技术', u'技术公司'])


def max_cut_for_ac(trie, sentence):
    result = []
    tmp = []
    segs = forward_max_cut(trie, sentence)

    for seg in segs:
        if seg == ' ':  # ignore space
            continue

        if seg in special_words:  # 和前一个词合并
            if tmp:
                append_or_merge(result, tmp)
                tmp = []

            append_or_merge(result, [seg])
        elif len(seg) == 1:
            tmp.append(seg)
        else:
            if tmp:
                append_or_merge(result, tmp)
                tmp = []

            result.append(seg)

    if tmp:
        append_or_merge(result, tmp)

    # 对于首字为单字的，向后合并
    if len(result) > 1 and len(result[0]) == 1:
        result[1] = result[0] + result[1]
        result = result[1:]

    return result


def open_dict(p):
    words, cnt = [], 0
    # start = time.time()

    for line in open(p):
        w = line.strip().decode('utf8')
        cnt += 1
        if len(w) <= 3:
            words.append(w)

    trie = build_trie(words)
    # logging.info("load dict, %d/%d entires, in %.2fs", len(words), cnt, time.time() - start)
    return trie
