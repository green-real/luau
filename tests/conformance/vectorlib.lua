-- This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
print('testing vectorlib')

-- constructor
local v1 = vector(2, 10, 11)
local v2 = vector(4, 5, 6)
local zero = vector.zero
local one = vector.one
local front = vector(0, 0, 1)
local back = vector(0, 0, -1)
local left = vector(-1, 0, 0)
local right = vector(1, 0, 0)
local up = vector(0, 1, 0)
local down = vector(0, -1, 0)

-- Note: since vectors are already rigorously tested by vector.lua, this file only tests the vector library functions
-- after the assertion below, we can assume that the constructors work correctly
assert(type(v1) == 'vector')

-- test all functions, then test all methods

-- vector.angle
assert(math.abs(vector.angle(front, back) - math.pi) < 0.0000001)
assert(math.abs(vector.angle(left, right) - math.pi) < 0.0000001)
assert(math.abs(vector.angle(up, down) - math.pi) < 0.0000001)
assert(vector.angle(zero, one) ~= vector.angle(zero, one))

-- vector.ceil
assert(vector.ceil(vector(1.1, 1.9, 1.5)) == vector(2, 2, 2))
assert(vector.ceil(vector(-1.1, -1.9, -1.5)) == vector(-1, -1, -1))

-- vector.cross
assert(vector.cross(front, right) == up)
assert(vector.cross(right, front) == down)
assert(vector.cross(front, left) == down)
assert(vector.cross(left, front) == up)
assert(vector.cross(right, up) == front)
assert(vector.cross(up, right) == back)

-- vector.dot
assert(vector.dot(front, front) == 1)
assert(vector.dot(front, back) == -1)
assert(vector.dot(front, right) == 0)
assert(vector.dot(front, up) == 0)
assert(vector.dot(right, up) == 0)
assert(vector.dot(right, right) == 1)

-- vector.floor
assert(vector.floor(vector(1.1, 1.9, 1.5)) == vector(1, 1, 1))
assert(vector.floor(vector(-1.1, -1.9, -1.5)) == vector(-2, -2, -2))

-- vector.magnitude
assert(vector.magnitude(v1) == 15)
assert(vector.magnitude(vector(0, 0, 0)) == 0)

-- vector.max
assert(vector.max(vector(1, 2, 3), vector(1, 1, 5)) == vector(1, 2, 5))
assert(vector.max(vector(1, 2, 3), vector(1, 3, 2)) == vector(1, 3, 3))

-- vector.min
assert(vector.min(vector(1, 2, 3), vector(1, 1, 5)) == vector(1, 1, 3))
assert(vector.min(vector(1, 2, 3), vector(1, 3, 2)) == vector(1, 2, 2))
assert(vector.min(vector(1, 2, 3), vector(3, 2, 1)) == vector(1, 2, 1))

-- vector.normalized
assert(vector.normalized(vector(2, 10, 11)) == vector(2, 10, 11) / 15)
assert(vector.normalized(zero) == zero)

-- vector methods
assert(v1:angle(v2) == vector.angle(v1, v2))
assert(v1:ceil() == vector.ceil(v1))
assert(v1:cross(v2) == vector.cross(v1, v2))
assert(v1:dot(v2) == vector.dot(v1, v2))
assert(v1:floor() == vector.floor(v1))
assert(v1:magnitude() == vector.magnitude(v1))
assert(v1:max(v2) == vector.max(v1, v2))
assert(v1:min(v2) == vector.min(v1, v2))
assert(v1:normalized() == vector.normalized(v1))

return 'OK'
