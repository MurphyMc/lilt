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
