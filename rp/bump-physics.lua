local function collisionFilter(e1, e2)
    if e1.isSolid then
        if e2.isSolid then
            return 'slide'
        elseif e2.isBouncy then
            return 'bounce'
        else
            return 'cross'
        end
    end
    return nil
end

function BumpPhysicsSystem(bumpWorld)
    local ret = tiny.processingSystem(
        tiny.requireAll("pos", "vel", "hitbox"),
        function (e, dt)
            local pos = e.pos
            local vel = e.vel
            local cols, len
            pos.x, pos.y, cols, len = bumpWorld:move(e, pos.x + vel.x * dt, pos.y + vel.y * dt, collisionFilter)

            for i = 1, len do
                local col = cols[i]
                local collided = true
                if col.type == "touch" then
                    vel.x, vel.y = 0, 0
                elseif col.type == "slide" then
                    if col.normal.x == 0 then
                        vel.y = 0
                    else
                        vel.x = 0
                    end
                elseif col.type == "bounce" then
                    if col.normal.x == 0 then
                        vel.y = -vel.y
                    else
                        vel.x = -vel.x
                    end
                end

                if e.onCollision and col.type ~= "cross" then 
                    e:onCollision(col) 
                end
            end 
        end,
        function (e)
            local pos = e.pos
            bumpWorld:add(e, pos.x, pos.y, e.hitbox.w, e.hitbox.h)
        end,
        function (e)
            bumpWorld:remove(e)
        end
    )
    return ret
end

return BumpPhysicsSystem