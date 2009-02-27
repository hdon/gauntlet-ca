#include <stdio.h>
#include <stdlib.h>
#include "SDL.h"

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

int main(int argc, char * argv[])
{
  SDL_Surface * screen;
  int done;
  int i, x, y, x2, y2, found;
  int plyx, plyy, score, oldplyx, oldplyy, health, maxhealth, bombs, keys;
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
  arrow_t arrows[MAXARROWS];

  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    fprintf(stderr, "Error init'ing SDL: %s\n", SDL_GetError());
    exit(1);
  }

  screen = SDL_SetVideoMode(SCREENW, SCREENH, 0, 0);
  if (screen == NULL)
  {
    fprintf(stderr, "Error opening video: %s\n", SDL_GetError());
    exit(1);
  }

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

  for (i = 0; i < MAXARROWS; i++)
    arrows[i].alive = 0;

  do
  {
    timestamp = SDL_GetTicks();
    toggle++;

    if (toggle % 2 == 0)
      health--;

    oldplyx = plyx;
    oldplyy = plyy;

    oldkey_up = key_up;
    oldkey_down = key_down;
    oldkey_left = key_left;
    oldkey_right = key_right;

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
          if (bombs > 0)
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
    }

    got = -1;

    if (health > 0)
    {
      if (key_fire)
      {
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
        if (key_up)
        {
          if ((key_left == 0 && key_right == 0) || (toggle % 2) == 0)
          {
            got = safe(map, plyx, plyy - 1, toggle);
            if (got != -1)
              plyy--;
          }
        }
        else if (key_down)
        {
          if ((key_left == 0 && key_right == 0) || (toggle % 2) == 0)
          {
            got = safe(map, plyx, plyy + 1, toggle);
            if (got != -1)
              plyy++;
          }
        }

        if (key_left)
        {
          if ((key_up == 0 && key_down == 0) || (toggle % 2) == 1)
          {
            got = safe(map, plyx - 1, plyy, toggle);
            if (got != -1)
              plyx--;
          }
        }
        else if (key_right)
        {
          if ((key_up == 0 && key_down == 0) || (toggle % 2) == 1)
          {
            got = safe(map, plyx + 1, plyy, toggle);
            if (got != -1)
              plyx++;
          }
        }
      }
    }

    if (got == '$')
    {
      map[plyy][plyx] = ' ';
      score += 10;
      printf("money (score = %d)\n", score);
    }
    else if (got == 'b')
    {
      map[plyy][plyx] = ' ';
      bombs++;
      printf("bomb (%d)\n", bombs);
    }
    else if (got == 'k')
    {
      map[plyy][plyx] = ' ';
      keys++;
      printf("key (%d)\n", keys);
    }
    else if (got == '=')
    {
      if (keys > 0)
      {
        keys--;
        printf("door - opening (keys = %d)\n", keys);
        opendoor(map, plyx, plyy);
      }
      else
      {
        printf("door - locked (no keys!)\n");
        plyx = oldplyx;
        plyy = oldplyy;
      }
    }
    else if (got == 'F')
    {
      map[plyy][plyx] = ' ';
      health += (MAXHEALTH / 30);
      if (health > maxhealth)
        health = maxhealth;
      printf("health (%d)\n", health);
    }
    else if (got == 'C' || got == '3')
    {
      map[plyy][plyx]--;
      plyx = oldplyx;
      plyy = oldplyy;
      health -= 10;
      score += 10;
    }
    else if (got == 'B' || got == '2')
    {
      map[plyy][plyx]--;
      plyx = oldplyx;
      plyy = oldplyy;
      health -= 10;
      score += 10;
    }
    else if (got == 'A' || got == '1')
    {
      map[plyy][plyx] = ' ';
      health -= 10;
      score += 10;
    }

    if (health < 0)
      health = 0;

    for (i = 0; i < MAXARROWS; i++)
    {
      if (arrows[i].alive)
      {
        arrows[i].x += arrows[i].xm;
        arrows[i].y += arrows[i].ym;
        if (arrows[i].y < 0 || arrows[i].y >= MAPH || arrows[i].x < 0 || arrows[i].x >= MAPW)
          arrows[i].alive = 0;
      }

      if (arrows[i].alive)
      {
        c = map[arrows[i].y][arrows[i].x];
        if (c == 'B' || c == 'C' || c == '2' || c == '3')
        {
          map[arrows[i].y][arrows[i].x]--;
          score += 10;
        }
        else if (c == 'A' || c == '1')
        {
          map[arrows[i].y][arrows[i].x] = ' ';
          score += 10;
        }
        else if (c == 'b')
        {
          map[arrows[i].y][arrows[i].x] = ' ';
          score += bomb(map, plyx, plyy);
        } 

        if (c != ' ')
          arrows[i].alive = 0;
      }
    }

    for (y = 0; y < MAPH; y++)
    {
      for (x = 0; x < MAPW; x++)
      {
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

        if (y2 != y && x2 != x)
        {
          if (toggle % 2 == 0)
            x2 = x;
          else
            y2 = y;
        }

        c = map[y][x];
        c2 = map[y2][x2];

        if (c == 'A' || c == 'B' || c == 'C')
        {
          if (x2 == plyx && y2 == plyy)
          {
            if (c == 'A' && (rand() % 10) < 3)
              health -= 10;
            else if (c == 'B' && (rand() % 10) < 5)
              health -= 10;
            else if (c == 'C' && (rand() % 10) < 7)
              health -= 10;
          }
          else
          {
            if (c2 == ' ' && (rand() % 10) < 3)
            {
              map[y][x] = ' ';
              map[y2][x2] = c;
            }
          }
        }
        else if (c == '1' || c == '2' || c == '3')
        {
          x2 = x + (rand() % 3) - 1;
          y2 = y + (rand() % 3) - 1;
          if (map[y2][x2] == ' ')
            map[y2][x2] = (c - '1' + 'A');
        }
      }
    }

    if (health > 0)
      SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
    else
      SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 32, 0, 0));

    for (y = plyy - (SCREENH / TILEH) / 2; y <= plyy + (SCREENH / TILEH) / 2; y++)
    {
      for (x = plyx - (SCREENW / TILEW) / 2; x <= plyx + (SCREENW / TILEW) / 2; x++)
      {
        if (y >= 0 && y < MAPH && x >= 0 && x < MAPW)
        {
          c = map[y][x];
          dest.x = (x - plyx + (SCREENW / TILEW) / 2) * TILEW;
          dest.y = (y - plyy + (SCREENH / TILEH) / 2) * TILEH;
          dest.w = TILEW;
          dest.h = TILEH;

          if (c == '#')
            SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 128, 128, 128));
          else if (c == '$')
            SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, ((toggle * 16) % 256), ((toggle * 16) % 256), 0));
          else if (c == '=')
            SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 32, 32, 32));
          else if (c == 'F')
            SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 192, 0));
          else if (c == '1')
            SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, ((toggle * 32) % 256), 0, 0));
          else if (c == '2')
            SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, ((toggle * 32) % 256), 0));
          else if (c == '3')
            SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, ((toggle * 32) % 256), 0, ((toggle * 32) % 256)));
          else if (c == 'A')
            SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 64, 0, 0));
          else if (c == 'B')
            SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 64, 0));
          else if (c == 'C')
            SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 64, 0, 64));
          else if (c == 'k')
            SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 255, 255));
          else if (c == 'b')
            SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 128, 255, 0));

          if (x == plyx && y == plyy && health > 0)
          {
            dest.x = (x - plyx + (SCREENW / TILEW) / 2) * TILEW + 4;
            dest.y = (y - plyy + (SCREENH / TILEH) / 2) * TILEH + 4;
            dest.w = TILEW - 8;
            dest.h = TILEH - 8;
            

            SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));
          }

          for (i = 0; i < MAXARROWS; i++)
          {
            if (arrows[i].alive && x == arrows[i].x && y == arrows[i].y)
            {
              dest.x = (x - plyx + (SCREENW / TILEW) / 2) * TILEW + 6;
              dest.y = (y - plyy + (SCREENH / TILEH) / 2) * TILEH + 6;
              dest.w = TILEW - 12;
              dest.h = TILEH - 12;

              SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 255));
            }
          }
        }
      }
    }

    if (health > 0)
    {
      dest.x = 0;
      dest.y = TILEH / 4;
      dest.w = (health * screen->w) / MAXHEALTH;
      dest.h = TILEH / 2;
      if (health > MAXHEALTH / 10 || (toggle % 2) == 0)
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 255));
      else
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 0, 0));
    }

    for (i = 0; i < keys; i++)
    {
      dest.x = (i * TILEW);
      dest.y = TILEH;
      dest.w = TILEW;
      dest.h = TILEH;
      SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 255, 255));
    }

    for (i = 0; i < bombs; i++)
    {
      dest.x = (i * TILEW);
      dest.y = TILEH * 2;
      dest.w = TILEW;
      dest.h = TILEH;
      SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 128, 255, 0));
    }

    SDL_Flip(screen);

    nowtimestamp = SDL_GetTicks();
    if (nowtimestamp - timestamp < (1000 / FPS))
      SDL_Delay(timestamp + (1000 / FPS) - nowtimestamp);
  }
  while (!done);

  SDL_Quit();

  return(0);
}

int safe(char map[MAPH][MAPW], int x, int y, int toggle)
{
  char c;

  if (x < 0 || y < 0 || x >= MAPW || y >= MAPH)
    return -1;

  c = map[y][x];

  if (c == ' ' || c == '$' || c == 'F' || c == '=' || c == 'b' || c == 'k')
    return (int) c;
  else if (c == 'A' || c == 'B' || c == 'C' ||
           c == '1' || c == '2' || c == '3')  
  {
    if ((toggle % 4) >= 2)
      return (int) c;
  }

  return -1;
}

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
          map[y][x]--;
          score += 10;
        }
        else if (c == 'A' || c == '1')
        {
          map[y][x] = ' ';
          score += 10;
        }
      }
    }
  }

  return(score);
}

void opendoor(char map[MAPH][MAPW], int x, int y)
{
  int x1, y1, x2, y2;
  char c1l, c1r, c1u, c1d;
  char c2l, c2r, c2u, c2d;
  int done;

  x1 = x2 = x;
  y1 = y2 = y;

  do
  {
    done = 1;

    c1l = map[y1][x1 - 1];
    c1r = map[y1][x1 + 1];
    c1u = map[y1 - 1][x1];
    c1d = map[y1 + 1][x1];

    map[y1][x1] = ' ';
    map[y2][x2] = ' ';

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

    if (!done)
      map[y1][x1] = ' ';

    c2l = map[y2][x2 - 1];
    c2r = map[y2][x2 + 1];
    c2u = map[y2 - 1][x2];
    c2d = map[y2 + 1][x2];

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
  }
  while (!done);
}

