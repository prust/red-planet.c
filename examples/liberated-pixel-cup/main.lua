if arg[#arg] == "-debug" then require("mobdebug").start() end

-- TODO: move these requires into the systems
-- and pass initialization into the systems

-- include grandparent (../../) folder, so we can include 'rp' modules in examples
package.path = "../../?.lua;" .. package.path
tiny = require("rp.lib.tiny")

local sti = require("rp.lib.sti")
local gamera = require("rp.lib.gamera")
local anim8 = require("rp.lib.anim8")
local bump = require("rp.lib.bump")

--love.window.setFullscreen(true)

local tileMap = sti.new("assets/test")
local bumpWorld = bump.newWorld(tileMap.tilewidth * 2)
local w, h = tileMap.tilewidth * tileMap.width, tileMap.tileheight * tileMap.height
local camera = gamera.new(0, 0, w, h)

-- find object layer, instantiate player, delete object layer
local player
for ix, layer in ipairs(tileMap.layers) do
  if layer.type == "tilelayer" and layer.properties and layer.properties.isSolid then
    for y, tiles in ipairs(layer.data) do
        for x, tile in pairs(tiles) do
          bumpWorld:add(
              {isSolid = true},
              x * tileMap.tilewidth + tile.offset.x,
              y * tileMap.tileheight + tile.offset.y,
              tile.width,
              tile.height
          )
      end
    end
  elseif layer.type == "objectgroup" then
      for _, object in ipairs(layer.objects) do
          --local ctor = require("src.entities." .. object.type)
          --local e = ctor(object)
          if object.type == "Player" then
              player = {pos = {x = object.x, y = object.y}}
          end
      end
      tileMap:removeLayer(ix)
  end
end

-- load player animations
local img_princess = love.graphics.newImage("assets/princess.png")
player.sprite = img_princess
local g = anim8.newGrid(64, 64, img_princess:getWidth(), img_princess:getHeight())
player.animation_up = anim8.newAnimation(g('2-9', 1), 0.1)
player.animation_left = anim8.newAnimation(g('2-9', 2), 0.1)
player.animation_down = anim8.newAnimation(g('2-9', 3), 0.1)
player.animation_right = anim8.newAnimation(g('2-9', 4), 0.1)
player.animation = player.animation_right
player.fg = true
player.vel = {x = 0, y = 0}
player.hitbox = {w = 40, h = 55}
player.controllable = true
player.isSolid = true
player.cameraTrack = {xoffset = 16, yoffset = -35}

local world = tiny.world(
  require("rp.direction-control")(),
  require("rp.bump-physics")(bumpWorld),
  require("rp.camera-tracking")(camera),
  require("rp.tilemap-render")(camera, tileMap)
)

-- attempting to inject the sprite drawing into a map draw() callback
function love.load()
  tileMap:addCustomLayer("Sprite Layer", 3)
  local spriteLayer = tileMap.layers["Sprite Layer"]
  function spriteLayer:update()
    local dt = love.timer.getDelta()
  end
  
  local sprite_system = require("rp.sprite")(camera, "fg")
  function spriteLayer:draw()
    local dt = love.timer.getDelta()
    sprite_system:update(dt)
  end
end

world:add(player)

function love.draw()
  if world then
    local dt = love.timer.getDelta()
    world:update(dt)
  end
end