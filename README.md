# Lilt: The Lil Terminal

Lilt is a graphical terminal emulator that emulates an ANSI terminal with
a couple of extensions (notably, xterm-style window title setting and mouse
clicking work).  Lilt was formerly (and briefly) known as Antsy Terminal.

It utilizes Rob King's Tiny Mock Terminal Library (libtmt) -- or my fork of
it -- to do the actual terminal escape sequence parsing, and it uses SDL1.2
to do the rendering, input, etc.

![Lilt screenshot](screenshot.png)

## Features

* Very simple/lightweight
* Nice default font and color scheme
* All resources get compiled into the binary
* Good enough emulation for vim, tmux, dialog, etc.
* X10/1002/1006 mouse support (works in vim and tmux)
* Simple dependencies (just SDL and the libtmt repository)
* Good for constrained systems (uses vfork() for systems with no MMU, uses
  only a single thread, comes with a tiny font)
* Comes with a converter for PSF fonts

Lilt should be fairly portable.  It currently has been tested with Linux
and macOS (see the "Building for macOS" section for more on the latter).

## Building

Lilt can be compiled using CMake as described in the following section.
For simple scenarios, it can also be pretty trivially compiled entirely
by hand -- just compile `lilt.c` and `libtmt/tmt.c` and link them with
libSDL.

If you're building on macOS, please see the subsection below.

### CMake

You need SDL 1.2.  On Ubuntu, `apt install libsdl1.2-dev` should do it.
Lilt also now works with the SDL1.2 compatibility library, so you
could do `apt install libsdl1.2-compat` instead.

You need cmake.  On Ubuntu, `apt install cmake` should do it.

You need libtmt.  `git clone https://github.com/MurphyMc/libtmt` should
do it.  (This gets Murphy's fork, which is required for full
functionalty, and -- as of mid-2023 -- is much more recently maintained
than the original.)

Configure the project using `cmake .` or `ccmake .`  The stock libtmt
(if you're using it instead of Murphy's fork) will require you to set
the `LILT_TITLE_SET` and `TMT_UNICODE_MAP` options to False.

You should then be able to build with `cmake --build .`.

### Building for macOS

In 2020, applications using SDL were getting blank windows when compiled
on macOS Mojave.  This affected Lilt as well as other things, such as
PyGame.  Hopefully this has since been fixed, but I haven't built on
macOS for years, so I wouldn't know.  If you try it, let me know your
results!

If you *do* find that you get a blank window, however, the way I got
success was by building with an older version of the macOS SDK.  To do
this without an old version of Xcode, you can download an old version
of the command line build tools from Apple's developer website in the
["More Downloads" area](https://developer.apple.com/download/all/).
Specifically, you are looking for the "Command Line Tools (macOS 10.13)
for Xcode 10.1".  Once you've got them, mount the `.dmg` file.  Then in
a terminal in the Lilt directory, you should be able to do something like:
```
pkgutil --expand-full /Volumes/Command*Tools/*10.13*.pkg CLT10_1
SDK=$(pwd)/$(find CLT10_1 -name MacOSX.sdk)
cmake -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13 -DMAC_SDK_ROOT="$SDK" .
```

If all goes successsfully, you should now be able to do `cmake --build .`
as usual!

Another possibility is to try building your own version of SDL using a
[patch](https://github.com/joncampbell123/dosbox-x/commit/fdf6061c)
from DOSBox-X (if you do this, let me know if it works!).

But, again, I hope someone has fixed this by now.

## Commandline options

* `-t TITLE` - Sets the initial window title
* `-e CMD ARGS...` - Run command instead of a shell (use as last argument)
* `-b MSEC` - Blink cursor every MSEC milliseconds
* `-d WxH` - Set window width and height (in characters)
* `-f` - Fixed size (no resizing allowed)
* `-m` - Start with X10 mouse mode enabled
* `-M` - Start with 1006 mouse mode enabled
* `-B PCT` - Background color brightness percent (defaults to 90)

## Fonts

The font is compiled in.  If you have an image file with a font you want
to use instead, convert it to a PNM (e.g., using ImageMagick), and then
use the included `makefont.py` script to generate a new `font.h`.  Or if
you have a PSF font you want to use (commonly used for, e.g., the Linux
terminal), you can use `psf_to_font.py`, which is even easier.

The default font (`font.h`) is based on one of the X fonts (7x14 fixed
width), and is a minor superset of Code Page 858.  It's my belief that
this font is in the public domain.  Thanks go to Lars C. Hassing for
making it available in an easy-to-consume form.

The second font (`font4x8.h`) is a tiny little font derived from the one
in the Novaterm terminal emulator for the Commodore 64.  It is code page
437 with some extra graphical/symbol characters.  It's small!  Characters
are only 4x7 pixels.  Notably, this allows for a full 80x25 character
display on a 320x200 pixel screen -- perfect for making a decent terminal
on a little LCD driven by a Raspberry Pi or suchlike.  Thanks are due to
Nick Rossi for releasing Novaterm to the public domain.

## Palettes

The palette is compiled in.  It's just 16 RGB colors -- 8 "normal" colors
followed by 8 "bright" variations which are used for bold.  Optionally,
you can include 8 separate colors to be used for the background (by
default these are just the 8 normal colors, but it can be nice to
customize them so they're darker or less saturated or whatever).

Two palettes are included.  The default palette was created with Maciej
Ciemborowicz's 4bit Terminal Color Scheme Designed, which you can find
on github (https://ciembor.github.io/4bit/).  Export it in Alacritty
format, and then you can extract the colors for `palette.h` with:
```
sed -n "s/.*\(0x[^']*\).*/\1,/p" alacritty.yml | tail -16
```

Previous versions of Lilt used a palette based off of vim-hybrid by w0ng
(https://github.com/w0ng/vim-hybrid) by default.  I got it from the nice
https://terminal.sexy site, which has a lot of palettes built in and
makes it easy to edit them, so if you want to make your own palette, it's
a nice tool for doing so.  It supports exporting in Alacritty format, so
you can use the technique above to put it in the form for `palette.h`.
