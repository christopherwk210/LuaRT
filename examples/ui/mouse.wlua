local ui = require "ui"
require "canvas"

ui.theme = "light"

local win = ui.Window("Mouse example", 640, 480)
local c = ui.Canvas(win)
c.align = "all"
c.cursor = "cross"
local lastx
local lasty

function c:onHover(x, y, buttons)
	c:begin()
	if not buttons or buttons.left then	
		self:line(lastx or x, lasty or y, x, y, 0x8c1affff, 3)
        lastx = x
        lasty = y	
    else
        lastx = nil
        lasty = nil
	end
	c:flip()
end

ui.run(win):wait()