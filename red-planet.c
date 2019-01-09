#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

#include "SDL.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "font8x8_basic.h"

typedef unsigned char byte;

// entity flags
#define DELETED 0x1
#define PLAYER 0x2
#define ENEMY 0x4
#define BULLET 0x8
#define COLLECTABLE 0x10
#define WEAPON 0x20

typedef struct {
  byte flags;
  byte health;
  int x;
  int y;
  int dx;
  int dy;
} Entity;

typedef struct {
  int x;
  int y;
  int w;
  int h;
} Viewport;

typedef struct {
  SDL_Texture* tex;
  int x;
  int y;
  int w;
  int h;
} Image;

void play_level(SDL_Window* window, SDL_Renderer* renderer);
void load(Entity players[], Entity enemies[], Entity bullets[], Entity collectables[], Entity weapons[]);
void on_keydown(SDL_Event* evt, bool* is_gameover, bool* is_paused, SDL_Window* window);
void update(double dt, unsigned int last_loop_time, unsigned int curr_time, Entity players[], Entity enemies[], Entity bullets[], Entity collectables[], Entity weapons[]);
void render(SDL_Renderer* renderer, SDL_Texture* sprites, Entity players[], Entity enemies[], Entity bullets[], Entity collectables[], Entity weapons[], unsigned int start_time);

// utility functions
void toggle_fullscreen(SDL_Window *win);
double calc_dist(int x1, int y1, int x2, int y2);
int clamp(int val, int min, int max);
int render_text(SDL_Renderer* renderer, char str[], int offset_x, int offset_y, int size);
Image load_img(SDL_Renderer* renderer, char* path);
void render_img(SDL_Renderer* renderer, Image* img);
void center_img(Image* img, Viewport* viewport);
void render_sprite(SDL_Renderer* renderer, SDL_Texture* sprites, int src_x, int src_y, int dest_x, int dest_y);
void error(char* activity);

// game globals
Viewport vp = {};

int bullet_w = 4;
int bullet_h = 4;
double bullet_speed = 1500.0; // in px/sec

// px dimensions of sprites
// TODO: read in from JSON
int sprite_w = 32;
int sprite_h = 32;

int max_players = 4;
int max_enemies = 100;
int max_bullets = 100;
int max_collectables = 20;
int max_weapons = 20;

int level = 1;

int header_height = 20;

const int num_snd_effects = 6;
Mix_Chunk *snd_effects[num_snd_effects];

int game_width = 1024;
int game_height = 768;

// top level (title screen)
int main(int num_args, char* args[]) {
  time_t seed = time(NULL); // 1529597895;
  srand(seed);
  printf("Seed: %lld\n", seed);
  
  // SDL setup
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    error("initializing SDL");

  SDL_Window* window;
  window = SDL_CreateWindow("Red Planet Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, game_width, game_height, SDL_WINDOW_RESIZABLE);
  if (!window)
    error("creating window");
  
  // toggle_fullscreen(window);
  SDL_GetWindowSize(window, &vp.w, &vp.h);
  vp.h -= header_height;

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer)
    error("creating renderer");

  if (SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND) < 0)
    error("setting blend mode");

  Image title_img = load_img(renderer, "example/title.png");
  title_img.y = 50;
  
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    error("opening audio device");

  // snd_effects[0] = Mix_LoadWAV("audio/tower_shoot.wav");
  // snd_effects[1] = Mix_LoadWAV("audio/tower_damage.wav");
  // snd_effects[2] = Mix_LoadWAV("audio/beast_damage.wav");
  // snd_effects[3] = Mix_LoadWAV("audio/entity_enabled.wav");
  // snd_effects[4] = Mix_LoadWAV("audio/tower_explosion.wav");
  // snd_effects[5] = Mix_LoadWAV("audio/levelup.wav");

  // for (int i = 0; i < num_snd_effects; ++i)
  //   if (!snd_effects[i])
  //     error("loading wav");

  center_img(&title_img, &vp);

  SDL_Event evt;
  bool exit_game = false;
  while (!exit_game) {
    while (SDL_PollEvent(&evt)) {
      if (evt.type == SDL_QUIT || (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE))
        exit_game = true;
      else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_RETURN)
        play_level(window, renderer);
    }

    // set BG color
    if (SDL_SetRenderDrawColor(renderer, 77, 49, 49, 255) < 0)
      error("setting bg color");
    if (SDL_RenderClear(renderer) < 0)
      error("clearing renderer");
        
    render_img(renderer, &title_img);
    
    SDL_RenderPresent(renderer);
    SDL_Delay(10);
  }

  // for (int i = 0; i < num_snd_effects; ++i)
  //   Mix_FreeChunk(snd_effects[i]);
  Mix_Quit();

  // if (SDL_SetWindowFullscreen(window, 0) < 0)
  //   error("exiting fullscreen");

  SDL_DestroyTexture(title_img.tex);
  
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}

void play_level(SDL_Window* window, SDL_Renderer* renderer) {
  time_t seed = 1529597895; // 005; // time(NULL);
  srand(seed);
  printf("Seed: %lld\n", seed);

  // reset global variables
  unsigned int start_time = SDL_GetTicks();
  unsigned int pause_start = 0;
  
  // load game
  Entity players[max_players];
  Entity enemies[max_enemies];
  Entity bullets[max_bullets];
  Entity collectables[max_collectables];
  Entity weapons[max_weapons];

  load(players, enemies, bullets, collectables, weapons);

  SDL_Texture* sprites = IMG_LoadTexture(renderer, "example/spritesheet.png");
  if (!sprites)
    error("loading image");

  // game loop (incl. events, update & draw)
  bool is_gameover = false;
  bool is_paused = false;
  unsigned int last_loop_time = SDL_GetTicks();
  while (!is_gameover) {
    SDL_Event evt;

    // handle pause state
    if (is_paused) {
      if (!pause_start)
        pause_start = SDL_GetTicks();
      while (SDL_PollEvent(&evt))
        if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_SPACE)
          is_paused = false;

      if (is_paused) {
        SDL_Delay(10);
        continue;
      }
      else {
        // reset last_loop_time when coming out of a pause state
        // otherwise the game will react as if a ton of time has gone by
        last_loop_time = SDL_GetTicks();

        // add elapsed pause time to start time, otherwise the pause time is added to the clock
        start_time += last_loop_time - pause_start;
        pause_start = 0;
      }
    }

    // manage delta time
    unsigned int curr_time = SDL_GetTicks();
    double dt = (curr_time - last_loop_time) / 1000.0; // dt should always be in seconds

    // handle events
    while (SDL_PollEvent(&evt)) {
      switch(evt.type) {
        case SDL_QUIT:
          is_gameover = true;
          break;
        case SDL_WINDOWEVENT:
          if (evt.window.event == SDL_WINDOWEVENT_RESIZED) {
            SDL_GetWindowSize(window, &vp.w, &vp.h);
            vp.h -= header_height;
          }
          break;
        case SDL_KEYDOWN:
          on_keydown(&evt, &is_gameover, &is_paused, window);
          break;
      }
    }

    update(dt, last_loop_time, curr_time, players, enemies, bullets, collectables, weapons);
    render(renderer, sprites, players, enemies, bullets, collectables, weapons, start_time);

    SDL_Delay(10);
    last_loop_time = curr_time;
  }

  SDL_DestroyTexture(sprites);
}

void load(Entity players[], Entity enemies[], Entity bullets[], Entity collectables[], Entity weapons[]) {
  // precreate all entities as deleted
  for (int i = 0; i < max_players; ++i)
    players[i].flags = PLAYER | DELETED;

  for (int i = 0; i < max_enemies; ++i)
    enemies[i].flags = ENEMY | DELETED;

  for (int i = 0; i < max_bullets; ++i)
    bullets[i].flags = BULLET | DELETED;

  for (int i = 0; i < max_collectables; ++i)
    collectables[i].flags = COLLECTABLE | DELETED;

  for (int i = 0; i < max_weapons; ++i)
    weapons[i].flags = WEAPON | DELETED;
}

void on_keydown(SDL_Event* evt, bool* is_gameover, bool* is_paused, SDL_Window* window) {
  switch (evt->key.keysym.sym) {
    case SDLK_ESCAPE:
      *is_gameover = true;
      break;
    case SDLK_f:
      toggle_fullscreen(window);
      break;
    case SDLK_SPACE:
      *is_paused = !*is_paused;
      break;
    // case SDLK_LEFT:
    //   scroll_to(vp.x - 20, vp.y);
    //   break;
    // case SDLK_RIGHT:
    //   scroll_to(vp.x + 20, vp.y);
    //   break;
    // case SDLK_UP:
    //   scroll_to(vp.x, vp.y - 20);
    //   break;
    // case SDLK_DOWN:
    //   scroll_to(vp.x, vp.y + 20);
    //   break;
  }
}

void update(double dt, unsigned int last_loop_time, unsigned int curr_time, Entity players[], Entity enemies[], Entity bullets[], Entity collectables[], Entity weapons[]) {
  // fortress firing
  // for (int i = 0; i < max_buildings; ++i) {
  //   Entity* turret = &buildings[i];
  //   if (turret->flags & DELETED)
  //     continue;
  //   if (!(turret->flags & TURRET))
  //     continue;
  //   if (!crossed_interval(last_loop_time, curr_time, turret, turret_fire_interval))
  //     continue;

  //   int pos = to_pos(turret->x, turret->y);
  //   Entity* beast = closest_entity(turret->x, turret->y, beasts, max_beasts);
  //   double beast_dist = -1;
  //   if (beast)
  //     beast_dist = calc_dist(beast->x, beast->y, turret->x, turret->y);

  //   Entity* nest = closest_entity(turret->x, turret->y, nests, max_nests);
  //   double nest_dist = -1;
  //   if (nest)
  //     nest_dist = calc_dist(nest->x, nest->y, turret->x, turret->y);
    
  //   Entity* enemy;
  //   double dist;
  //   if (beast && nest) {
  //     if (beast_dist < nest_dist) {
  //       enemy = beast;
  //       dist = beast_dist;
  //     }
  //     else {
  //       enemy = nest;
  //       dist = nest_dist;
  //     }
  //   }
  //   else if (beast) {
  //     enemy = beast;
  //     dist = beast_dist;
  //   }
  //   else if (nest) {
  //     enemy = nest;
  //     dist = nest_dist;
  //   }
  //   else {
  //     continue;
  //   }

  //   if (dist > fortress_attack_dist)
  //     continue;

  //   Mix_PlayChannel(-1, snd_effects[0], 0);

  //   // dividing by the distance gives us a normalized 1-unit vector
  //   double dx = (enemy->x - turret->x) / dist;
  //   double dy = (enemy->y - turret->y) / dist;
  //   for (int j = 0; j < max_bullets; ++j) {
  //     Bullet* b = &bullets[j];
  //     if (b->flags & DELETED) {
  //       b->flags &= (~DELETED); // clear the DELETED bit
        
  //       // start in top/left corner
  //       int start_x = turret->x * block_w;
  //       int start_y = turret->y * block_h;
  //       if (dx > 0)
  //         start_x += block_w;
  //       else if (dx == 0)
  //         start_x += block_w / 2;
  //       else
  //         start_x -= 1; // so it's not on top of itself

  //       if (dy > 0)
  //         start_y += block_h;
  //       else if (dy == 0)
  //         start_y += block_h / 2;
  //       else
  //         start_y -= 1; // so it's not on top of itself

  //       b->x = start_x;
  //       b->y = start_y;
  //       b->dx = dx;
  //       b->dy = dy;
  //       break;
  //     }
  //   }
  // }

  // update bullet positions; handle bullet collisions
  for (int i = 0; i < max_bullets; ++i) {
    if (bullets[i].flags & DELETED)
      continue;

    bullets[i].x += bullets[i].dx * bullet_speed * dt;
    bullets[i].y += bullets[i].dy * bullet_speed * dt;
    // delete bullets that have gone out of the game
    if ((bullets[i].x < 0 || bullets[i].x > game_width) ||
      bullets[i].y < 0 || bullets[i].y > game_height) {
        bullets[i].flags |= DELETED; // set deleted bit on
        continue;
    }

    // int grid_x = bullets[i].x / block_w;
    // int grid_y = bullets[i].y / block_h;
    // if (is_in_grid(grid_x, grid_y)) {
    //   int pos = to_pos(grid_x, grid_y);
    //   Entity* ent = grid[pos];
    //   if (ent && ent->flags & TURRET) {
    //     bullets[i].flags |= DELETED;
    //     continue;
    //   }
    //   else if (ent && ent->flags & ENEMY) {
    //     inflict_damage(ent, grid);
    //     bullets[i].flags |= DELETED;
    //     if (ent->flags & DELETED && !countLiveEntities(nests, max_nests)) {
    //       Mix_PlayChannel(-1, snd_effects[5], 0);
    //       level++;
    //       if (level <= 4)
    //         create_nests(level, start_pos, nests, grid, grid_flags);
    //     }
    //     else {
    //       Mix_PlayChannel(-1, snd_effects[2], 0);
    //     }
    //   }
    // }
  }
}

void render(SDL_Renderer* renderer, SDL_Texture* sprites, Entity players[], Entity enemies[], Entity bullets[], Entity collectables[], Entity weapons[], unsigned int start_time) {
  // set BG color
  if (SDL_SetRenderDrawColor(renderer, 77, 49, 49, 255) < 0)
    error("setting bg color");
  if (SDL_RenderClear(renderer) < 0)
    error("clearing renderer");

  // render players
  for (int i = 0; i < max_players; ++i) {
    if (players[i].flags & DELETED)
      continue;

    render_sprite(renderer, sprites, 1,3, players[i].x, players[i].y);
  }

  // render enemies
  for (int i = 0; i < max_enemies; ++i) {
    if (enemies[i].flags & DELETED)
      continue;

    render_sprite(renderer, sprites, 1,3, enemies[i].x, enemies[i].y);
  }

  // render bullets
  for (int i = 0; i < max_bullets; ++i) {
    if (bullets[i].flags & DELETED)
      continue;

    render_sprite(renderer, sprites, 1,3, bullets[i].x, bullets[i].y);
  }

  // render collectables
  for (int i = 0; i < max_collectables; ++i) {
    if (collectables[i].flags & DELETED)
      continue;

    render_sprite(renderer, sprites, 1,3, collectables[i].x, collectables[i].y);
  }

  // render weapons
  for (int i = 0; i < max_players; ++i) {
    if (weapons[i].flags & DELETED)
      continue;

    render_sprite(renderer, sprites, 1,3, weapons[i].x, weapons[i].y);
  }

  // header
  int text_px_size = 2;
  if (SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) < 0)
    error("setting header color");
  SDL_Rect header_rect = {
    .x = 0,
    .y = 0,
    .w = vp.w,
    .h = header_height
  };
  if (SDL_RenderFillRect(renderer, &header_rect) < 0)
    error("filling header rect");

  if (SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255) < 0)
    error("setting text color");

  char level_str[10];
  snprintf(level_str, sizeof(level_str), "Level: %d", level);
  render_text(renderer, level_str, 5, 10, 1);

  char player_str[10];
  snprintf(player_str, sizeof(player_str), "Player 1: %d", 4);
  render_text(renderer, player_str, 35, 20, 1);

  unsigned int elapsed = SDL_GetTicks() - start_time;
  int sec = (int)(elapsed / 1000.0);
  int min = sec / 60;
  sec -= min * 60;
  char time_str[9]; // 9 digits goes up to -99999:59
  snprintf(time_str, sizeof(time_str), "%d:%02d", min, sec);
  render_text(renderer, time_str, vp.w - 80, 5, 2);

  SDL_RenderPresent(renderer);
}


// Game-Specific Functions
Entity* closest_entity(int x, int y, Entity entities[], int num_entities) {
  Entity* winner = NULL;
  double winner_dist = -1;
  
  for (int i = 0; i < num_entities; ++i) {
    if (entities[i].flags & DELETED)
      continue;

    double dist = calc_dist(x, y, entities[i].x, entities[i].y);
    if (winner_dist == -1 || dist < winner_dist) {
      winner = &entities[i];
      winner_dist = dist;
    }
  }
  return winner;
}

void inflict_damage(Entity* ent, Entity* grid[]) {
  ent->health--;
  if (ent->health <= 0)
    ent->flags |= DELETED; // flip DELETED bit on
}


// Generic Functions

void toggle_fullscreen(SDL_Window *win) {
  Uint32 flags = SDL_GetWindowFlags(win);
  if ((flags & SDL_WINDOW_FULLSCREEN_DESKTOP) || (flags & SDL_WINDOW_FULLSCREEN))
    flags = 0;
  else
    flags = SDL_WINDOW_FULLSCREEN;

  if (SDL_SetWindowFullscreen(win, flags) < 0)
    error("Toggling fullscreen mode failed");
}

double calc_dist(int x1, int y1, int x2, int y2) {
  return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

int clamp(int val, int min, int max) {
  if (val < min)
    return min;
  else if (val > max)
    return max;
  else
    return val;
}

int render_text(SDL_Renderer* renderer, char str[], int offset_x, int offset_y, int size) {
  int i;
  for (i = 0; str[i] != '\0'; ++i) {
    int code = str[i];
    if (code < 0 || code > 127)
      error("Text code out of range");

    char* bitmap = font8x8_basic[code];
    int set = 0;
    for (int y = 0; y < 8; ++y) {
      for (int x = 0; x < 8; ++x) {
        set = bitmap[y] & 1 << x;
        if (!set)
          continue;

        SDL_Rect r = {
          .x = offset_x + i * (size) * 8 + x * size,
          .y = offset_y + y * size,
          .w = size,
          .h = size
        };
        if (SDL_RenderFillRect(renderer, &r) < 0)
          error("drawing text block");
      }
    }
  }

  // width of total text string
  return i * size * 8;
}

// it may be more efficient to load the image into an sdl image
// then get the dimensions, then load it into a texture
// instead of loading it directly to a texture & then querying the texture?
Image load_img(SDL_Renderer* renderer, char* path) {
  Image img = {};
  img.tex = IMG_LoadTexture(renderer, path);
  if (!img.tex)
    error("loading image");

  SDL_QueryTexture(img.tex, NULL, NULL, &img.w, &img.h);
  return img;
}

void render_img(SDL_Renderer* renderer, Image* img) {
  SDL_Rect r = {.x = img->x, .y = img->y, .w = img->w, .h = img->h};
  if (SDL_RenderCopy(renderer, img->tex, NULL, &r) < 0)
    error("renderCopy");
}

// centers the image horizontally in the viewport
void center_img(Image* img, Viewport* viewport) {
  img->x = viewport->w / 2 - img->w / 2;
}

void render_sprite(SDL_Renderer* renderer, SDL_Texture* sprites, int src_x, int src_y, int dest_x, int dest_y) {
  SDL_Rect src = {.x = src_x * sprite_w / 2, .y = src_y * sprite_h / 2, .w = sprite_w / 2, .h = sprite_h / 2};
  SDL_Rect dest = {.x = dest_x * sprite_w - vp.x, .y = dest_y * sprite_h - vp.y, .w = sprite_w, .h = sprite_h};
  if (SDL_RenderCopy(renderer, sprites, &src, &dest) < 0)
    error("renderCopy");
}

// TODO: consolidate w/ above is_mouseover()
bool contains(SDL_Rect* r, int x, int y) {
  return x >= r->x && x <= (r->x + r->w) &&
    y >= r->y && y <= (r->y + r->h);
}

void error(char* activity) {
  printf("%s failed: %s\n", activity, SDL_GetError());
  SDL_Quit();
  exit(-1);
}
