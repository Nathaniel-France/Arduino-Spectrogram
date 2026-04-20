import math

N = 128
SCALE = 32767
numStages = 7

twiddle_r = []
twiddle_j = []

window = []

bitRev = []

for k in range(N // 2):
    angle = 2 * math.pi * k / N
    twiddle_r.append(int(round(math.cos(angle) * SCALE)))
    twiddle_j.append(int(round(-math.sin(angle) * SCALE)))

for i in range(N):
    angleIndex = 2 * math.pi * i / (N - 1)
    cosVal = int(round(math.cos(angleIndex) * SCALE))
    val = (SCALE - cosVal) // 2
    window.append(val)

for i in range(N):
    rev = 0
    x = i
    for _ in range(numStages):
        rev = (rev << 1) | (x & 1)
        x >>= 1
    bitRev.append(rev)
    


print("const int16_t twiddle_r[] PROGMEM = {" + ", ".join(map(str, twiddle_r)) + "};")
print("const int16_t twiddle_j[] PROGMEM = {" + ", ".join(map(str, twiddle_j)) + "};")
print("const q15 window[] PROGMEM = {" + ", ".join(map(str, window)) + "};")
print("const uint8_t bit_reversal[] PROGMEM = {" + ", ".join(map(str, bitRev)) + "};")