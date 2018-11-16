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

// grid functions
bool in_bounds(int x, int y);
int find_avail_pos(Entity* grid[], byte grid_flags[]);
void move(Entity* ent, Entity* grid[], int x, int y);
void set_pos(Entity* ent, Entity* grid[], int pos);
void set_xy(Entity* ent, Entity* grid[], int x, int y);
void remove_from_grid(Entity* ent, Entity* grid[]);
int to_x(int ix);
int to_y(int ix);
int to_pos(int x, int y);
bool is_in_grid(int x, int y);

// game-specific functions
void play_level(SDL_Window* window, SDL_Renderer* renderer);
int load(Entity* grid[], byte grid_flags[], Entity blocks[], Entity beasts[], Entity buildings[], Entity nests[], Bullet bullets[]);
void gen_water(byte grid_flags[], short top_left, short top_right, short bottom_left, short bottom_right, int x, int y, int w);
void create_nests(int level, int start_pos, Entity nests[], Entity* grid[], byte grid_flags[]);
void remove_sm_islands(byte grid_flags[]);
void remove_sm_lakes(byte grid_flags[]);
void on_mousemove(SDL_Event* evt, Entity* grid[], byte grid_flags[], Entity buildings[]);
void on_mousedown(SDL_Event* evt, Entity* grid[], byte grid_flags[], Entity buildings[]);
void place_entity(int x, int y, Entity* grid[], byte grid_flags[], Entity buildings[]);
void update_explored(int pos, byte grid_flags[]);
void on_keydown(SDL_Event* evt, Entity* grid[], bool* is_gameover, bool* is_paused, SDL_Window* window);
void on_scroll(SDL_Event* evt);
void scroll_to(int x, int y);
void update(double dt, unsigned int last_loop_time, unsigned int curr_time, Entity* grid[], byte grid_flags[], Entity buildings[], Entity beasts[], Entity nests[], Bullet bullets[]);
bool crossed_interval(unsigned int last_loop_time, unsigned int curr_time, Entity* ent, int interval);
void render(SDL_Renderer* renderer, SDL_Texture* sprites, Entity* grid[], byte grid_flags[], Entity blocks[], Entity buildings[], Entity beasts[], Entity nests[], Bullet bullets[], unsigned int start_time);
int countLiveEntities(Entity arr[], int arr_len);

double toExponential(int num_coins);
bool is_ent_adj(Entity* ent1, Entity* ent2);
bool is_adj(Entity* grid[], byte grid_flags[], int x, int y);

bool is_road(byte grid_flags[], int x, int y);
bool is_wall(Entity* grid[], int x, int y);
bool is_building(Entity* grid[], int x, int y);

Entity* closest_entity(int x, int y, Entity entities[], int num_entities);
void del_entity(Entity* ent, Entity* grid[]);
int choose_adj_pos(Entity* beast, Entity* closest_turret, Entity* grid[], byte grid_flags[]);
void inflict_damage(Entity* ent, Entity* grid[]);
int calc_island_size(int pos, byte grid_flags[]);
void flood_fill_island(int pos, byte grid_flags[]);
void clear_flood_fill(byte grid_flags[]);

// generic functions
void toggle_fullscreen(SDL_Window *win);
double calc_dist(int x1, int y1, int x2, int y2);
int clamp(int val, int min, int max);
int render_text(SDL_Renderer* renderer, char str[], int offset_x, int offset_y, int size);
Image load_img(SDL_Renderer* renderer, char* path);
void render_img(SDL_Renderer* renderer, Image* img);
void center_img(Image* img, Viewport* viewport);
void render_sprite(SDL_Renderer* renderer, SDL_Texture* sprites, int src_x, int src_y, int dest_x, int dest_y);
void render_sprite_btn(SDL_Renderer* renderer, SDL_Texture* sprites, int src_x, int src_y, int dest_x_px, int dest_y_px);
void render_corner(SDL_Renderer* renderer, SDL_Texture* sprites, int src_x, int src_y, int dest_x, int dest_y);
bool is_mouseover(Image* img, int x, int y);
bool contains(SDL_Rect* r, int x, int y);
void error(char* activity);

// game globals
Viewport vp = {};

int num_collected_blocks;
int num_blocks_per_road = 1;
int num_blocks_per_wall = 2;
int num_blocks_per_turret = 30;
int num_blocks_per_mine = 40;
int num_blocks_per_bridge = 100;
int beast_attack_dist = 30; // how close a beast has to be before he moves towards you
int fortress_attack_dist = 6; // how close before a turret fires on a beast/nest
byte beast_health = 3;
byte fortress_health = 3;
byte mine_health = 1; // with a single blow, a beast can destroy a mine
byte nest_health = 100;
byte wall_health = 8;
int explored_dist = 6; // how many blocks to show next to "explored" areas

int block_w = 40;
int block_h = 40;
int bullet_w = 4;
int bullet_h = 4;
double bullet_speed = 1500.0; // in px/sec
int block_density_pct = 4;

int num_blocks_w = 128; // 2^7
int num_blocks_h = 128; // 2^7
int grid_len;

int beast_move_interval = 500; // ms between beast moves
int turret_fire_interval = 900; // ms between turret firing
int mine_interval = 2000; // ms between mine generating metal
int beast_spawn_interval = 10000;

int max_beasts = 500;
int max_buildings = 1000;
int max_bullets = 100;
int max_blocks;
int max_nests = 12;
int num_starting_nests = 3;

int level = 1;
int start_pos = -1;

int header_height = 75;

SDL_Rect road_btn = {.x = 0, .y = 5, .w = 50, .h = 50};
SDL_Rect wall_btn = {.x = 0, .y = 5, .w = 50, .h = 50};
SDL_Rect fortress_btn = {.x = 0, .y = 5, .w = 50, .h = 50};
SDL_Rect mine_btn = {.x = 0, .y = 5, .w = 50, .h = 50};
SDL_Rect bridge_btn = {.x = 0, .y = 5, .w = 50, .h = 50};

SDL_Rect* selected_btn = &fortress_btn;
SDL_Rect* hover_btn = NULL;
int ui_bar_w = 440;
int ui_bar_h = 70;
SDL_Cursor* arrow_cursor = NULL;
SDL_Cursor* hand_cursor = NULL;

const int num_snd_effects = 6;
Mix_Chunk *snd_effects[num_snd_effects];

// top level (title screen)
int main(int num_args, char* args[]) {
  time_t seed = 1529597895; //time(NULL);
  srand(seed);
  printf("Seed: %lld\n", seed);
  
  // SDL setup
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    error("initializing SDL");

  SDL_Window* window;
  window = SDL_CreateWindow("Future Fortress", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, num_blocks_w * block_w, num_blocks_h * block_h, SDL_WINDOW_RESIZABLE);
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

  arrow_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
  hand_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);

  Image title_img = load_img(renderer, "images/title.png");
  title_img.y = 50;
  Image start_game_img = load_img(renderer, "images/start-game.png");
  start_game_img.y = 500;
  Image start_game_hover_img = load_img(renderer, "images/start-game-hover.png");
  start_game_hover_img.y = 500;
  Image hints_img = load_img(renderer, "images/hints.png");
  hints_img.y = 500 + start_game_img.h + 50;
   
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    error("opening audio device");

  snd_effects[0] = Mix_LoadWAV("audio/tower_shoot.wav");
  snd_effects[1] = Mix_LoadWAV("audio/tower_damage.wav");
  snd_effects[2] = Mix_LoadWAV("audio/beast_damage.wav");
  snd_effects[3] = Mix_LoadWAV("audio/entity_enabled.wav");
  snd_effects[4] = Mix_LoadWAV("audio/tower_explosion.wav");
  snd_effects[5] = Mix_LoadWAV("audio/levelup.wav");

  for (int i = 0; i < num_snd_effects; ++i)
    if (!snd_effects[i])
      error("loading wav");

  center_img(&title_img, &vp);
  center_img(&start_game_img, &vp);
  center_img(&start_game_hover_img, &vp);
  center_img(&hints_img, &vp);

  SDL_Event evt;
  bool exit_game = false;
  while (!exit_game) {
    while (SDL_PollEvent(&evt)) {
      if (evt.type == SDL_QUIT || (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE)) {
        exit_game = true;
      }
      else if (evt.type == SDL_MOUSEBUTTONDOWN && is_mouseover(&start_game_img, evt.button.x, evt.button.y)) {
        SDL_SetCursor(arrow_cursor);
        play_level(window, renderer);
      }
    }

    // set BG color
    if (SDL_SetRenderDrawColor(renderer, 44, 34, 30, 255) < 0)
      error("setting bg color");
    if (SDL_RenderClear(renderer) < 0)
      error("clearing renderer");
    
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    
    render_img(renderer, &title_img);
    if (is_mouseover(&start_game_img, mouse_x, mouse_y)) {
      render_img(renderer, &start_game_hover_img);
      SDL_SetCursor(hand_cursor);
    }
    else {
      render_img(renderer, &start_game_img);
      SDL_SetCursor(arrow_cursor);
    }
    render_img(renderer, &hints_img);
    
    SDL_RenderPresent(renderer);
    SDL_Delay(10);
  }

  for (int i = 0; i < num_snd_effects; ++i)
    Mix_FreeChunk(snd_effects[i]);
  Mix_Quit();

  // if (SDL_SetWindowFullscreen(window, 0) < 0)
  //   error("exiting fullscreen");

  SDL_FreeCursor(arrow_cursor);
  SDL_FreeCursor(hand_cursor);

  SDL_DestroyTexture(title_img.tex);
  SDL_DestroyTexture(start_game_img.tex);
  SDL_DestroyTexture(start_game_hover_img.tex);
  SDL_DestroyTexture(hints_img.tex);
  
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}

void play_level(SDL_Window* window, SDL_Renderer* renderer) {
  time_t seed = 1529597895; // 005; // time(NULL);
  srand(seed);
  printf("Seed: %lld\n", seed);

  // reset global variables
  num_collected_blocks = 100;
  grid_len = num_blocks_w * num_blocks_h;
  max_blocks = grid_len * block_density_pct * 3 / 100; // x3 b/c default is 20% density, but we need up to 60% due to mines
  unsigned int start_time = SDL_GetTicks();
  unsigned int pause_start = 0;
  
  // load game
  Entity* grid[grid_len];
  byte grid_flags[grid_len];

  Entity blocks[max_blocks];
  Entity beasts[max_beasts];
  Entity buildings[max_buildings];
  Entity nests[max_nests];
  
  Bullet bullets[max_bullets];

  start_pos = load(grid, grid_flags, blocks, beasts, buildings, nests, bullets);

  SDL_Texture* sprites = IMG_LoadTexture(renderer, "images/spritesheet.png");

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
        case SDL_MOUSEMOTION:
          on_mousemove(&evt, grid, grid_flags, buildings);
          break;
        case SDL_MOUSEBUTTONDOWN:
          on_mousedown(&evt, grid, grid_flags, buildings);
          break;
        case SDL_KEYDOWN:
          on_keydown(&evt, grid, &is_gameover, &is_paused, window);
          break;
        case SDL_MOUSEWHEEL:
          on_scroll(&evt);
          break;
      }
    }

    update(dt, last_loop_time, curr_time, grid, grid_flags, buildings, beasts, nests, bullets);
    render(renderer, sprites, grid, grid_flags, blocks, buildings, beasts, nests, bullets, start_time);

    SDL_Delay(10);
    last_loop_time = curr_time;
  }

  SDL_DestroyTexture(sprites);
}

int load(Entity* grid[], byte grid_flags[], Entity blocks[], Entity beasts[], Entity buildings[], Entity nests[], Bullet bullets[]) {
  // have to manually init b/c C doesn't allow initializing VLAs w/ {0}
  for (int i = 0; i < grid_len; ++i) {
    grid[i] = NULL;
    grid_flags[i] = 0;
  }

  // precreate all turrets/bullets/beasts as deleted (has to be before placing the starting fortress)
  for (int i = 0; i < max_buildings; ++i)
    buildings[i].flags = DELETED;

  for (int i = 0; i < max_bullets; ++i)
    bullets[i].flags = DELETED;

  for (int i = 0; i < max_beasts; ++i)
    beasts[i].flags = ENEMY | DELETED;

  for (int i = 0; i < max_nests; ++i)
    nests[i].flags = ENEMY | DELETED;

  gen_water(grid_flags, 0, 0, 0, 0, 0,0, num_blocks_w);
  remove_sm_islands(grid_flags);
  remove_sm_lakes(grid_flags);

  int num_tries = 0;
  int max_size = 0;
  int start_pos = -1;
  for (int i = 0; i < 30; ++i) {
    int pos = find_avail_pos(grid, grid_flags);
    int size = calc_island_size(pos, grid_flags);
    printf("%d->%d,", pos, size);
    if (size > max_size) {
      start_pos = pos;
      max_size = size;
    }
  }

  printf(" Start: %d, Size: %d\n\n", start_pos, max_size);

  // build a starting fortress
  int start_x = to_x(start_pos);
  int start_y = to_y(start_pos);
  place_entity(start_x, start_y, grid, grid_flags, buildings);

  // scroll so that the starting pos is in the center
  scroll_to(start_x * block_w - vp.w / 2, start_y * block_h - vp.h / 2);

  // create nests -- flood-fill before & clear after so the first nests are on the same island
  flood_fill_island(start_pos, grid_flags);
  create_nests(level, start_pos, nests, grid, grid_flags);
  clear_flood_fill(grid_flags);
  return start_pos;
}

void create_nests(int level, int start_pos, Entity nests[], Entity* grid[], byte grid_flags[]) {
  int start_x = to_x(start_pos);
  int start_y = to_y(start_pos);
  int num_starting_nests = level * 2;
  int start_dist = level * 20;

  for (int i = 0; i < num_starting_nests; ++i) {
    nests[i].flags &= (~DELETED); // clear deleted bit
    
    int pos, x, y;
    double dist;
    bool is_island_ok;
    do {
      pos = find_avail_pos(grid, grid_flags);
      x = to_x(pos);
      y = to_y(pos);
      dist = calc_dist(x, y, start_x, start_y);
      if (level == 1)
        is_island_ok = grid_flags[pos] & PROCESSED;
      else
        is_island_ok = true;

    } while (!is_island_ok || dist < start_dist - 2 || dist > start_dist + 2);

    nests[i].x = x;
    nests[i].y = y;
    nests[i].create_time = SDL_GetTicks();
    nests[i].health = nest_health;
    grid[pos] = &nests[i];
  }
}

void gen_water(byte grid_flags[], short top_left, short top_right, short bottom_left, short bottom_right, int x, int y, int w) {
  short water_level = 0;
  short avg = (top_left + top_right + bottom_left + bottom_right) / 4;
  short deviation = rand() % USHRT_MAX - SHRT_MAX; // generate a random signed short
  short center = clamp(avg + deviation, SHRT_MIN, SHRT_MAX);
  
  // for now, set center val to top center, bottom center, right center, left center
  if (center < water_level) {
    grid_flags[to_pos(x+w/2, y)] |= WATER; // top center
    grid_flags[to_pos(x+w/2, y+w-1)] |= WATER; // bottom center
    grid_flags[to_pos(x, y+w/2)] |= WATER; // left center
    grid_flags[to_pos(x+w-1, y+w/2)] |= WATER; // right center
  }
  
  // recurse to calc nested squares
  if (w >= 2) {
    w = w / 2;
    gen_water(grid_flags, center, center, center, center, x,y, w);
    gen_water(grid_flags, center, center, center, center, x+w,y, w);
    gen_water(grid_flags, center, center, center, center, x,y+w, w);
    gen_water(grid_flags, center, center, center, center, x+w,y+w, w);
  }
}

void remove_sm_islands(byte grid_flags[]) {
  for (int x = 0; x < num_blocks_w; ++x) {
    for (int y = 0; y < num_blocks_h; ++y) {
      if (grid_flags[to_pos(x, y)] & WATER)
        continue;

      bool has_adj_land = false;
      if (x > 0 && !(grid_flags[to_pos(x - 1, y)] & WATER))
        has_adj_land = true;
      if (y > 0 && !(grid_flags[to_pos(x, y - 1)] & WATER))
        has_adj_land = true;
      if (x < (num_blocks_w - 1) && !(grid_flags[to_pos(x + 1, y)] & WATER))
        has_adj_land = true;
      if (y < (num_blocks_h - 1) && !(grid_flags[to_pos(x, y + 1)] & WATER))
        has_adj_land = true;

      // if it's a tiny 1-square island, flood it w/ water
      if (!has_adj_land)
        grid_flags[to_pos(x, y)] |= WATER;
    }
  }
}

void remove_sm_lakes(byte grid_flags[]) {
  for (int x = 0; x < num_blocks_w; ++x) {
    for (int y = 0; y < num_blocks_h; ++y) {
      if (!(grid_flags[to_pos(x, y)] & WATER))
        continue;

      bool has_adj_water = false;
      if (x > 0 && grid_flags[to_pos(x - 1, y)] & WATER)
        has_adj_water = true;
      if (y > 0 && grid_flags[to_pos(x, y - 1)] & WATER)
        has_adj_water = true;
      if (x < (num_blocks_w - 1) && grid_flags[to_pos(x + 1, y)] & WATER)
        has_adj_water = true;
      if (y < (num_blocks_h - 1) && grid_flags[to_pos(x, y + 1)] & WATER)
        has_adj_water = true;

      // if it's a tiny 1-square lake, remove the water
      if (!has_adj_water)
        grid_flags[to_pos(x, y)] &= ~(WATER);
    }
  }
}

int calc_island_size(int pos, byte grid_flags[]) {
  flood_fill_island(pos, grid_flags);

  // count island tiles & reset PROCESSED bit for whole grid
  int size = 0;
  for (int i = 0; i < grid_len; ++i)
    if (grid_flags[i] & PROCESSED)
      size++;
  
  clear_flood_fill(grid_flags);
  return size;
}

void clear_flood_fill(byte grid_flags[]) {
  for (int i = 0; i < grid_len; ++i)
    if (grid_flags[i] & PROCESSED)
      grid_flags[i] &= (~PROCESSED);
}

void flood_fill_island(int pos, byte grid_flags[]) {
  if (grid_flags[pos] & WATER || grid_flags[pos] & PROCESSED)
    return;

  grid_flags[pos] |= PROCESSED;
  int x = to_x(pos);
  int y = to_y(pos);

  // TODO: create an array of direction vectors & iterate it for this
  // maybe an array of all 8, and just loop the non-diagonal ones?
  if (is_in_grid(x + 1, y))
    flood_fill_island(to_pos(x + 1, y), grid_flags);
  if (is_in_grid(x, y + 1))
    flood_fill_island(to_pos(x, y + 1), grid_flags);
  if (is_in_grid(x - 1, y))
    flood_fill_island(to_pos(x - 1, y), grid_flags);
  if (is_in_grid(x, y - 1))
    flood_fill_island(to_pos(x, y - 1), grid_flags);
}

void on_mousemove(SDL_Event* evt, Entity* grid[], byte grid_flags[], Entity buildings[]) {
  if (!(evt->motion.state & SDL_BUTTON_LMASK))
    return;

  int x = (evt->button.x + vp.x) / block_w;
  int y = (evt->button.y + vp.y) / block_h;
  place_entity(x, y, grid, grid_flags, buildings);
}

void on_mousedown(SDL_Event* evt, Entity* grid[], byte grid_flags[], Entity buildings[]) {
  // check for button-clicks
  if (contains(&road_btn, evt->button.x, evt->button.y)) {
    selected_btn = &road_btn;
    return;
  }
  else if (contains(&wall_btn, evt->button.x, evt->button.y)) {
    selected_btn = &wall_btn;
    return;
  }
  else if (contains(&fortress_btn, evt->button.x, evt->button.y)) {
    selected_btn = &fortress_btn;
    return;
  }
  else if (contains(&mine_btn, evt->button.x, evt->button.y)) {
    selected_btn = &mine_btn;
    return;
  }
  else if (contains(&bridge_btn, evt->button.x, evt->button.y)) {
    selected_btn = &bridge_btn;
    return;
  }

  int x = (evt->button.x + vp.x) / block_w;
  int y = (evt->button.y + vp.y) / block_h;
  place_entity(x, y, grid, grid_flags, buildings);
}

void place_entity(int x, int y, Entity* grid[], byte grid_flags[], Entity buildings[]) {
  if (!is_in_grid(x, y))
    return;

  // try to place a road/fortress/bridge
  int pos = to_pos(x, y);

  int num_required_blocks;

  if (selected_btn != &bridge_btn && grid_flags[pos] & WATER)
    return; // can only build bridges on water
  else if (selected_btn == &bridge_btn && !(grid_flags[pos] & WATER))
    return; // can't build bridges on land

  if (selected_btn == &road_btn)
    num_required_blocks = num_blocks_per_road;
  else if (selected_btn == &bridge_btn)
    num_required_blocks = num_blocks_per_bridge;
  else if (selected_btn == &wall_btn)
    num_required_blocks = num_blocks_per_wall;
  else if (selected_btn == &fortress_btn)
    num_required_blocks = num_blocks_per_turret;
  else if (selected_btn == &mine_btn)
    num_required_blocks = num_blocks_per_mine;
  else
    error("selected button is not road/fortress/mine/bridge");

  // can't build w/out required blocks; can't build on top of an entity
  if (grid[pos] || num_collected_blocks < num_required_blocks)
    return;

  // can't build anything on an existing road/bridge
  if (grid_flags[pos] & ROAD)
    return;

  if (selected_btn == &fortress_btn || selected_btn == &mine_btn || selected_btn == &wall_btn) {

    // if there's nothing adjacent, disallow if there is an existing building
    if (!is_adj(grid, grid_flags, x, y)) {
      bool are_buildings = false;
      for (int i = 0; i < max_buildings; ++i)
        if (!(buildings[i].flags & DELETED))
          are_buildings = true;

      if (are_buildings)
        return;
    }

    Entity* building = NULL;
    for (int i = 0; i < max_buildings; ++i) {
      if (buildings[i].flags & DELETED) {
        building = &buildings[i];
        break;
      }
    }

    if (!building)
      return; // TODO: alert the player if max_buildings has been reached

    if (selected_btn == &mine_btn) {
      building->flags = MINE; // clear DELETED, set entity bit
      building->health = mine_health;
    }
    else if (selected_btn == &wall_btn) {
      building->flags = WALL;
      building->health = wall_health;
    }
    else {
      building->flags = TURRET; // clear DELETED, set entity bits
      building->health = fortress_health;
    }
    
    building->create_time = SDL_GetTicks();
    set_xy(building, grid, x, y);
  }
  else {
    // abort if there's nothing adjacent
    if (!is_adj(grid, grid_flags, x, y))
      return;

    grid_flags[pos] |= ROAD; // set road bit
  }

  num_collected_blocks -= num_required_blocks;
  update_explored(pos, grid_flags);
}

void update_explored(int pos, byte grid_flags[]) {
  int x = to_x(pos);
  int y = to_y(pos);
  for (int i = 0; i < grid_len; ++i) {
    double dist = calc_dist(x, y, to_x(i), to_y(i));
    if (dist < explored_dist)
      grid_flags[i] |= EXPLORED;
  }
}

void on_keydown(SDL_Event* evt, Entity* grid[], bool* is_gameover, bool* is_paused, SDL_Window* window) {
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
    case SDLK_LEFT:
      scroll_to(vp.x - 20, vp.y);
      break;
    case SDLK_RIGHT:
      scroll_to(vp.x + 20, vp.y);
      break;
    case SDLK_UP:
      scroll_to(vp.x, vp.y - 20);
      break;
    case SDLK_DOWN:
      scroll_to(vp.x, vp.y + 20);
      break;
  }
}

void on_scroll(SDL_Event* evt) {
  int dx = evt->wheel.x * 8;
  int dy = evt->wheel.y * 8;
  if (evt->wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
    dy = -dy;

  scroll_to(vp.x + dx, vp.y + dy);
}

void scroll_to(int x, int y) {
  vp.x = clamp(x, 0, num_blocks_w * block_w - vp.w);
  vp.y = clamp(y, -header_height, num_blocks_h * block_h - vp.h - header_height);
}

void update(double dt, unsigned int last_loop_time, unsigned int curr_time, Entity* grid[], byte grid_flags[], Entity buildings[], Entity beasts[], Entity nests[], Bullet bullets[]) {
  // mining
  for (int i = 0; i < max_buildings; ++i) {
    Entity* mine = &buildings[i];
    if (!(mine->flags & DELETED) && (mine->flags & MINE) && crossed_interval(last_loop_time, curr_time, mine, mine_interval))
      num_collected_blocks++;
  }

  // fortress firing
  for (int i = 0; i < max_buildings; ++i) {
    Entity* turret = &buildings[i];
    if (turret->flags & DELETED)
      continue;
    if (!(turret->flags & TURRET))
      continue;
    if (!crossed_interval(last_loop_time, curr_time, turret, turret_fire_interval))
      continue;

    int pos = to_pos(turret->x, turret->y);
    Entity* beast = closest_entity(turret->x, turret->y, beasts, max_beasts);
    double beast_dist = -1;
    if (beast)
      beast_dist = calc_dist(beast->x, beast->y, turret->x, turret->y);

    Entity* nest = closest_entity(turret->x, turret->y, nests, max_nests);
    double nest_dist = -1;
    if (nest)
      nest_dist = calc_dist(nest->x, nest->y, turret->x, turret->y);
    
    Entity* enemy;
    double dist;
    if (beast && nest) {
      if (beast_dist < nest_dist) {
        enemy = beast;
        dist = beast_dist;
      }
      else {
        enemy = nest;
        dist = nest_dist;
      }
    }
    else if (beast) {
      enemy = beast;
      dist = beast_dist;
    }
    else if (nest) {
      enemy = nest;
      dist = nest_dist;
    }
    else {
      continue;
    }

    if (dist > fortress_attack_dist)
      continue;

    Mix_PlayChannel(-1, snd_effects[0], 0);

    // dividing by the distance gives us a normalized 1-unit vector
    double dx = (enemy->x - turret->x) / dist;
    double dy = (enemy->y - turret->y) / dist;
    for (int j = 0; j < max_bullets; ++j) {
      Bullet* b = &bullets[j];
      if (b->flags & DELETED) {
        b->flags &= (~DELETED); // clear the DELETED bit
        
        // start in top/left corner
        int start_x = turret->x * block_w;
        int start_y = turret->y * block_h;
        if (dx > 0)
          start_x += block_w;
        else if (dx == 0)
          start_x += block_w / 2;
        else
          start_x -= 1; // so it's not on top of itself

        if (dy > 0)
          start_y += block_h;
        else if (dy == 0)
          start_y += block_h / 2;
        else
          start_y -= 1; // so it's not on top of itself

        b->x = start_x;
        b->y = start_y;
        b->dx = dx;
        b->dy = dy;
        break;
      }
    }
    // TODO: determine when max_bullets is exceeded & notify player?
  }

  // beast spawning
  for (int i = 0; i < max_nests; ++i) {
    Entity* nest = &nests[i];
    if (nest->flags & DELETED)
      continue;
    if (!crossed_interval(last_loop_time, curr_time, nest, beast_spawn_interval))
      continue;

    int spawn_pos = choose_adj_pos(nest, NULL, grid, grid_flags);
    if (spawn_pos == -1)
      continue;

    for (int i = 0; i < max_beasts; ++i) {
      Entity* beast = &beasts[i];
      // find deleted beast & revive it
      if (beast->flags & DELETED) {
        beast->flags &= (~DELETED); // clear deleted bit
        beast->health = beast_health;
        beast->create_time = SDL_GetTicks();
        set_pos(beast, grid, spawn_pos);
        break;
      }
    }
  }

  // beast moving
  for (int i = 0; i < max_beasts; ++i) {
    Entity* beast = &beasts[i];
    if (beast->flags & DELETED)
      continue;

    if (!crossed_interval(last_loop_time, curr_time, beast, beast_move_interval))
      continue;

    Entity* closest_turret = NULL;
    double closest_dist = beast_attack_dist;

    for (int i = 0; i < max_buildings; ++i) {
      if (buildings[i].flags & DELETED)
        continue;

      double dist = calc_dist(buildings[i].x, buildings[i].y, beast->x, beast->y);
      if (dist < closest_dist) {
        closest_dist = dist;
        closest_turret = &buildings[i];
      }
    }

    if (closest_turret && is_ent_adj(closest_turret, beast)) {
      inflict_damage(closest_turret, grid);
      if (closest_turret->flags & DELETED) {
        closest_turret = NULL;
        Mix_PlayChannel(-1, snd_effects[4], 0);
      }
      else {
        Mix_PlayChannel(-1, snd_effects[1], 0);
      }
    }
    
    int dest_pos = choose_adj_pos(beast, closest_turret, grid, grid_flags);
    if (dest_pos > -1)
      move(beast, grid, to_x(dest_pos), to_y(dest_pos));
  }

  // update bullet positions; handle bullet collisions
  for (int i = 0; i < max_bullets; ++i) {
    if (bullets[i].flags & DELETED)
      continue;

    bullets[i].x += bullets[i].dx * bullet_speed * dt;
    bullets[i].y += bullets[i].dy * bullet_speed * dt;
    // delete bullets that have gone out of the game
    if ((bullets[i].x < 0 || bullets[i].x > num_blocks_w * block_w) ||
      bullets[i].y < 0 || bullets[i].y > num_blocks_h * block_h) {
        bullets[i].flags |= DELETED; // set deleted bit on
        continue;
    }

    int grid_x = bullets[i].x / block_w;
    int grid_y = bullets[i].y / block_h;
    if (is_in_grid(grid_x, grid_y)) {
      int pos = to_pos(grid_x, grid_y);
      Entity* ent = grid[pos];
      if (ent && ent->flags & TURRET) {
        bullets[i].flags |= DELETED;
        continue;
      }
      else if (ent && ent->flags & ENEMY) {
        inflict_damage(ent, grid);
        bullets[i].flags |= DELETED;
        if (ent->flags & DELETED && !countLiveEntities(nests, max_nests)) {
          Mix_PlayChannel(-1, snd_effects[5], 0);
          level++;
          if (level <= 4)
            create_nests(level, start_pos, nests, grid, grid_flags);
        }
        else {
          Mix_PlayChannel(-1, snd_effects[2], 0);
        }
      }
    }
  }
}

bool crossed_interval(unsigned int last_loop_time, unsigned int curr_time, Entity* ent, int interval) {
  int floored_last = (last_loop_time - ent->create_time)/interval;
  int floored_now = (curr_time - ent->create_time)/interval;
  return floored_last != floored_now;
}

void render(SDL_Renderer* renderer, SDL_Texture* sprites, Entity* grid[], byte grid_flags[], Entity blocks[], Entity buildings[], Entity beasts[], Entity nests[], Bullet bullets[], unsigned int start_time) {
  // set BG color
  if (SDL_SetRenderDrawColor(renderer, 38, 67, 44, 255) < 0)
    error("setting bg color");
  if (SDL_RenderClear(renderer) < 0)
    error("clearing renderer");

  if (SDL_SetRenderDrawColor(renderer, 145, 103, 47, 255) < 0)
    error("setting land color");

  for (int i = 0; i < grid_len; ++i) {
    int x = to_x(i);
    int y = to_y(i);

    if (grid_flags[i] & WATER) {
      for (int corner_x = 0; corner_x <= 1; ++corner_x) {
        for (int corner_y = 0; corner_y <= 1; ++corner_y) {
          int adj_x = corner_x ? x + 1 : x - 1;
          int adj_y = corner_y ? y + 1 : y - 1;

          // treat edges as water
          if (adj_x < 0 || adj_x >= num_blocks_w || adj_y < 0 || adj_y >= num_blocks_h)
            continue;

          // if there is adjacent land in both directions & diagonally, round the (interior/acute) corner
          if (!(grid_flags[to_pos(adj_x, y)] & WATER) && !(grid_flags[to_pos(x, adj_y)] & WATER) && !(grid_flags[to_pos(adj_x, adj_y)] & WATER))
            render_corner(renderer, sprites, 8 + corner_x, 0 + corner_y, x * 2 + corner_x, y * 2 + corner_y);
        }
      }
    }
    else {
      // draw each corner, rounded if necessary
      for (int corner_x = 0; corner_x <= 1; ++corner_x) {
        for (int corner_y = 0; corner_y <= 1; ++corner_y) {
          int adj_x = corner_x ? x + 1 : x - 1;
          int adj_y = corner_y ? y + 1 : y - 1;

          // treat edges as water
          // if there is no adjacent land in either direction, round the (exterior/obtuse) corner
          if ((adj_x < 0 || adj_x >= num_blocks_w || grid_flags[to_pos(adj_x, y)] & WATER) &&
            (adj_y < 0 || adj_y >= num_blocks_h || grid_flags[to_pos(x, adj_y)] & WATER)) {
              render_corner(renderer, sprites, 6 + corner_x, 0 + corner_y, x * 2 + corner_x, y * 2 + corner_y);
          }
          else {
            SDL_Rect land_rect = {
              .x = x * block_w + corner_x * block_w/2 - vp.x,
              .y = y * block_h + corner_y * block_h/2 - vp.y,
              .w = block_w/2,
              .h = block_h/2
            };
            if (SDL_RenderFillRect(renderer, &land_rect) < 0)
              error("filling land rect");
          }
        }
      }
    }
  }

  // render stones
  for (int i = 0; i < max_blocks; ++i) {
    if (blocks[i].flags & DELETED)
      continue;

    render_sprite(renderer, sprites, 1,3, blocks[i].x,blocks[i].y);
  }

  // draw buildings
  for (int i = 0; i < max_buildings; ++i) {
    Entity* turret = &buildings[i];
    if (turret->flags & DELETED)
      continue;

    if (turret->flags & MINE) {
      render_sprite(renderer, sprites, 0,3, buildings[i].x,buildings[i].y);
    }
    else if (turret->flags & TURRET) {
      render_sprite(renderer, sprites, 4,5, buildings[i].x,buildings[i].y);
    }
    else {
      bool is_above = is_wall(grid, turret->x, turret->y - 1);
      bool is_below = is_wall(grid, turret->x, turret->y + 1);
      bool is_left = is_wall(grid, turret->x - 1, turret->y);
      bool is_right = is_wall(grid, turret->x + 1, turret->y);

      int x_offset = (turret->health < fortress_health) ? 4 : 0;

      // top-left corner
      render_corner(renderer, sprites, (is_left ? 0 : 2) + x_offset,is_above ? 10 : 8, turret->x * 2,turret->y * 2);
      // top-right corner
      render_corner(renderer, sprites, (is_right ? 1 : 3) + x_offset,is_above ? 10 : 8, turret->x * 2 + 1,turret->y * 2);
      // bottom-left corner
      render_corner(renderer, sprites, (is_left ? 0 : 2) + x_offset,is_below ? 10 : 11, turret->x * 2,turret->y * 2 + 1);
      // bottom-right corner
      render_corner(renderer, sprites, (is_right ? 1 : 3) + x_offset,is_below ? 10 : 11, turret->x * 2 + 1,turret->y * 2 + 1);
    }
  }

  if (SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255) < 0)
    error("setting Bullet color");
  for (int i = 0; i < max_bullets; ++i) {
    if (bullets[i].flags & DELETED)
      continue;
    
    int x = bullets[i].x - vp.x;
    int y = bullets[i].y - vp.y;
    SDL_Rect bullet_rect = {
      .x = x,
      .y = y,
      .w = bullet_w,
      .h = bullet_h
    };
    if (SDL_RenderFillRect(renderer, &bullet_rect) < 0)
      error("filling bullet rect");
  }

  // draw roads
  for (int i = 0; i < grid_len; ++i) {
    if (grid_flags[i] & ROAD && !(grid_flags[i] & WATER)) {
      int x = to_x(i);
      int y = to_y(i);

      bool is_above = is_road(grid_flags, x, y - 1);
      bool is_below = is_road(grid_flags, x, y + 1);
      bool is_left = is_road(grid_flags, x - 1, y);
      bool is_right = is_road(grid_flags, x + 1, y);

      if (is_above && is_below) {
        if (is_left && is_right)
          render_sprite(renderer, sprites, 3,3, x,y);
        else if (is_left)
          render_sprite(renderer, sprites, 4,1, x,y);
        else if (is_right)
          render_sprite(renderer, sprites, 5,1, x,y);
        else
          render_sprite(renderer, sprites, 3,1, x,y);
      }
      else if (is_left && is_right) {
        if (is_above)
          render_sprite(renderer, sprites, 4,2, x,y);
        else if (is_below)
          render_sprite(renderer, sprites, 5,2, x,y);
        else
          render_sprite(renderer, sprites, 3,2, x,y);
      }
      else if (is_above) {
        if (is_left)
          render_sprite(renderer, sprites, 4,3, x,y);
        else if (is_right)
          render_sprite(renderer, sprites, 5,3, x,y);
        else
          render_sprite(renderer, sprites, 3,1, x,y); // vert default
      }
      else if (is_below) {
        if (is_left)
          render_sprite(renderer, sprites, 4,4, x,y);
        else if (is_right)
          render_sprite(renderer, sprites, 5,4, x,y);
        else
          render_sprite(renderer, sprites, 3,1, x,y); // vert default
      }
      else {
        render_sprite(renderer, sprites, 3,2, x,y); // horiz default
      }
    }
  }

  // draw nests
  for (int i = 0; i < max_nests; ++i) {
    Entity* nest = &nests[i];
    if (nest->flags & DELETED)
      continue;

    render_sprite(renderer, sprites, 5,0, nest->x, nest->y);
  }

  // draw bridges
  for (int i = 0; i < grid_len; ++i) {
    if (grid_flags[i] & ROAD && grid_flags[i] & WATER) {
      int x = to_x(i);
      int y = to_y(i);
      bool road_above = is_road(grid_flags, x, y - 1);
      bool road_below = is_road(grid_flags, x, y + 1);
      bool bridge_above = road_above && (grid_flags[i] & WATER);
      bool bridge_below = road_below && (grid_flags[i] & WATER);

      int y_offset = 0;
      if (!bridge_above && bridge_below)
        y_offset = 1;
      else if (bridge_above && bridge_below)
        y_offset = 2;
      else if (bridge_above && !bridge_below)
        y_offset = 3;

      render_sprite(renderer, sprites, 6,y_offset, to_x(i), to_y(i));
    }
  }

  // draw beasts in & out of water
  for (int i = 0; i < max_beasts; ++i) {
    if (beasts[i].flags & DELETED)
      continue;

    int sprite_x_pos = 0;
    int sprite_y_pos = 1;
    if (grid_flags[to_pos(beasts[i].x, beasts[i].y)] & WATER)
      sprite_y_pos += 1;
    if (beasts[i].health == 2)
      sprite_x_pos = 1;
    else if (beasts[i].health == 1)
      sprite_x_pos = 2;

    render_sprite(renderer, sprites, sprite_x_pos,sprite_y_pos, beasts[i].x, beasts[i].y);
  }

  // draw black unexplored mask
  for (int i = 0; i < grid_len; ++i) {
    if (grid_flags[i] & EXPLORED)
      continue;

    int x = to_x(i);
    int y = to_y(i);

    // if adjacent cell is explored, do 50% opacity mask
    if ((is_in_grid(x + 1, y) && grid_flags[to_pos(x + 1, y)] & EXPLORED) ||
      (is_in_grid(x - 1, y) && grid_flags[to_pos(x - 1, y)] & EXPLORED) ||
      (is_in_grid(x, y + 1) && grid_flags[to_pos(x, y + 1)] & EXPLORED) ||
      (is_in_grid(x, y - 1) && grid_flags[to_pos(x, y - 1)] & EXPLORED)) {
      if (SDL_SetRenderDrawColor(renderer, 0, 0, 0, 125) < 0)
        error("setting unexplored half-mask");
    }
    else {
      if (SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) < 0)
        error("setting unexplored mask");
    }

    SDL_Rect unexplored_r = {
      .x = x * block_w - vp.x,
      .y = y * block_h - vp.y,
      .w = block_w,
      .h = block_h
    };
    if (SDL_RenderFillRect(renderer, &unexplored_r) < 0)
      error("filling unexplored rect");
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

  int control_x = vp.w/2 - ui_bar_w/2;
  int num_btns = 5;
  double space_per_btn = ui_bar_w / num_btns;

  road_btn.x = control_x + space_per_btn * 0;
  wall_btn.x = control_x + space_per_btn * 1;
  fortress_btn.x = control_x + space_per_btn * 2;
  mine_btn.x = control_x + space_per_btn * 3;
  bridge_btn.x = control_x + space_per_btn * 4;

  double coin_bar_len = toExponential(num_collected_blocks);

  // coin bar background
  SDL_Rect coin_bar_bg = {
    .x = control_x + 10,
    .y = 60,
    .w = (int)(40.0 * (5.0 + 2.5 + 1.25 + 0.625 + 0.3125 + 0.15625 + 0.078125)),
    .h = 5
  };
  if (SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255) < 0)
    error("setting filled coin bar bg color");
  if (SDL_RenderFillRect(renderer, &coin_bar_bg) < 0)
    error("filling coin bar bg");

  // tick marks
  if (SDL_SetRenderDrawColor(renderer, 59, 59, 59, 255) < 0)
    error("setting tick mark color");

  // ticks for required resources
  SDL_Rect tick = {.x = 0, .y = 58, .w = 2, .h = 9};
  tick.x = control_x + 10 + toExponential(num_blocks_per_road);
  if (SDL_RenderFillRect(renderer, &tick) < 0)
    error("filling tick");
  
  tick.x = control_x + 10 + toExponential(num_blocks_per_wall);
  if (SDL_RenderFillRect(renderer, &tick) < 0)
    error("filling tick");
  
  tick.x = control_x + 10 + toExponential(num_blocks_per_turret);
  if (SDL_RenderFillRect(renderer, &tick) < 0)
    error("filling tick");
  
  tick.x = control_x + 10 + toExponential(num_blocks_per_mine);
  if (SDL_RenderFillRect(renderer, &tick) < 0)
    error("filling tick");

  tick.x = control_x + 10 + toExponential(num_blocks_per_bridge);
  if (SDL_RenderFillRect(renderer, &tick) < 0)
    error("filling tick");

  // ticks at doubling/halving points
  tick.x = control_x + 10 + 40 * 5;
  if (SDL_RenderFillRect(renderer, &tick) < 0)
    error("filling tick");
  tick.x = control_x + 10 + 40 * (5 + 2.5);
  if (SDL_RenderFillRect(renderer, &tick) < 0)
    error("filling tick");
  tick.x = control_x + 10 + 40 * (5 + 2.5 + 1.25);
  if (SDL_RenderFillRect(renderer, &tick) < 0)
    error("filling tick");
  tick.x = control_x + 10 + 40 * (5 + 2.5 + 1.25 + 0.625);
  if (SDL_RenderFillRect(renderer, &tick) < 0)
    error("filling tick");
  tick.x = control_x + 10 + 40 * (5 + 2.5 + 1.25 + 0.625 + 0.3125);
  if (SDL_RenderFillRect(renderer, &tick) < 0)
    error("filling tick");

  // coin bar fill
  SDL_Rect coin_bar_rect = {
    .x = control_x + 10,
    .y = 60,
    .w = (int)coin_bar_len,
    .h = 5
  };
  if (SDL_SetRenderDrawColor(renderer, 59, 59, 59, 255) < 0)
    error("setting filled coin bar color");
  if (SDL_RenderFillRect(renderer, &coin_bar_rect) < 0)
    error("filling coin bar rect");

  // hover button styling
  int mouse_x, mouse_y;
  SDL_GetMouseState(&mouse_x, &mouse_y);
  
  if (contains(&road_btn, mouse_x, mouse_y))
    hover_btn = &road_btn;
  else if (contains(&wall_btn, mouse_x, mouse_y))
    hover_btn = &wall_btn;
  else if (contains(&fortress_btn, mouse_x, mouse_y))
    hover_btn = &fortress_btn;
  else if (contains(&mine_btn, mouse_x, mouse_y))
    hover_btn = &mine_btn;
  else if (contains(&bridge_btn, mouse_x, mouse_y))
    hover_btn = &bridge_btn;
  else
    hover_btn = NULL;

  if (hover_btn) {
    SDL_SetCursor(hand_cursor);
    if (SDL_SetRenderDrawColor(renderer, 44, 34, 30, 100) < 0)
      error("setting hover btn bg color");
    if (SDL_RenderFillRect(renderer, hover_btn) < 0)
      error("filling hover_btn rect");
  }
  else {
    SDL_SetCursor(arrow_cursor);
  }

  if (SDL_SetRenderDrawColor(renderer, 44, 34, 30, 255) < 0)
    error("setting selected btn bg color");

  if (SDL_RenderFillRect(renderer, selected_btn) < 0)
    error("filling selected_btn rect");

  if (SDL_SetRenderDrawColor(renderer, 227, 167, 11, 255) < 0)
    error("setting selected btn highlight");

  SDL_Rect selected_btn_highlight = {
    .x = selected_btn->x, .y = selected_btn->y, .w = selected_btn->w, .h = 3
  };
  if (SDL_RenderFillRect(renderer, &selected_btn_highlight) < 0)
    error("filling selected btn highlight");

  render_sprite_btn(renderer, sprites, 3,2, road_btn.x + 5, 13);
  render_sprite_btn(renderer, sprites, 0,0, wall_btn.x + 5, 13);
  render_sprite_btn(renderer, sprites, 4,5, fortress_btn.x + 5, 13);
  render_sprite_btn(renderer, sprites, 0,3, mine_btn.x + 5, 13);
  render_sprite_btn(renderer, sprites, 6,0, bridge_btn.x + 5, 13);
  render_sprite_btn(renderer, sprites, 2,3, control_x + ui_bar_w - 21, ui_bar_h - 16);

  if (SDL_SetRenderDrawColor(renderer, 0, 0, 0, 170) < 0)
    error("setting disabled overlay color");

  if (num_collected_blocks < num_blocks_per_road)
    if (SDL_RenderFillRect(renderer, &road_btn) < 0)
      error("filling disabled overlay");

  if (num_collected_blocks < num_blocks_per_wall)
    if (SDL_RenderFillRect(renderer, &wall_btn) < 0)
      error("filling disabled overlay");

  if (num_collected_blocks < num_blocks_per_turret)
    if (SDL_RenderFillRect(renderer, &fortress_btn) < 0)
      error("filling disabled overlay");

  if (num_collected_blocks < num_blocks_per_mine)
    if (SDL_RenderFillRect(renderer, &mine_btn) < 0)
      error("filling disabled overlay");

  if (num_collected_blocks < num_blocks_per_bridge)
    if (SDL_RenderFillRect(renderer, &bridge_btn) < 0)
      error("filling disabled overlay");

  if (SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255) < 0)
    error("setting text color");

  char level_str[10];
  snprintf(level_str, sizeof(level_str), "Level: %d", level);
  render_text(renderer, level_str, control_x + ui_bar_w + 5, 10, 1);

  char nests_str[10];
  snprintf(nests_str, sizeof(nests_str), "Nests: %d", countLiveEntities(nests, max_nests));
  render_text(renderer, nests_str, control_x + ui_bar_w + 5, 20, 1);

  unsigned int elapsed = SDL_GetTicks() - start_time;
  int sec = (int)(elapsed / 1000.0);
  int min = sec / 60;
  sec -= min * 60;
  char time_str[9]; // 9 digits goes up to -99999:59
  snprintf(time_str, sizeof(time_str), "%d:%02d", min, sec);
  render_text(renderer, time_str, vp.w - 80, 5, 2);

  SDL_RenderPresent(renderer);
}

int countLiveEntities(Entity arr[], int arr_len) {
  int count = 0;
  for (int i = 0; i < arr_len; ++i)
    if (!(arr[i].flags & DELETED))
      count++;
  
  return count;
}

double toExponential(int num_coins) {
  if (num_coins <= 40)
    return num_coins * 5;
  else if (num_coins <= 80)
    return 40.0 * 5.0 + (num_coins - 40.0) * 2.5;
  else if (num_coins <= 120)
    return 40.0 * (5.0 + 2.5) + (num_coins - 80.0) * 1.25;
  else if (num_coins <= 160)
    return 40.0 * (5.0 + 2.5 + 1.25) + (num_coins - 120.0) * 0.625;
  else if (num_coins <= 200)
    return 40.0 * (5.0 + 2.5 + 1.25 + 0.625) + (num_coins - 160.0) * 0.3125;
  else if (num_coins <= 240)
    return 40.0 * (5.0 + 2.5 + 1.25 + 0.625 + 0.3125) + (num_coins - 200.0) * 0.15625;
  else if (num_coins <= 280)
    return 40.0 * (5.0 + 2.5 + 1.25 + 0.625 + 0.3125 + 0.15625) + (num_coins - 240.0) * 0.078125;
  else
    return 40.0 * (5.0 + 2.5 + 1.25 + 0.625 + 0.3125 + 0.15625 + 0.078125);
}

// Grid Functions

bool in_bounds(int x, int y) {
  return x >= 0 && x < num_blocks_w &&
    y >= 0 && y < num_blocks_h;
}

int find_avail_pos(Entity* grid[], byte grid_flags[]) {
  int x;
  int y;
  int pos;
  do {
    x = rand() % num_blocks_w;
    y = rand() % num_blocks_h;
    pos = to_pos(x, y);
  } while (grid[pos] || grid_flags[pos] & WATER);
  return pos;
}

void move(Entity* ent, Entity* grid[], int x, int y) {
  remove_from_grid(ent, grid);
  set_xy(ent, grid, x, y);
}

void set_pos(Entity* ent, Entity* grid[], int pos) {
  set_xy(ent, grid, to_x(pos), to_y(pos));
}

void set_xy(Entity* ent, Entity* grid[], int x, int y) {
  ent->x = x;
  ent->y = y;
  grid[to_pos(x, y)] = ent;
}

void remove_from_grid(Entity* ent, Entity* grid[]) {
  int prev_pos = to_pos(ent->x, ent->y);
  grid[prev_pos] = NULL;
}

int to_x(int ix) {
  return ix % num_blocks_w;
}

int to_y(int ix) {
  return ix / num_blocks_w;
}

int to_pos(int x, int y) {
  int pos = x + y * num_blocks_w;
  if (pos < 0)
    error("position out of bounds (negative)");
  if (pos >= grid_len)
    error("position out of bounds (greater than grid size)");
  return pos;
}

bool is_in_grid(int x, int y) {
  if ((x < 0 || x >= num_blocks_w) ||
    (y < 0 || y >= num_blocks_h))
      return false;

  return true;
}


// Game-Specific Functions
bool is_ent_adj(Entity* ent1, Entity* ent2) {
  return abs(ent1->x - ent2->x) <= 1 && abs(ent1->y - ent2->y) <= 1;
}

bool is_adj(Entity* grid[], byte grid_flags[], int x, int y) {
  return is_road(grid_flags, x - 1, y) || is_building(grid, x - 1, y) ||
    is_road(grid_flags, x + 1, y) || is_building(grid, x + 1, y) ||
    is_road(grid_flags, x, y - 1) || is_building(grid, x, y - 1) ||
    is_road(grid_flags, x, y + 1) || is_building(grid, x, y + 1);
}

bool is_road(byte grid_flags[], int x, int y) {
  if (!is_in_grid(x, y))
    return false;
  return grid_flags[to_pos(x, y)] & ROAD;
}

bool is_building(Entity* grid[], int x, int y) {
  if (!is_in_grid(x, y))
    return false;

  int pos = to_pos(x, y);
  return grid[pos] && (grid[pos]->flags & TURRET || grid[pos]->flags & MINE || grid[pos]->flags & WALL);
}

bool is_wall(Entity* grid[], int x, int y) {
  if (!is_in_grid(x, y))
    return false;

  int pos = to_pos(x, y);
  return grid[pos] && (grid[pos]->flags & WALL);
}

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

void del_entity(Entity* ent, Entity* grid[]) {
  ent->flags |= DELETED; // flip DELETED bit on
  remove_from_grid(ent, grid);
}

int choose_adj_pos(Entity* beast, Entity* closest_turret, Entity* grid[], byte grid_flags[]) {
  int x = beast->x;
  int y = beast->y;

  int dir_x = 0;
  int dir_y = 0;
  if (closest_turret && closest_turret->x < x)
    dir_x = -1;
  else if (closest_turret && closest_turret->x > x)
    dir_x = 1;

  if (closest_turret && closest_turret->y < y)
    dir_y = -1;
  else if (closest_turret && closest_turret->y > y)
    dir_y = 1;

  bool found_direction = true;

  // a quarter of the time we want them to move randomly
  // this keeps them from being too deterministic & from getting stuck
  bool move_randomly = !closest_turret || rand() % 100 > 75;


  // if we're already next to the turret, we can't move any closer
  if (closest_turret && is_ent_adj(closest_turret, beast))
    move_randomly = true;
  
  // try to move towards the fortress, if possible
  if (!move_randomly && dir_x && dir_y && in_bounds(x + dir_x, y + dir_y) && !grid[to_pos(x + dir_x, y + dir_y)] && !(grid_flags[to_pos(x + dir_x, y + dir_y)] & WATER)) {
    x += dir_x;
    y += dir_y;
  }
  else if (!move_randomly && dir_x && in_bounds(x + dir_x, y) && !grid[to_pos(x + dir_x, y)] && !(grid_flags[to_pos(x + dir_x, y)] & WATER)) {
    x += dir_x;
  }
  else if (!move_randomly && dir_y && in_bounds(x, y + dir_y) && !grid[to_pos(x, y + dir_y)] && !(grid_flags[to_pos(x, y + dir_y)] & WATER)) {
    y += dir_y;
  }
  // if there's no delta in one dimension, try +/- 1
  else if (!move_randomly && !dir_x && in_bounds(x + 1, y + dir_y) && !grid[to_pos(x + 1, y + dir_y)] && !(grid_flags[to_pos(x + 1, y + dir_y)] & WATER)) {
    x += 1;
    y += dir_y;
  }
  else if (!move_randomly && !dir_x && in_bounds(x - 1, y + dir_y) && !grid[to_pos(x - 1, y + dir_y)] && !(grid_flags[to_pos(x - 1, y + dir_y)] & WATER)) {
    x -= 1;
    y += dir_y;
  }
  else if (!move_randomly && !dir_y && in_bounds(x + dir_x, y + 1) && !grid[to_pos(x + dir_x, y + 1)] && !(grid_flags[to_pos(x + dir_x, y + 1)] & WATER)) {
    x += dir_x;
    y += 1;
  }
  else if (!move_randomly && !dir_y && in_bounds(x + dir_x, y - 1) && !grid[to_pos(x + dir_x, y - 1)] && !(grid_flags[to_pos(x + dir_x, y - 1)] & WATER)) {
    x += dir_x;
    y -= 1;
  }
  else {
    // test all combinations of directions
    found_direction = false;
    for (int mv_x = -1; mv_x <= 1; ++mv_x) {
      if (!found_direction) {
        for (int mv_y = -1; mv_y <= 1; ++mv_y) {
          if (!mv_x && !mv_y)
            continue; // 0,0 isn't a real move

          if (in_bounds(x + mv_x, y + mv_y) && !grid[to_pos(x + mv_x, y + mv_y)] && !(grid_flags[to_pos(x + mv_x, y + mv_y)] & WATER)) {
            found_direction = true;
            break;
          }
        }
      }
    }
    if (found_direction) {
      int mv_x = -1;
      int mv_y = -1;
      do {
        int dir = rand() % 8; // there are 8 possible directions

        if (dir == 0) {
          mv_x = -1;
          mv_y = -1;
        }
        else if (dir == 1) {
          mv_x = 0;
          mv_y = -1;
        }
        else if (dir == 2) {
          mv_x = 1;
          mv_y = -1;
        }
        else if (dir == 3) {
          mv_x = -1;
          mv_y = 0;
        }
        else if (dir == 4) {
          mv_x = 1;
          mv_y = 0;
        }
        else if (dir == 5) {
          mv_x = -1;
          mv_y = 1;
        }
        else if (dir == 6) {
          mv_x = 0;
          mv_y = 1;
        }
        else { // dir == 7
          mv_x = 1;
          mv_y = 1;
        }
      } while (!in_bounds(x + mv_x, y + mv_y) || grid[to_pos(x + mv_x, y + mv_y)] || grid_flags[to_pos(x + mv_x, y + mv_y)] & WATER);
      x += mv_x;
      y += mv_y;
    }
  }

  if (!found_direction || !in_bounds(x, y))
    return -1;
  else
    return to_pos(x, y);
}

void inflict_damage(Entity* ent, Entity* grid[]) {
  ent->health--;
  if (ent->health <= 0)
    del_entity(ent, grid);
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

// TODO: it's probably a little more efficient to load the image into an sdl image
// then get the dimensions, then load it into a texture
// instead of loading it directly to a texture & then querying the texture...
Image load_img(SDL_Renderer* renderer, char* path) {
  Image img = {};
  img.tex = IMG_LoadTexture(renderer, path);
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
  SDL_Rect src = {.x = src_x * block_w / 2, .y = src_y * block_h / 2, .w = block_w / 2, .h = block_h / 2};
  SDL_Rect dest = {.x = dest_x * block_w - vp.x, .y = dest_y * block_h - vp.y, .w = block_w, .h = block_h};
  if (SDL_RenderCopy(renderer, sprites, &src, &dest) < 0)
    error("renderCopy");
}

void render_sprite_btn(SDL_Renderer* renderer, SDL_Texture* sprites, int src_x, int src_y, int dest_x_px, int dest_y_px) {
  SDL_Rect src = {.x = src_x * block_w / 2, .y = src_y * block_h / 2, .w = block_w / 2, .h = block_h / 2};
  SDL_Rect dest = {.x = dest_x_px, .y = dest_y_px, .w = block_w, .h = block_h};
  if (SDL_RenderCopy(renderer, sprites, &src, &dest) < 0)
    error("renderCopy btn");
}

void render_corner(SDL_Renderer* renderer, SDL_Texture* sprites, int src_x, int src_y, int dest_x, int dest_y) {
  SDL_Rect src = {.x = src_x * block_w/4, .y = src_y * block_h/4, .w = block_w/4, .h = block_h/4};
  SDL_Rect dest = {.x = dest_x * block_w/2 - vp.x, .y = dest_y * block_h/2 - vp.y, .w = block_w/2, .h = block_h/2};
  if (SDL_RenderCopy(renderer, sprites, &src, &dest) < 0)
    error("renderCopy");
}

// TODO: consolidate w/ below contains()
bool is_mouseover(Image* img, int x, int y) {
  return x >= img->x && x <= (img->x + img->w) &&
    y >= img->y && y <= (img->y + img->h);
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
