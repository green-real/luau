-- This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
print('testing vectorlib')

-- constructor
local v1 = vector.new(1, 2, 3)
local v2 = vector.new(4, 5, 6)

-- Note: since vectors are already rigorously tested by vector.lua, this file only tests the vector library functions
-- after the assertion below, we can assume that the constructors work correctly
assert(type(v1) == 'vector')

-- cross, dot
assert(vector.cross(v1, v2) == vector.new(-3, 6, -3))
assert(vector.dot(v1, v2) == 32)

-- magnitude, normalize
local v3 = vector.new(2, 10, 11)
assert(vector.magnitude(v3) == 15)
assert(vector.normalize(v3) == (v3 / 15))

-- zero vector tests, nan test
local vzero = vector.new(0, 0, 0)
assert(vector.magnitude(vzero) == 0)
assert(vector.normalize(vzero) ~= vector.normalize(vzero))

-- optional constructor arguments
assert(vector.new().x == 0)
assert(vector.new().y == 0)
assert(vector.new().z == 0)

return 'OK'
