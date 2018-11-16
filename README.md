# Red Planet Game Engine

Red Planet is intended to be an easy, batteries-included 2D game engine that provides the basics needed for most 2D games:

Types of Entities:

* Player
* Enemies
* Weapons
* Collectables

Functionality:

* Viewport that follows the player
* Collisions
* Damage & Health
* Animation
* Tileset Importing
* Basic controller support for player movement

Basic Game Functionality:

* Start Screen, Level Intro/End Screens
* Pause Screen
* 2-Player Selection

If you just need to tweak parameters, set spritesheets, etc, you can do basic configuration with a JSON file.

If you need to do deeper customizations, you can do so in C (but you will need to recompile). In later versions, we hope to add bindings to dynamic languages (probably Javascript, Lua and Python), so you can write custom game logic in a high-level language of your choosing.

## Getting Started

```json
{
  "start_screen": "images/title.png",
  "start_screen_bg": [44, 34, 30]
}
```

## Scope

The core Red Planet library is focused on functionality that is useful across most 2D action genres (Platformers, Shooters, Action RPGs, Roguelikes, Real Time Strategy, etc). Functionality that is not commonly used across most genres should be relegated to a module.

## License

Red Planet is licensed under the terms of the [MIT license](LICENSE.md).
