#!/usr/bin/env python3

import gzip

def fopen (fn):
  if fn.endswith(".gz"):
    return gzip.open(fn, 'rb')
  return open(fn, 'rb')


def readmagic (f):
  m1,m2 = f.read(2)
  if m1 == 0x36 and m2 == 0x04: return 1
  if m1 == 0x72 and m2 == 0xb5:
    m1,m2 = f.read(2)
    if m1 == 0x4a and m2 == 0x86:
      return 2
  return None

HB = 1
LB = 0

import string
is_printable = lambda s: set(s).issubset(string.digits+string.ascii_letters+string.punctuation)

mappings = [
( 0 , "RIGHT ARROW",            ">", 0x2192 ),
( 1 , "LEFT ARROW",             "<", 0x2190 ),
( 2 , "UP ARROW",               "^", 0x2191 ),
( 3 , "DOWN ARROW",             "v", 0x2193 ),
( 4 , "BLOCK",                  "#", 0x2588 ),
( 5 , "DIAMOND",                "+", 0x2666 ),
( 6 , "CHECKERBOARD",           "#", 0x2592 ),
( 7 , "DEGREE",                 "o", 0x00b0 ),
( 8 , "PLUS/MINUS",             "+", 0x00b1 ),
( 9 , "BOARD",                  ":", 0x25a6 ),
( 10, "LOWER RIGHT CORNER",     "+", 0x2518 ),
( 11, "UPPER RIGHT CORNER",     "+", 0x2510 ),
( 12, "UPPER LEFT CORNER",      "+", 0x250C ),
( 13, "LOWER LEFT CORNER",      "+", 0x2514 ),
( 14, "CROSS",                  "+", 0x253C ),
( 15, "SCAN LINE 1",            "~", 0x23BA ),
( 16, "SCAN LINE 3",            "-", 0x23BB ),
( 17, "HORIZONTAL LINE",        "-", 0x2500 ),
( 18, "SCAN LINE 7",            "-", 0x23BC ),
( 19, "SCAN LINE 9",            "_", 0x23BD ),
( 20, "LEFT TEE",               "+", 0x251C ),
( 21, "RIGHT TEE",              "+", 0x2524 ),
( 22, "BOTTOM TEE",             "+", 0x2534 ),
( 23, "TOP TEE",                "+", 0x252C ),
( 24, "VERTICAL LINE",          "|", 0x2502 ),
( 25, "LESS THAN OR EQUAL",     "<", 0x2264 ),
( 26, "GREATER THAN OR EQUAL",  ">", 0x2265 ),
( 27, "PI",                     "*", 0x03c0 ),
( 28, "NOT EQUAL",              "!", 0x2260 ),
( 29, "POUND STERLING",         "f", 0x00a3 ),
( 30, "BULLET",                 "o", 0x00b7 ),
]

def read_v1_unicode_table (f):
  tab = {}
  g = -1
  while True:
    g += 1
    uc = f.read(2)
    if not uc: return tab
    uc = uc[HB] << 8 | uc[LB]
    if uc != 0xfffe:
      #print("%02x " % (uc,), end="")
      tab[uc] = g
    while True:
      uc = f.read(2)
      if not uc: return tab
      uc = uc[HB] << 8 | uc[LB]
      if uc == 0xffff: break

def read_v2_unicode_table (f):
  # Based on https://wiki.osdev.org/PC_Screen_Font
  tab = {}
  g = 0
  s = 0
  data = f.read()
  while s < len(data):
    uc = data[s]
    if uc == 0xff:
      g += 1
      s += 1
      continue
    if uc & 128:
      if (uc & 32) == 0:
        uc = ((data[s] & 0x1F)<<6)+(data[s+1] & 0x3F)
        s += 1
      elif (uc & 16) == 0:
        uc = ((((data[s+0] & 0xF)<<6)+(data[s+1] & 0x3F))<<6)+(data[s+2] & 0x3F)
        s += 2
      elif (uc & 8) == 0:
        uc = ((((((data[s+0] & 0x7)<<6)+(data[s+1] & 0x3F))<<6)+(data[s+2] & 0x3F))<<6)+(data[s+3] & 0x3F)
        s += 3
      else:
        uc = 0
    tab[uc] = g
    s += 1
  return tab

def readint (f):
  o = 0
  vv = f.read(4)
  for v in reversed(vv):
    o <<= 8
    o |= v
  return o



def convert (f, basename, lpad=0, rpad=0,tpad=0, bpad=0,
             bgcolor=0x00000000, fgcolor=0xffffffff,
             xspc=None, yspc=None):
  version = readmagic(f)
  if version is None:
    raise RuntimeError("Bad version")

  if version == 1:
    mode,charsize = f.read(2)
    w = 8
    h = charsize
    num_chars = 256
    if mode & 1: num_chars = 512
    raw = f.read(h * num_chars)
    assert len(raw) == h * num_chars
    if mode & 2:
      tab = read_v1_unicode_table(f)
    else:
      tab = dict([(x,x) for x in range(0,256)])
  elif version == 2:
    vers2 = readint(f)
    headersize = readint(f)
    flags = readint(f)
    num_chars = readint(f)
    charsize = readint(f)
    h = readint(f)
    w = readint(f)
    f.seek(headersize)
    raw = f.read(charsize * num_chars)
    if flags & 1:
      tab = read_v2_unicode_table(f)
    else:
      tab = dict([(x,x) for x in range(0,256)])

  ow = (w+lpad+rpad) # output width
  oh = h+tpad+bpad

  alldata = []

  from array import array

  for i in range(num_chars):
    data = []
    for y in range(oh):
      data.append( [bgcolor] * ow )
    alldata.append(data)
    off = charsize * i
    xoff = lpad
    for y in range(h):
      for x in range(w):
        bbit = 1<<(7 - (x % 8))
        bbyte = x // 8
        d = raw[off + y * charsize // h + bbyte]
        if d & bbit:
          data[y+tpad][xoff + x] = fgcolor

  acs_map = []
  for index,name,char,uni in mappings:
    if uni in tab:
      c = tab[uni]
    else:
      c = ord(char)
    assert index == len(acs_map)
    acs_map.append(c)

  # Which ones aren't we going to include already?
  extra_chars = [x for x in acs_map if x >= 256]
  extra_map = {}

  all_chars = alldata[:256]
  all_map = {x:x for x in range(256)}
  for ec in extra_chars:
    all_map[ec] = len(all_chars)
    all_chars.append( alldata[ec] )

  print( "//", " ".join(sys.argv) )
  print()
  print( "#define FONT_W", w )
  print( "#define FONT_H", h )
  print()

  print( "#define FONT_ACS_CHARS 1")
  print( "#define FONT_CHAR_COUNT " + str(len(all_chars)) )
  print()

  print( "const wchar_t acs_chars[] = {" )
  for i in range(0,32,8):
    print("  "+"".join(f"0x{all_map[x]:02x}," for x in acs_map[i:i+8]))
  print( "  0 // Null terminator")
  print( "};" )
  print()

  print("uint8_t font_raw[] = {")

  #print(needed_chars)
  for c,cc in all_map.items():
    print("/* Char %3s 0x%02x %s */" % (c,c,chr(c) if is_printable(chr(c)) else "_"))
    for row in all_chars[cc]:
      print("".join(str(x)+"," for x in row))
    print()
  print( "};" )


import argparse
import os
import sys

p = argparse.ArgumentParser(prog=sys.argv[0])
p.add_argument("filename", nargs='+')
p.add_argument("-l", "--pad-left", type=int)
p.add_argument("-r", "--pad-right", type=int)
p.add_argument("-t", "--pad-top", type=int)
p.add_argument("-b", "--pad-bottom", type=int)
p.add_argument("-p", "--pad", type=int, default=None)
p.add_argument("-xs", "--x-space", default=None)
p.add_argument("-ys", "--y-space", default=None)

args = p.parse_args()

lpad = rpad = bpad = tpad = 0

if args.pad is not None:
  lpad = rpad = tpad = bpad = args.pad

if args.pad_left is not None: lpad = args.pad_left
if args.pad_right is not None: lpad = args.pad_right
if args.pad_top is not None: lpad = args.pad_top
if args.pad_bottom is not None: lpad = args.pad_bottom

bg=0
fg=1

for fn in args.filename:
  bname = fn.split(".")
  while bname:
    l = bname[-1]
    if "/" in l: break
    if l == "gz": del bname[-1]
    elif l == "psf": del bname[-1]
    elif l == "psfu": del bname[-1]
    else: break
  bname = ".".join(bname)

  convert(fopen(fn), bname,
          lpad=lpad, rpad=rpad, tpad=tpad, bpad=bpad,
          fgcolor=fg, bgcolor=bg,
          xspc=args.x_space, yspc=args.y_space)
