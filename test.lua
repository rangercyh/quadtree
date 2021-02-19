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

local t = qt:query(0, 0, 3, 4)
for k, v in pairs(t) do
    print(v)
end
qt:cleanup()
