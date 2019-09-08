#!/usr/bin/env python
import sys

filename = sys.argv[1]
left = int(sys.argv[2])
top = int(sys.argv[3])
cellw = int(sys.argv[4])
chw = int(sys.argv[5])
cellh = int(sys.argv[6])
chh = int(sys.argv[7])

# For a packed font:
# foo.pnm 0 0 char-width char-width char-height char-height

d = open(sys.argv[1],"r").read().split("\n",2)
assert d[0] == "P1"
size = d[1].split()
iw = int(size[0])
ih = int(size[1])

chars_wide = iw / cellw
chars_high = ih / cellh

if (iw % cellw) - left >= chw: chars_wide += 1

d = d[-1].split()

COL1 = "0"#0xff000000"
COL2 = "1"#0xffffffff"

SEP = ","

import string
is_printable = lambda s: set(s).issubset(string.digits+string.letters+string.punctuation)

print "//", " ".join(sys.argv)
print "#define FONT_W", chw
print "#define FONT_H", chh
print

print "uint8_t font_raw[] = {"
assert len(d) == (iw*ih)
c = 0
for y in range(chars_high):
  for x in range(chars_wide):
    print "/* Char %3s 0x%02x %s */" % (c,c,chr(c) if is_printable(chr(c)) else "_")
    c += 1
    for yy in range(top,top+chh):
      l = []
      for xx in range(left,left+chw):
        p = (y*cellh+yy) * iw + x * cellw + xx
        assert d[p] in ("0","1")
        l.append(COL1 if d[p]=="0" else COL2)
      print SEP.join(l)+SEP
    print
print "};"
