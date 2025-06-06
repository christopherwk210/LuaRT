local ui = require "ui"

local Tab = Object(ui.Tab)

local function items_style(panel, obj, property)
    local widget = ui.Combobox(panel, false, {"text", "icons"}, 100, panel.pos-3)
    widget.onSelect = function (self, item)
        tracker.widget.style = item.text
    end
    widget.update = function(tracked)
        widget.selected = widget.items[tracked.style]
    end
    return widget
end

return inspector:register(Tab, {style = items_style}, { count = true })