import math

def encodeNormals(x: int, y: int, z: int):
    return ((z & 0b11) << 4) | ((y & 0b11) << 2) | (x & 0b11)

def decodeNormals(normal: int):
    z = math.floor(normal / 16)
    zmul = z * 16
    normalyx = normal - zmul
    y = math.floor(normalyx / 4)
    ymul = y * 4
    x = normalyx - ymul
    return x, y, z

print(1 << 2)
x = 2
y = 2
z = 2
print(x, y, z)
print(encodeNormals(x, y, z))
print(decodeNormals(encodeNormals(x, y, z)))
