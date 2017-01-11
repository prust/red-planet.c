local function DirectionControlSystem()
	return tiny.processingSystem(
		tiny.requireAll("controllable"),
		function(e, dt)
			local vel = e.vel
			local acceleration = 1000
			local friction = 2000
			local speed = 130
			
			local l, r = love.keyboard.isDown('a'), love.keyboard.isDown('d')
			local u, d = love.keyboard.isDown('w'), love.keyboard.isDown('s')
			local dir = {x = 0, y = 0}
			if l and not r then
			  dir.x = -1
			elseif r and not l then
				dir.x = 1
			end
			if u and not d then
				dir.y = -1
			elseif d and not u then
				dir.y = 1
			end

			for i, dim in ipairs({"x", "y"}) do
				if dir[dim] == 0 then
					if vel[dim] > 0 then
							vel[dim] = math.max(0, vel[dim] - friction * dt)
					elseif vel[dim] < 0 then
							vel[dim] = math.min(0, vel[dim] + friction * dt)
					end
				else
					if dir[dim] == -1 then
						vel[dim] = math.max(dir[dim] * speed, vel[dim] + dir[dim] * acceleration * dt)
					else
						vel[dim] = math.min(dir[dim] * speed, vel[dim] + dir[dim] * acceleration * dt)
					end
				end
			end
			
			if vel.x > 0 then
				e.animation = e.animation_right
			elseif vel.x < 0 then
		    e.animation = e.animation_left
			elseif vel.y < 0 then
			  e.animation = e.animation_up
			else
			  e.animation = e.animation_down
			end
		end
		)
end

return DirectionControlSystem