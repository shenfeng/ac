# encoding: utf8
import struct

__author__ = 'feng'


class AcItem(object):
    # all input are unicode
    def __init__(self, show):
        self.show = show
        self.indexes = []

    # index: ['看准', '看准网']
    def add_index(self, index, score, check=u''):
        self.indexes.append((index, score, check))

    def to_bytes(self):
        # little endian, unsigned int
        show = self.show.encode('utf8')

        # struct.pack('<I', self.score)

        buf = chr(len(show)) + show + chr(len(self.indexes))

        for index, score, check in self.indexes:
            index = [i.encode('utf8') for i in index]
            # print index
            offs, acc = [], 0
            for i in index[:-1]:
                acc += len(i)  # ignore 0, since it's the default. ignore last
                offs.append(acc)

            # print offs
            offs = bytearray(offs)
            index = ''.join(index)
            # index+score+offs+check
            buf += chr(len(index)) + index + struct.pack('<I', score)

            check = check.encode('utf8')
            buf += chr(len(offs)) + offs + chr(len(check)) + check

            # buf += '%c%s%c%s%c%s' % (chr(len(index)), index, chr(len(offs)), offs, chr(len(check)), check)

        return buf

    def __str__(self):
        return repr(self.__dict__).decode('raw_unicode_escape').encode('utf8')


def run_test():
    with open('/tmp/ac_item_tmp', 'w') as f:
        words = [u'abc', u'about', u'看准网', u'百度云', u'互联网时代', u'火车票', u'火影忍者漫画']

        size = 0
        for idx, w in enumerate(words):
            item = AcItem(w, 1023 + idx)
            item.add_index(list(w), w if idx % 2 == 0 else '')
            # item.add_index(list(w))
            buf = item.to_bytes()
            f.write(buf)
            size += len(buf)

        print size


if __name__ == '__main__':
    pass
    # dump_pinyins()
    # run_test()
