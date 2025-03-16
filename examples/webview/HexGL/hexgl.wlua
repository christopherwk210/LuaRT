-- HexGL is an open source HTML5 racing game by Thibaut Despoulain (bkcore.com)
-- Released under the MIT License, available on https://github.com/BKcore/HexGL

local ui = require "ui"
require "webview"

local win = ui.Window("HexGL example - Powered by LuaRT", "raw")
local wv = ui.Webview(win, { url = "https://hexgl.bkcore.com/play" })
wv.align = "all"

function wv:onReady()
    wv.statusbar = false
    wv.devtools = false
    wv.contextmenu = false
    wv.acceleratorkeys = false
end

win:maximize()

ui.run(win):wait()