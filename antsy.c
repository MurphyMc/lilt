#define _GNU_SOURCE
#include "SDL.h"
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "libtmt/tmt.h"

#define DEFAULT_TITLE "Antsy"

#define DEF_FG 15
#define DEF_BG 0

int term_w = 80;
int term_h = 25; // vt52 was 24, but DOS machines usually 25

#include "font.h"
#include "palette.h"

// Actual colors to use
SDL_Color colors [16];

// Current colors
SDL_Color pal [2] = {0}; // 0 is background, 1 is current foreground

static void terminal_callback (tmt_msg_t m, TMT *vt, const void *a, void *p);
static void draw_cursor (const TMTSCREEN * s, const TMTPOINT * p, bool enabled);


SDL_Surface * screen;
SDL_Surface * font;

TMT * vt = 0;

// It'd be nice if we could disable mouse by default, but we don't seem
// to get the mode enabled.  Probably because the ansi termcap doesn't
// know anything about it?
static bool mouse_enabled = true;
static bool cursor_enabled = true;

int master_fd = -1;


static void send_data (const char * key)
{
  if (master_fd < 0) return;

  size_t len = strlen(key);

  if (write(master_fd, key, len) != len)
  {
    perror("Couldn't send data");
    //TODO: Buffer this!
  }
}

static void handle_sdl_keypress (SDL_KeyboardEvent * event)
{
  bool shift = (event->keysym.mod & (KMOD_LSHIFT|KMOD_RSHIFT));
  char * key = NULL;
  switch (event->keysym.sym)
  {
    case SDLK_UP:
      key = TMT_KEY_UP;
      break;
    case SDLK_DOWN:
      key = TMT_KEY_DOWN;
      break;
    case SDLK_RIGHT:
      key = TMT_KEY_RIGHT;
      break;
    case SDLK_LEFT:
      key = TMT_KEY_LEFT;
      break;
    case SDLK_HOME:
      key = TMT_KEY_HOME;
      break;
    case SDLK_END:
      //key = TMT_KEY_END;
      key = "\033OF";
      break;
    case SDLK_INSERT:
      key = TMT_KEY_INSERT;
      break;
    case SDLK_BACKSPACE:
      //key = TMT_KEY_BACKSPACE;
      key = "\x7f";
      break;
    case SDLK_ESCAPE:
      key = TMT_KEY_ESCAPE;
      break;
    case SDLK_PAGEUP:
      key = TMT_KEY_PAGE_UP;
      break;
    case SDLK_PAGEDOWN:
      key = TMT_KEY_PAGE_DOWN;
      break;
    case SDLK_F1:
      key = TMT_KEY_F1;
      break;
    case SDLK_F2:
      key = TMT_KEY_F2;
      break;
    case SDLK_F3:
      key = TMT_KEY_F3;
      break;
    case SDLK_F4:
      key = TMT_KEY_F4;
      break;
    case SDLK_F5:
      key = TMT_KEY_F5;
      break;
    case SDLK_F6:
      key = TMT_KEY_F6;
      break;
    case SDLK_F7:
      key = TMT_KEY_F7;
      break;
    case SDLK_F8:
      key = TMT_KEY_F8;
      break;
    case SDLK_F9:
      key = TMT_KEY_F9;
      break;
    case SDLK_F10:
      key = TMT_KEY_F10;
      break;

    case SDLK_DELETE:
      key = "\x1b[3~";
      break;

    case SDLK_TAB:
      if (shift)
      {
        key = TMT_KEY_BACK_TAB;
      }
      break;
    default:
      break;
  }
  char ch[2] = {0};
  if (key == NULL)
  {
    if ((event->keysym.unicode & 0xFF80) == 0)
    {
      ch[0] = event->keysym.unicode & 0x7f;
      key = ch;
    }
  }
  if (key) send_data(key);
}


static char * shell[] = {"/bin/sh", 0};


static bool init_master (char * argv0, char * cshell, char ** run_cmd)
{
  //TODO: Proper handling of /etc/shells and so forth
  char * nshell = getenv("SHELL");
  if (nshell && strlen(nshell)) shell[0] = nshell;
  if (cshell && strlen(cshell)) shell[0] = cshell;

  char ** cmd = shell;
  if (run_cmd && run_cmd[0]) cmd = run_cmd;

  int cmd_len = 0;
  while (cmd[cmd_len]) cmd_len++;

  master_fd = posix_openpt(O_RDWR);
  if (master_fd < 0) return false;
  if (grantpt(master_fd)) return false;
  if (unlockpt(master_fd)) return false;

  int slave_fd = open(ptsname(master_fd), O_RDWR);
  if (slave_fd < 0) return false;

  char ** argv = calloc(6 + cmd_len, sizeof(char *));
  int a = 0;
  argv[0] = argv0;
  argv[1] = "-d";
  if (asprintf(&argv[2], "%ix%i", term_w, term_h) == -1) return false;
  argv[3] = "-s";
  if (asprintf(&argv[4], "%i,%i", slave_fd, master_fd) == -1) return false;
  for (int i = 0; i < cmd_len; i++) argv[5+i] = cmd[i];

  if (vfork() == 0)
  {
    execv(argv0, argv);
    _exit(1); // Shouldn't be reached!
  }

  // Parent
  free(argv[2]);
  free(argv[4]);
  free(argv);

  close(slave_fd);

  return true;
}

void init_slave (char * arg, char * argv[])
{
  setenv("TERM", "ansi", 1);
  setenv("LANG", "", 1);

  char buf[10];
  sprintf(buf, "%i", term_h);
  setenv("LINES", buf, 1);
  sprintf(buf, "%i", term_w);
  setenv("COLS", buf, 1);

  int slave_fd = -1;
  if (sscanf(arg, "%i,%i", &slave_fd, &master_fd) != 2) exit(2);
  if (master_fd < 3 || slave_fd < 3) exit(3);

  close(master_fd);
  dup2(slave_fd, 0);
  dup2(slave_fd, 1);
  dup2(slave_fd, 2);

  setsid();

  ioctl(slave_fd, TIOCSCTTY, 1);

  struct winsize ws = {0};
  ws.ws_row = term_h;
  ws.ws_col = term_w;
  ws.ws_xpixel = term_w * FONT_W;
  ws.ws_ypixel = term_h * FONT_H;
  ioctl(slave_fd, TIOCSWINSZ, &ws);

  close(slave_fd);

  execvp(argv[0], &argv[0]);
  exit(1); // Shouldn't be reached!
}


static bool set_dimensions (const char * d)
{
  int w = -1, h = -1;
  if (sscanf(d, "%i,%i", &w, &h) != 2)
  {
    if (sscanf(d, "%ix%i", &w, &h) != 2)
    {
      return false;
    }
  }

  if (w < 5) w = 5;
  if (h < 5) h = 5;

  term_w = w;
  term_h = h;

  return true;
}


int main (int argc, char * argv[])
{
  int cursor_blink_delay = 0; // 0 is disable

  int opt_count = 0;
  int opt_e_index = -1;
  char * title = DEFAULT_TITLE;
  int opt;
  while ((opt = getopt(argc, argv, "+s:et:d:b:")) != -1)
  {
    opt_count++;
    switch (opt)
    {
      case 's':
        init_slave(optarg, argv+optind);
        break;
      case 'e':
        opt_e_index = opt_count;
        break;
      case 't':
        title = optarg;
        break;
      case 'b':
        cursor_blink_delay = atoi(optarg);
        if (cursor_blink_delay < 0) cursor_blink_delay = 0;
        break;
      case 'd':
        set_dimensions(optarg);
        break;
    }
    if (opt_e_index != -1) break;
  }

  char ** run_cmd = NULL;
  char * cshell = NULL;
  if (opt_e_index == opt_count)
  {
    // Given a command to run
    run_cmd = argv + opt_e_index + 1;
  }
  else if (optind == argc - 1)
  {
    cshell = argv[optind];
  }
  //printf("argc:%i optind:%i opt_count:%i opt_e_index:%i\n",
  //       argc, optind, opt_count, opt_e_index);

  for (int i = 0; i < 16; i++)
  {
    colors[i].b = (palette_raw[i] >>  0) & 0xff;
    colors[i].g = (palette_raw[i] >>  8) & 0xff;
    colors[i].r = (palette_raw[i] >> 16) & 0xff;
    colors[i].unused = 0;
  }

  pal[0] = colors[DEF_FG];
  pal[1] = colors[DEF_BG];

  SDL_Init(SDL_INIT_VIDEO);

  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
  SDL_EnableUNICODE(1);

  SDL_WM_SetCaption(title, NULL);

  screen = SDL_SetVideoMode(FONT_W * term_w, FONT_H * term_h, 32, 0);

  font = SDL_CreateRGBSurfaceFrom(font_raw, FONT_W, FONT_H * 256, 8, FONT_W, 0, 0, 0, 0);

  vt = tmt_open(term_h, term_w, terminal_callback, NULL, NULL);

  if (!screen || !font || !vt) return 1;

  if (!init_master(argv[0], cshell, run_cmd)) return 2;

  SDL_SetColors(font, pal, 0, 2);
  SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, colors[0].r, colors[0].g, colors[0].b));
  SDL_Flip(screen);

  tmt_write(vt, "", 0);
  //tmt_write(vt, "\x1b[35mHello,\x1b[0m world!", 0);

  bool cursor_blink = false;
  uint32_t cursor_blink_at = SDL_GetTicks();
  //int cursor_blink_delay = 0; //300;

  bool quitting = false;

  const int max_sdl_delay = 100; // Max time between SDL polls
  const int sdl_boost_time = 600; // How long to boost SDL polls after SDL event

  bool sdl_boosting = false; // Currently boosting SDL?
  uint32_t sdl_boost_start; // Time boosting starting

  // We sometimes delay the mouse being active so that clicking
  // on an inactive window doesn't send a mouse event.
  const int mouse_active_delay = 300;
  uint32_t mouse_active_at = 0;
  bool mouse_active = false;

  while (!quitting)
  {
    bool got_sdl_event = false;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      sdl_boost_start = SDL_GetTicks();
      sdl_boosting = true;

      if (event.type == SDL_QUIT)
      {
        quitting = true;
        break;
      }
      else if (event.type == SDL_KEYDOWN)
      {
        handle_sdl_keypress(&event.key);
      }
      else if (event.type == SDL_ACTIVEEVENT)
      {
        mouse_active = false;
        if (event.active.state == SDL_APPINPUTFOCUS && event.active.gain)
        {
          mouse_active_at = SDL_GetTicks() + mouse_active_delay;
        }
      }
      else if (event.type == SDL_MOUSEBUTTONDOWN)
      {
        if (mouse_enabled && mouse_active)
        {
          int button = 0;
          switch (event.button.button)
          {
            case SDL_BUTTON_RIGHT: button = 1; break;
            case SDL_BUTTON_MIDDLE: button = 2; break;
            case SDL_BUTTON_WHEELDOWN: button = 65; break;
            case SDL_BUTTON_WHEELUP: button = 66; break;
          }

          int x = event.button.x / FONT_W;
          int y = event.button.y / FONT_H;
          char * buf;

          // See ctlseqs in xterm manual

          //if (asprintf(&buf, "\x1b[%i;%i;%i%c", button, x, y,
          //    (event.type == SDL_MOUSEBUTTONDOWN) ? 'M' : 'm') > 0)

          // X10-style mouse
          if (asprintf(&buf, "\x1b[M%c%c%c", button+32, x+1+32, y+1+32) > 0)
          {
            send_data(buf);
            // printf("ESC%s\n", buf+1);
            free(buf);
          }
        }
      }
    }

    if (quitting) break;

    uint32_t now = SDL_GetTicks();
    if (!mouse_active && (mouse_active_at < now))
    {
      mouse_active = true;
    }
    if (cursor_blink_delay && (now >= cursor_blink_at))
    {
      cursor_blink_at = now + cursor_blink_delay;
      cursor_blink = !cursor_blink;
      const TMTSCREEN * s = tmt_screen(vt);
      const TMTPOINT * c = tmt_cursor(vt);
      draw_cursor(s, c, cursor_blink);
      SDL_Flip(screen);
      //printf(cursor_blink?"blink\n":"BLINK\n");
    }

    int delay = max_sdl_delay;
    if (sdl_boosting)
    {
      int boosting = SDL_GetTicks() - sdl_boost_start;
      if (boosting > sdl_boost_time)
      {
        boosting = false;
      }
      else
      {
        delay = boosting << 2;
        if (delay < 10) delay = 10;
        else if (delay > max_sdl_delay) delay = max_sdl_delay;
      }
    }
    if (cursor_blink_delay && (delay > (cursor_blink_at-now)))
    {
      delay = cursor_blink_at - now;
    }

    fd_set fd_in;
    FD_ZERO(&fd_in);
    FD_SET(master_fd, &fd_in);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = delay * 1000;
    //printf("%i\n", delay);

    int rv = select(master_fd+1, &fd_in, NULL, NULL, &tv);
    if (rv == 0)
    {
      // Timeout -- that's fine
    }
    else if (rv == -1)
    {
      if (errno == EINTR)
      {
        quitting = true;
        break;
      }
      perror("select()");
    }
    else
    {
      static char buf[2048];
      rv = read(master_fd, buf, sizeof(buf));
      if (rv > 0)
      {
        tmt_write(vt, buf, rv);
        #if 0
        for (int i = 0; i < rv; i++)
        {
          char d[10] = {0};
          char c = buf[i];
          d[0] = c;
          if (c == '\x1b') strcpy(d, "<ESC>");
          write(2, d, strlen(d));
        }
        #endif
      }
      else
      {
        // Child terminated?
        quitting = true;
      }
    }
  }

  SDL_FreeSurface(font);
  SDL_Quit();
  return 0;
}


static int last_fg = -100; // Invalid
static int last_bg = -100;


static inline void draw_cell (size_t x, size_t y, TMTCHAR * c)
{
  SDL_Rect srcrect = {0,0,FONT_W,FONT_H};
  SDL_Rect dstrect = {0,0,FONT_W,FONT_H};
  SDL_Rect underrect = {0,0,FONT_W,1};

  int fg = c->a.fg-1;
  if (fg < 0) fg = DEF_FG;
  if (c->a.bold && fg < 8) fg += 8;
  //TODO: dim
  int bg = c->a.bg-1;
  if (bg < 0) bg = DEF_BG;
  if (c->a.blink)
  {
    // Draw blink as bright background
    // Maybe should do real blink?
    bg += 8;
    if (fg == bg && !c->a.invisible) fg -= 8;
  }
  if (c->a.reverse)
  {
    int tmp = fg;
    fg = bg;
    bg = tmp;
  }
  if (c->a.invisible) fg = bg;

  if (fg != last_fg || bg != last_bg)
  {
    pal[0] = colors[bg];
    pal[1] = colors[fg];
    //printf("fg:%i bg:%i pos:%lu,%lu\n", fg, bg, x, y);
    SDL_SetColors(font, pal, 0, 2);
    last_fg = fg;
    last_bg = bg;
  }

  dstrect.x = FONT_W * x;
  dstrect.y = FONT_H * y;
  srcrect.y = FONT_H * (c->c & 0xff);

  SDL_BlitSurface(font, &srcrect, screen, &dstrect);

  if (c->a.underline)
  {
    underrect.x = dstrect.x;
    underrect.y = dstrect.y + FONT_H - 2;
    SDL_FillRect(screen, &underrect, SDL_MapRGB(screen->format, colors[fg].r, colors[fg].g, colors[fg].b));
  }
}


static void draw_cursor (const TMTSCREEN * s, const TMTPOINT * p, bool enabled)
{
  static int oldx = -1, oldy = -1;
  if ((oldx != p->c) || (oldy != p->r))
  {
    if (oldx != -1)
    {
      TMTCHAR * c = &(s->lines[oldy]->chars[oldx]);
      draw_cell(oldx, oldy, c);
    }
    oldx = p->c;
    oldy = p->r;
  }

  // Always draw the character; that way the colors are correct.
  TMTCHAR * c = &(s->lines[p->r]->chars[p->c]);
  //printf("%p %p %p\n", p, s, c);
  draw_cell(p->c, p->r, c);

  if (enabled && cursor_enabled)
  {
    SDL_Rect underrect = {0,0,FONT_W,2};
    underrect.x = p->c * FONT_W;
    underrect.y = p->r * FONT_H + FONT_H - 2;
    SDL_FillRect(screen, &underrect, SDL_MapRGB(screen->format, colors[last_fg].r, colors[last_fg].g, colors[last_fg].b));
  }
}


void terminal_callback (tmt_msg_t m, TMT *vt, const void *a, void *p)
{
  /* grab a pointer to the virtual screen */
  const TMTSCREEN * s = tmt_screen(vt);

  switch (m){
    case TMT_MSG_BELL:
    {
      //TODO: Invert screen or something
      break;
    }

    case TMT_MSG_UPDATE:
    {
      bool do_cursor = false;
      const TMTPOINT * c = tmt_cursor(vt);
      for (size_t y = 0; y < s->nline; y++)
      {
        if (!s->lines[y]->dirty) continue;
        if (y == c->r) do_cursor = true;
        for (size_t x = 0; x < s->ncol; x++)
        {
          TMTCHAR * ch = &(s->lines[y]->chars[x]);
          draw_cell(x, y, ch);
        }
      }

      if (do_cursor)
      {
        // Always update cursor if current line is dirty, because
        // deletions will redraw the line without moving cursor.
        draw_cursor(s, c, true);
      }

      tmt_clean(vt);
      SDL_Flip(screen);
      break;
    }

    case TMT_MSG_ANSWER:
    {
      send_data((const char *)a);
      break;
    }

    case TMT_MSG_MOVED:
    {
      draw_cursor(s, (TMTPOINT*)a, true);
      SDL_Flip(screen);
      break;
    }

    case TMT_MSG_CURSOR:
    {
      cursor_enabled = *(char*)a == 't';
      break;
    }

#ifdef ANTSY_TITLE_SET
    case TMT_MSG_TITLE:
    {
      char * title = (char*)a;
      if (strlen(title) == 0) title = DEFAULT_TITLE;
      //TODO: It'd be nicer to go back to title set by -t (if any).
      SDL_WM_SetCaption(title, NULL);
      break;
    }
#endif

#if 0
    case TMT_MSG_X10MOUSE:
    {
      mouse_enabled = *(char*)a == 't';
      break;
    }
#endif
    default:
      break;
  }
}
