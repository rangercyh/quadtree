-- 适合用做静态多对象跟运动的物体边界也好镜头也好的查找
local quadtree = require("quadtree")

local qt = quadtree.new({
    w = 6, h = 6,
    max_num = 2, max_depth = 8,
})

print("i = ", qt:insert(1, 0, 0, 2, 2))
print("i = ", qt:insert(2, 1, 2, 2, 3))
print("i = ", qt:insert(3, 3, 1, 4, 2))
print("i = ", qt:insert(4, 2, 3, 4, 4))
print("i = ", qt:insert(5, 4, 4, 5, 5))
print("i = ", qt:insert(6, 1, 5, 2, 6))
qt:cleanup()

-- move
--[[
qt:remove()
qt:insert()
]]

local t = qt:query(0, 0, 3, 4)
for k, v in pairs(t) do
    print(v)
end
qt:cleanup()

-- local uniform_grid = require("ugrid")
local uniform_grid = require("ugrid2")

local ugrid = uniform_grid.new({
    radius = 10, cw = 10, ch = 10,
    l = 0, t = 0, r = 1000, b = 1000,
})
print('insert idx = ', ugrid:insert(1, 0, 0))
print('insert idx = ', ugrid:insert(2, 501.11, 10.22))
print('insert idx = ', ugrid:insert(3, 100.2, 88))
print('insert idx = ', ugrid:insert(4, 999999, 999999))
ugrid:optimize()
print("in in_bounds = ", ugrid:in_bounds(9999, 999990))
print("in in_bounds = ", ugrid:in_bounds(99, 980))
ugrid:remove(1, 0, 0)
ugrid:remove(3, 100.2, 88)
local t = ugrid:query(0, 0, 0, 200)
print('++++++ query')
for k, v in pairs(t) do
    print(v)
end
ugrid:move(2, 501.11, 10.22, 200, 200)
local t = ugrid:query(0, 0, 0, 300)
print('++++++ query')
for k, v in pairs(t) do
    print(v)
end
ugrid:optimize()
