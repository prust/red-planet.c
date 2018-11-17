# Red Planet Game Engine

Red Planet is intended to be a small batteries-included 2D game engine that allows you to get started quickly and to focus on what makes your game unique instead of writing boilerplate code.

Types of Entities:

* Player
* Enemies
* Bullets
* Weapons
* Collectables

Functionality:

- [ ] Viewport that follows the player(s)
- [ ] Collisions
- [ ] Damage & Health
- [ ] Animation
- [ ] Tileset Importing (from [Tiled](https://www.mapeditor.org))
- [ ] Controller support

Basic Game Functionality:

- [ ] Start Screen
- [ ] Level Intro Screens
- [ ] Level End Screens
- [ ] Pause Screen
- [ ] 2-Player Selection

Basic customizations can be done in a JSON file, but custom logic and extensions need to be done in C (in later versions we hope to add bindings to Javascript, Lua and/or Python).

## Getting Started

```json
{
  "start_screen": "images/title.png",
  "start_screen_bg": [44, 34, 30]
}
```

## Scope

The Red Planet core is focused on functionality that is useful across most 2D action genres (Platformers, Shooters, Action RPGs, Roguelikes, Real Time Strategy, etc). Functionality that is not commonly used across most of these genres should be relegated to a module.

## License

Red Planet is licensed under the terms of the [MIT license](LICENSE.md).
