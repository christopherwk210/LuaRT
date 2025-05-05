local ui = require "ui"
require "webview"

sys.currentdir = sys.File(arg[0]).path
local win = ui.Window("Hello World with Webview example", 640, 540)

local wv = ui.Webview(win, {url = "file:///hello.html"}, 0, 46)
wv.align = "all"

win:center()

ui.run(win):wait()