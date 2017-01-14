# Red Planet Game Framework

Red Planet is an easy yet powerful framework for making 2D games in Lua and LÖVE. It integrates best-of-breed libraries from the LÖVE community:

* [bump](https://github.com/kikito/bump.lua) for Collision Detection
* [anim8](https://github.com/kikito/anim8) for Sprite Animation
* [gamera](https://github.com/kikito/gamera) for a Camera System
* [sti](https://github.com/karai17/Simple-Tiled-Implementation/) for Tiled Map Loading and Rendering

On top of these, Red Planet provides a layer of common code that game devs typically copy from project to project:

* Registering Map Objects & Tiles with the Collision System
* Tracking the Player with the Camera
* Top-Down Key Bindings & Movement
* Side-Scrolling Key Bindings & Movement

To-Do:
* Enemies & Hazards
* Damage & Health
* Bullets & Shooting
* Local Multi-Player

This common code is designed to have sensible defaults, but to also be configurable, so you can precisely control speed, acceleration, friction (and, once implemented, health, damage and AI movement/behavior). It is pluggable, so you can swap out the above systems with ones of your own or ones from the plugins folder.

To promote interoperability, Red Planet uses an Entity Component System ([tiny-ecs](https://github.com/bakpakin/tiny-ecs)) with conventions for commonly-used properties (for instance, `pos.x` & `pos.y` for position, and `vel.x` & `vel.y` for velocity).

## Getting Started

Copy the `rp` folder into your project's folder. To-Do: Upload to LuaRocks. 

## Creating & Rendering an Animated, Controllable Sprite

```lua
local rp = require('rp')

-- define spritesheet as a grid of 64x64 tiles
local sheet = rp.loadSheet('assets/princess.png', 64)

local player = {
  id = 'player1',
  sprite = sheet:sprite(1, 1), -- base sprite is top-left corner (x=1, y=1)
  animations = {
    up = sheet:anim('2-9', 1), -- up walking animation is (x=2-9, y=1)
    left = sheet:anim('2-9', 2),
    down = sheet:anim('2-9', 3),
    right = sheet:anim('2-9', 4)
  }
}

local world = rp.world(
  -- create each of the systems for the game
  rp.topDownControl('wasd', {id = 'player1'})
  rp.spriteRender()
)
world:add(player)
```

## Scope

The core Red Planet library is focused on functionality that is useful across many popular genres (Platformers, Shooters, Action RPGs, Roguelikes, Sandbox RPGs, Real Time Strategy, Tower Defense, etc). Functionality that is unique to a specific game or to a niche genre will be placed in a plugins folder.

## License

Red Planet is licensed under the terms of the [MIT license](LICENSE.md).
