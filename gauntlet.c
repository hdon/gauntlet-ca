#include <stdio.h>
#include <stdlib.h>
#include "SDL.h"
#include "SDL_image.h"
#include "noise.h"

#define SCREENW 640
#define SCREENH 480

#define TILEW 32
#define TILEH 32

#define MAPW 80
#define MAPH 60

#define MAXHEALTH 1200

#define FPS 10

typedef struct arrow_s {
  int alive;
  int x, y, xm, ym;
} arrow_t;

#define MAXARROWS 3

int safe(char map[MAPH][MAPW], int x, int y, int toggle);
int bomb(char map[MAPH][MAPW], int plyx, int plyy);
void opendoor(char map[MAPH][MAPW], int x, int y);
SDL_Surface * loadimage(char * fname);
void drawimage(SDL_Surface * screen, SDL_Surface * sheet, int tilew, int tileh, int tilex, int tiley, int x, int y);

/* audio */
SDL_AudioSpec audio;
int freq = 0;

int main(int argc, char * argv[])
{
  SDL_Surface * screen;
  int done;
  int i, j, x, y, x2, y2, found;
  int plyx, plyy, score, oldplyx, oldplyy, health, maxhealth, bombs, keys, oldhealth;
  char map[MAPH][MAPW];
  char c, c2;
  int got;
  SDL_Rect dest;
  char line[1024];
  FILE * fi;
  SDL_Event event;
  SDLKey key;
  void * dummy;
  Uint32 timestamp, nowtimestamp;
  int toggle;
  int key_up, key_down, key_left, key_right, key_fire;
  int oldkey_up, oldkey_down, oldkey_left, oldkey_right;
  int plydirx, plydiry;
  arrow_t arrows[MAXARROWS];
  SDL_Surface * img_generators, * img_collectibles, * img_blockers, * img_arrows, * img_badguys, * img_player;

  /* Initialize SDL */

  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    fprintf(stderr, "Error init'ing SDL: %s\n", SDL_GetError());
    exit(1);
  }

  /* Open a window / screen */

  screen = SDL_SetVideoMode(SCREENW, SCREENH, 0, 0);
  if (screen == NULL)
  {
    fprintf(stderr, "Error opening video: %s\n", SDL_GetError());
    SDL_Quit();
    exit(1);
  }

  /* Initialize audio */
  audio.freq = 22050;
  audio.samples = 512; /* very small buffer! */
  audio.channels = 0; /* mono sound */
  audio.format = AUDIO_U8;
  audio.callback = noise_callback;
  if (SDL_OpenAudio(&audio, NULL)) {
    fprintf(stderr, "Error opening audio: %s\n", SDL_GetError());
    SDL_Quit();
    exit(1);
  }
  SDL_PauseAudio(0);

  /* Load imagery */

  img_generators = loadimage("generators.png");
  img_collectibles = loadimage("collectibles.png");
  img_blockers = loadimage("blockers.png");
  img_arrows = loadimage("arrows.png");
  img_badguys = loadimage("badguys.png");
  img_player = loadimage("player.png");

  /* Load the map */

  fi = fopen("map.txt", "r");
  if (fi == NULL)
  {
    perror("map.txt");
    SDL_Quit();
    exit(1);
  }

  for (y = 0; y < MAPH; y++)
  {
    dummy = fgets(line, 1024, fi);

    for (x = 0; x < MAPW; x++)
    {
      map[y][x] = line[x];
    }
  }

  /* Set initial game state: */

  plyx = 1;
  plyy = 1;
  map[plyy][plyx] = ' ';

  done = 0;
  toggle = 0;
  key_up = key_down = key_left = key_right = key_fire = 0;
  score = 0;
  bombs = 0;
  keys = 0;
  maxhealth = MAXHEALTH / 4;
  health = maxhealth;

  /* Start out facing down */
  plydirx = 2;
  plydiry = 3;

  for (i = 0; i < MAXARROWS; i++)
    arrows[i].alive = 0;


  /* Main loop: */

  do
  {
    timestamp = SDL_GetTicks();
    toggle++;

    if (toggle % 2 == 0)
      health--;

    oldhealth = health;


    /* Temporarily keep track of old state for comparison or rollback: */

    oldplyx = plyx;
    oldplyy = plyy;

    oldkey_up = key_up;
    oldkey_down = key_down;
    oldkey_left = key_left;
    oldkey_right = key_right;

    /* Handle events: */

    while (SDL_PollEvent(&event) > 0)
    {
      if (event.type == SDL_KEYDOWN)
      {
        key = event.key.keysym.sym;
        if (key == SDLK_ESCAPE)
          done = 1;
        else if (key == SDLK_DOWN)
          key_down = 1;
        else if (key == SDLK_UP)
          key_up = 1;
        else if (key == SDLK_LEFT)
          key_left = 1;
        else if (key == SDLK_RIGHT)
          key_right = 1;
        else if (key == SDLK_SPACE)
          key_fire = 1;
        else if (key == SDLK_b)
        {
          if (bombs > 0 && health > 0)
          {
            score += bomb(map, plyx, plyy);
            bombs--;
          }
        }
      }
      else if (event.type == SDL_KEYUP)
      {
        key = event.key.keysym.sym;
        if (key == SDLK_DOWN)
          key_down = 0;
        else if (key == SDLK_UP)
          key_up = 0;
        else if (key == SDLK_LEFT)
          key_left = 0;
        else if (key == SDLK_RIGHT)
          key_right = 0;
        else if (key == SDLK_SPACE)
          key_fire = 0;
      }
      else if (event.type == SDL_QUIT) {
        done = 1;
      }
    }

    /* Based on keys, which direction is the player facing? */

    if (key_up)
      plydiry = 1;
    else if (key_down)
      plydiry = 3;
    else if (key_right || key_left)
      plydiry = 2;

    if (key_left)
      plydirx = 1;
    else if (key_right)
      plydirx = 3;
    else if (key_up || key_down)
      plydirx = 2;

    got = -1;

    if (health > 0)
    {
      if (key_fire)
      {
        /* If pressing [Fire], and a direction, try to shoot: */

        found = -1;
        if ((key_up && !oldkey_up) ||
            (key_down && !oldkey_down) ||
            (key_left && !oldkey_left) ||
            (key_right && !oldkey_right))
        {
          for (i = 0; i < MAXARROWS && found == -1; i++)
          {
            if (arrows[i].alive == 0)
              found = i;
          }
        }

        if (found != -1)
        {
          /* Add an arrow: */

          arrows[found].alive = 1;
          arrows[found].x = plyx;
          arrows[found].y = plyy;

          if (key_up)
            arrows[found].ym = -1;
          else if (key_down)
            arrows[found].ym = 1;
          else
            arrows[found].ym = 0;

          if (key_left)
            arrows[found].xm = -1;
          else if (key_right)
            arrows[found].xm = 1;
          else
            arrows[found].xm = 0;
        }
      }
      else
      {
        /* Not pressing [Fire]; move the player, if they're pressing any arrow key(s): */

        /* Note: We cannot move diagonally in a single frame, so we use 'toggle % 2'
           determine which direction to go if the user is pressing two arrow keys at once. */

        if (key_up)
        {
          if ((key_left == 0 && key_right == 0) || (toggle % 2) == 0)
          {
            got = safe(map, plyx, plyy - 1, toggle);
            if (got != -1) {
              make_noise(280.0, 200);
              plyy--;
            }
          }
        }
        else if (key_down)
        {
          if ((key_left == 0 && key_right == 0) || (toggle % 2) == 0)
          {
            got = safe(map, plyx, plyy + 1, toggle);
            if (got != -1) {
              make_noise(200.0, 200);
              plyy++;
            }
          }
        }

        if (key_left)
        {
          if ((key_up == 0 && key_down == 0) || (toggle % 2) == 1)
          {
            got = safe(map, plyx - 1, plyy, toggle);
            if (got != -1) {
              make_noise(255.0, 200);
              plyx--;
            }
          }
        }
        else if (key_right)
        {
          if ((key_up == 0 && key_down == 0) || (toggle % 2) == 1)
          {
            got = safe(map, plyx + 1, plyy, toggle);
            if (got != -1) {
              make_noise(268.0, 200);
              plyx++;
            }
          }
        }
      }
    }

    /* If they player bumped into something that we can act upon... */

    if (got == '$')
    {
      /* Money */

      map[plyy][plyx] = ' ';
      score += 10;
    }
    else if (got == 'b')
    {
      /* A bomb */

      map[plyy][plyx] = ' ';
      bombs++;
    }
    else if (got == 'k')
    {
      /* A key */

      map[plyy][plyx] = ' ';
      keys++;
    }
    else if (got == '=')
    {
      /* A door */

      if (keys > 0)
      {
        /* We have a key; remove the door: */

        keys--;
        opendoor(map, plyx, plyy);
      }
      else
      {
        /* No keys; user cannot go through the door! */

        plyx = oldplyx;
        plyy = oldplyy;
      }
    }
    else if (got == 'F')
    {
      /* Food */

      map[plyy][plyx] = ' ';
      health += (MAXHEALTH / 30);
      if (health > maxhealth)
        health = maxhealth;
    }
    else if (got == 'C' || got == 'B' || got == '3' || got == '2')
    {
      /* A bad guy or generator, higher than level 1:
         Reduce the bad guy or generator, bounce player back, reduce player's health */

      map[plyy][plyx]--;
      plyx = oldplyx;
      plyy = oldplyy;
      health -= 10;
      score += 10;
    }
    else if (got == 'A' || got == '1')
    {
      /* A bad guy or generator, at level 1:
         Remove the bad guy or generator, reduce player's health */

      map[plyy][plyx] = ' ';
      health -= 10;
      score += 10;
    }

    /* Are we still alive?! */

    if (health < 0)
      health = 0;

    /* Handle arrows: */
    /* FIXME: These can be handled as part of the map's cellular automata -bjk 2009.02.26 */

    for (i = 0; i < MAXARROWS; i++)
    {
      /* Since bad guys can move into the arrow's position as the arrow
         is about to go by, we check whether the arrow hit anything before and after
         moving it. */

      for (j = 0; j < 2; j++)
      {
        if (j == 1 && arrows[i].alive)
        {
          /* Arrow exists;
             If we've already checked for bad guys at the original position,
             move it (and see if it's still on the map) */

          arrows[i].x += arrows[i].xm;
          arrows[i].y += arrows[i].ym;
          if (arrows[i].y < 0 || arrows[i].y >= MAPH || arrows[i].x < 0 || arrows[i].x >= MAPW)
            arrows[i].alive = 0;
        }

        if (arrows[i].alive)
        {
          /* Arrow still exists; see what it hit into */

          c = map[arrows[i].y][arrows[i].x];
          if (c == 'B' || c == 'C' || c == '2' || c == '3')
          {
            /* A bad guy or generator, higher than level 1:
               Reduce the bad guy or generator */

            map[arrows[i].y][arrows[i].x]--;
            score += 10;
          }
          else if (c == 'A' || c == '1')
          {
            /* A bad guy or generator, at level 1:
               Remove the bad guy or generator */

            map[arrows[i].y][arrows[i].x] = ' ';
            score += 10;
          }
          else if (c == 'b')
          {
            /* A bomb; activate it now! */

            map[arrows[i].y][arrows[i].x] = ' ';
            score += (bomb(map, plyx, plyy) / 2);
          } 
          else if (c == 'F')
          {
            /* Food; remove it (don't shoot food!!!) */

            map[arrows[i].y][arrows[i].x] = ' ';
          } 


          /* Arrows go away once they hit _anything_ */

          if (c != ' ')
            arrows[i].alive = 0;
        }
      }
    }


    /* Handle objects on the map, as a cellular automata: */

    for (y = 0; y < MAPH; y++)
    {
      for (x = 0; x < MAPW; x++)
      {
        /* For those objects interested, which way do we go to
           head towards the player */

        if (plyy > y)
          y2 = y + 1;
        else if (plyy < y)
          y2 = y - 1;
        else
          y2 = y;

        if (plyx > x)
          x2 = x + 1;
        else if (plyx < x)
          x2 = x - 1;
        else
          x2 = x;

        /* Note: We cannot move diagonally in a single frame,
           so use (toggle % 2) to determine which way to go
           (up/down, or left/right), if we're headed diagonally */

        if (y2 != y && x2 != x)
        {
          if (toggle % 2 == 0)
            x2 = x;
          else
            y2 = y;
        }

        /* What's at our current position (and the one we want to
           go to, if we're a bad-guy) */

        c = map[y][x];
        c2 = map[y2][x2];

        if (c == 'A' || c == 'B' || c == 'C')
        {
          /* Bad guys (who haven't moved already) */

          if (x2 == plyx && y2 == plyy)
          {
            /* If we're adjacent to the player, try to do a melee attack */

            if (key_fire == 0)
            {
              /* If player is not trying to attack us,
                 try to attack them.
                 (Our chance of succeeding in the melee attack depends
                 on what level we are) */

              if (c == 'A' && (rand() % 10) < 3)
                health -= 10;
               else if (c == 'B' && (rand() % 10) < 5)
                health -= 10;
               else if (c == 'C' && (rand() % 10) < 7)
                health -= 10;
             }
          }
          else
          {
            /* Head toward's the player */
            /* Note: We place a modified bad guy onto the map, so that
               they can't move right or down more than once per frame,
               as we traverse the map.  They get replaced with normal
               bad guys after the entire map has been processed */

            if (c2 == ' ' && (rand() % 10) < 3)
            {
              map[y][x] = ' ';
              map[y2][x2] = c - 'A' + 'X';
            }
          }
        }
        else if (c == '1' || c == '2' || c == '3')
        {
          /* Generators */

          /* Pick a random adjacent spot */
          x2 = x + (rand() % 3) - 1;
          y2 = y + (rand() % 3) - 1;

          /* If the spot is empty, generate a bad guy of
             our level into the spot */

          if (map[y2][x2] == ' ')
            map[y2][x2] = (c - '1' + 'A');
        }
      }
    }

    /* Convert modified ('has already moved this frame') bad guys
       back to normal */
    for (y = 0; y < MAPH; y++)
      for (x = 0; x < MAPW; x++)
        if (map[y][x] == 'X' || map[y][x] == 'Y' || map[y][x] == 'Z')
          map[y][x] = map[y][x] - 'X' + 'A';


    /* Clear the screen */
    if (health > 0)
      SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
    else
      SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 32, 0, 0));


    /* Draw the objects we can see from here */

    for (y = plyy - (SCREENH / TILEH) / 2; y <= plyy + (SCREENH / TILEH) / 2; y++)
    {
      for (x = plyx - (SCREENW / TILEW) / 2; x <= plyx + (SCREENW / TILEW) / 2; x++)
      {
        if (y >= 0 && y < MAPH && x >= 0 && x < MAPW)
        {
          /* Draw map objects: */

          c = map[y][x];
          dest.x = (x - plyx + (SCREENW / TILEW) / 2) * TILEW;
          dest.y = (y - plyy + (SCREENH / TILEH) / 2) * TILEH;
          dest.w = TILEW;
          dest.h = TILEH;

          if (c == '#')
            drawimage(screen, img_blockers, 2, 1, 1, 1, dest.x, dest.y);
          else if (c == '$')
            drawimage(screen, img_collectibles, 4, 1, 4, 1, dest.x, dest.y);
          else if (c == '=')
            drawimage(screen, img_blockers, 2, 1, 2, 1, dest.x, dest.y);
          else if (c == 'F')
            drawimage(screen, img_collectibles, 4, 1, 1, 1, dest.x, dest.y);
          else if (c == '1')
            drawimage(screen, img_generators, 3, 1, 1, 1, dest.x, dest.y);
          else if (c == '2')
            drawimage(screen, img_generators, 3, 1, 2, 1, dest.x, dest.y);
          else if (c == '3')
            drawimage(screen, img_generators, 3, 1, 3, 1, dest.x, dest.y);
          else if (c == 'A' || c == 'B' || c == 'C')
            drawimage(screen, img_badguys, 6, 9,
              (plyx < x ? 1 : (plyx == x ? 2 : 3)) + (3 * (rand() % 2)),
              (plyy < y ? 1 : (plyy == y ? 2 : 3)) + (3 * (c - 'A')),
              dest.x, dest.y);
          else if (c == 'k')
            drawimage(screen, img_collectibles, 4, 1, 2, 1, dest.x, dest.y);
          else if (c == 'b')
            drawimage(screen, img_collectibles, 4, 1, 3, 1, dest.x, dest.y);

          /* Draw the player */

          if (x == plyx && y == plyy && health > 0)
          {
            /* Flash around player if they just got hurt */

            if (health < oldhealth)
              SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));

            drawimage(screen, img_player, 6, 3,
              plydirx + (((key_up || key_down || key_left || key_right) && !key_fire) ? (3 * (toggle % 2)) : 0),
              plydiry,
              dest.x, dest.y);
          }

          /* Draw any arrows we can still see: */

          for (i = 0; i < MAXARROWS; i++)
          {
            if (arrows[i].alive && x == arrows[i].x && y == arrows[i].y)
            {
              drawimage(screen, img_arrows, 3, 3, 2 + arrows[i].xm, 2 + arrows[i].ym, dest.x, dest.y);
            }
          }
        }
      }
    }

    /* Draw our health meter */

    if (health > 0)
    {
      dest.x = 0;
      dest.y = TILEH / 4;
      dest.w = (health * screen->w) / MAXHEALTH;
      dest.h = TILEH / 2;

      /* Flash if it's low! */

      if (health > MAXHEALTH / 10 || (toggle % 2) == 0)
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 255));
      else
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 0, 0));
    }


    /* Draw our collected objects */

    /* Keys */

    for (i = 0; i < keys; i++)
    {
      dest.x = (i * TILEW);
      dest.y = TILEH;
      drawimage(screen, img_collectibles, 4, 1, 2, 1, dest.x, dest.y);
    }

    /* Bombs */

    for (i = 0; i < bombs; i++)
    {
      dest.x = (i * TILEW);
      dest.y = TILEH * 2;
      drawimage(screen, img_collectibles, 4, 1, 3, 1, dest.x, dest.y);
    }

    /* Update the screen, and keep framerate throttled: */

    SDL_Flip(screen);

    nowtimestamp = SDL_GetTicks();
    if (nowtimestamp - timestamp < (1000 / FPS))
      SDL_Delay(timestamp + (1000 / FPS) - nowtimestamp);
  }
  while (!done);

  SDL_FreeSurface(img_generators);
  SDL_FreeSurface(img_collectibles);
  SDL_FreeSurface(img_blockers);
  SDL_FreeSurface(img_arrows);
  SDL_FreeSurface(img_badguys);
  SDL_FreeSurface(img_player);

  SDL_Quit();

  return(0);
}


/* Determine whether player can move into a particular spot */

int safe(char map[MAPH][MAPW], int x, int y, int toggle)
{
  char c;

  if (x < 0 || y < 0 || x >= MAPW || y >= MAPH)
    return -1;

  c = map[y][x];

  if (c == '#')
  {
    /* Cannot walk through walls: */
    return -1;
  }
  else if (c == 'A' || c == 'B' || c == 'C' ||
           c == '1' || c == '2' || c == '3')  
  {
    /* Can only melee attack bad guys occassionally */
    if ((toggle % 4) >= 2)
      return -1;
  }

  /* Otherwise, go ahead and try moving there */
  /* Note: We may be bounced back (e.g., if you try to walk
     through a door without any keys, or you're melee-attacking
     a bad guy, you'll get bounced back, even though we report
     it as being 'safe' here). */

  return (int) c;
}


/* Explode a bomb - hurt all bad things visible on the screen */

int bomb(char map[MAPH][MAPW], int plyx, int plyy)
{
  int x, y;
  char c;
  int score;

  score = 0;

  for (y = plyy - (SCREENH / TILEH) / 2; y <= plyy + (SCREENH / TILEH) / 2; y++)
  {
    for (x = plyx - (SCREENW / TILEW) / 2; x <= plyx + (SCREENW / TILEW) / 2; x++)
    {
      if (y >= 0 && y < MAPH && x >= 0 && x < MAPW)
      {
        c = map[y][x];
        if (c == 'B' || c == 'C' || c == '2' || c == '3')
        {
          /* A bad guy or generator, higher than level 1:
             Reduce the bad guy or generator */

          map[y][x]--;
          score += 10;
        }
        else if (c == 'A' || c == '1')
        {
          /* A bad guy or generator, at level 1:
             Remove the bad guy or generator */

          map[y][x] = ' ';
          score += 10;
        }
      }
    }
  }

  return(score);
}


/* 'Open' a door, by removing all door pieces that touch.
   Note: a recursive function, but uses two x/y coordinates
   and does up/down/left/right adjacency tests to crawl
   along the door and remove it.  This means no more than
   two door pieces should touch on the map, or the door
   won't open right! */

void opendoor(char map[MAPH][MAPW], int x, int y)
{
  int x1, y1, x2, y2;
  char c1l, c1r, c1u, c1d;
  char c2l, c2r, c2u, c2d;
  int done;

  /* Start at the spot on the door that the player touched */

  x1 = x2 = x;
  y1 = y2 = y;

  do
  {
    done = 1;

    /* Look around x/y coordinate #1 */

    c1l = map[y1][x1 - 1];
    c1r = map[y1][x1 + 1];
    c1u = map[y1 - 1][x1];
    c1d = map[y1 + 1][x1];


    /* Remove door at x/y coordinates #1 and #2 */

    map[y1][x1] = ' ';
    map[y2][x2] = ' ';

    /* Crawl x/y coordinate #1 towards any door piece
       that's adjacent */

    if (c1l == '=')
    {
      done = 0;
      x1--;
    }
    else if (c1r == '=')
    {
      done = 0;
      x1++;
    }
    else if (c1u == '=')
    {
      done = 0;
      y1--;
    }
    else if (c1d == '=')
    {
      done = 0;
      y1++;
    }

    /* If we found a piece, remove it
       (so that x/y coordinate #2 doesn't see it) */

    if (!done)
      map[y1][x1] = ' ';


    /* Look around x/y coordinate #1 */

    c2l = map[y2][x2 - 1];
    c2r = map[y2][x2 + 1];
    c2u = map[y2 - 1][x2];
    c2d = map[y2 + 1][x2];

    /* Crawl x/y coordinate #1 towards any door piece
       that's adjacent */

    if (c2l == '=')
    {
      done = 0;
      x2--;
    }
    else if (c2r == '=')
    {
      done = 0;
      x2++;
    }
    else if (c2u == '=')
    {
      done = 0;
      y2--;
    }
    else if (c2d == '=')
    {
      done = 0;
      y2++;
    }

    /* Note: The piece at x/y coordinate #2 will be removed
       when this loop repeats. */
  }
  while (!done);

  /* Note: Loop stops repeating once all adjacent door pieces have
     been found. */
}

SDL_Surface * loadimage(char * fname)
{
  char fullfname[1024];
  SDL_Surface * tmp1, * tmp2;

  snprintf(fullfname, sizeof(fullfname), "data/images/%s", fname);
  tmp1 = IMG_Load(fullfname);
  if (tmp1 == NULL)
  {
    fprintf(stderr, "Couldn't load %s: %s\n", fullfname, SDL_GetError());
    SDL_Quit();
    exit(1);
  }

  tmp2 = SDL_DisplayFormatAlpha(tmp1);
  SDL_FreeSurface(tmp1);
  if (tmp2 == NULL)
  {
    fprintf(stderr, "Couldn't convert %s: %s\n", fullfname, SDL_GetError());
    SDL_Quit();
    exit(1);
  }
  
  return(tmp2);
}

void drawimage(SDL_Surface * screen, SDL_Surface * sheet, int tilew, int tileh, int tilex, int tiley, int x, int y)
{
  SDL_Rect src, dest;

  src.x = (sheet->w / tilew) * (tilex - 1);
  src.y = (sheet->h / tileh) * (tiley - 1);
  src.w = (sheet->w / tilew);
  src.h = (sheet->h / tileh);

  dest.x = x;
  dest.y = y;

  SDL_BlitSurface(sheet, &src, screen, &dest);
}

#if 0
static void audio_callback(void *data, Uint8 *buf, int len) {
  static int counter = 0;
  int i, ex, ex2;
  if (freq == 0) {
    for (i=0; i<len; i++)
      buf[i] = audio.silence;
    return;
  }

  /* calculate wave length */
  ex = (int) (1.0 / freq * 22050);
  ex2 = ex * 2;
  for (i=0; i<len; i++) {
    if (counter >= ex2) counter = 0;
    if (counter > ex)
      buf[i] = 140;
    else
      buf[i] = 0;
    counter++;
  }
}
#endif

