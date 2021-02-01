local quadtree = require("quadtree")

local qt = quadtree.new({
    w = 100, h = 100,
    max_num = 2, max_depth = 8,
})

qt:insert(1, 1, 1, 2, 2)
