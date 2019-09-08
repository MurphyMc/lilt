# Antsy Term

Antsy is a graphical terminal emulator that emulates an ANSI terminal with
a couple of extensions (notably, xterm-style window title setting and mouse
clicking work).

It utilizes Rob King's Tiny Mock Terminal Library (libtmt) -- or my fork of
it -- to do the actual terminal escape sequence parsing, and it uses SDL1
to do the rendering, input, etc.

## Compiling

You need SDL 1.2.  On Ubuntu, `apt install libsdl1.2-dev` should do it.

You need cmake.  On Ubuntu, `apt install cmake` should do it.

You need libtmt.  `git clone https://github.com/MurphyMc/libtmt` should
do it.

You should then be able to build with `cmake --build .`.

If you want xterm window title setting, you'll need to enable
`ANSTY_TITLE_SET`.

## Commandline options

* `-t TITLE` - Sets the initial window title
* `-e CMD ARGS...` - Run command instead of a shell
* `-b MSEC` - Blink cursor every MSEC milliseconds
* `-d WxH` - Set window width and height (in characters)

## Fonts

The font is compiled in.  If you have an image file with a font you want
to use instead, convert it to a PNM (e.g., using ImageMagick), and then
use the included `makefont.py` script to generate a new `font.h`.

The included one is based on one of the X fonts (7x14 fixed width).  It's
my belief that this font is in the public domain.  Thanks go to Lars C.
Hassing for making it available in an easy-to-consume form.

## Palettes

The palette is compiled in.  It's just 16 RGB colors -- 8 "normal" colors
followed by 8 "bright" variations which are used for bold.  The included
palette is vim-hybrid by w0ng (https://github.com/w0ng/vim-hybrid), though
I got it from https://terminal.sexy.  If you want to make your own, it's
a nice tool for doing so.  Export it in Alacritty format, and it's then
pretty easy to put it into the right form for `palette.h`.
