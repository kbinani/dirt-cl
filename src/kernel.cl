#if defined(__OPENCL_C_VERSION__ )
typedef long i64;
typedef int i32;
#define MULTIPLIER (0x5DEECE66DL)
#define ADDEND (0xBL)
#define MASK ((1L << 48) - 1L)
#define LONG(v) (v##L)
#else
typedef int64_t i64;
typedef int32_t i32;
#define MULTIPLIER (0x5DEECE66DLL)
#define ADDEND (0xBLL)
#define MASK ((1LL << 48) - 1LL)
#define LONG(v) (v##LL)
#endif

static i64 Rand(i64 seed) {
  return (seed ^ MULTIPLIER) & MASK;
}

static i32 Next(i64 *state, i32 bits) {
  *state = (*state * MULTIPLIER + ADDEND) & MASK;
  return (i32)(*state >> (48 - bits));
}

static i32 NextInt(i64 *state) {
  return Next(state, 32);
}

static i32 NextIntBounded(i64* state, int n) {
  if ((n & -n) == n) {  // i.e., n is a power of 2
    return (i32)((n * (i64)Next(state, 31)) >> 31);
  }

  i32 bits;
  i32 val;
  do {
    bits = Next(state, 31);
    val = bits % n;
  } while (bits - val + (n - 1) < 0);
  return val;
}

static i64 NextLong(i64 *state) {
  i64 ret = ((i64)Next(state, 32)) << 32;
  ret += Next(state, 32);
  return ret;
}

static i64 GetPositionSeed(i32 x, i32 y, i32 z) {
  i64 i = (i64)(x * LONG(3129871)) ^ (i64)z * LONG(116129781) ^ (i64)y;
  i = i * i * LONG(42317861) + i * LONG(11);
  return i >> 16;
}

static i32 GetRandomItemIndex(i32 totalWeight, i32 weight) {
  for (i32 i = 0; i < totalWeight; i++) {
    i32 t = i;
    weight -= 1;
    if (weight < 0) {
      return t;
    }
  }
  return -1;
}

static i32 Abs(i32 v) {
  if (v < 0) {
    return -v;
  } else {
    return v;
  }
}

static i32 DirtRotation(i32 x, i32 y, i32 z, i32 dataVersion) {
  i64 seed = GetPositionSeed(x, y, z);
  i64 state = Rand(seed);
  i32 numFacingTypes = 4;
  if (dataVersion >= 4063) {
    // 24w36a
    return NextIntBounded(&state, numFacingTypes);
  } else {
    // 24w35a
    i32 weight = Abs((i32)NextLong(&state)) % numFacingTypes;
    return GetRandomItemIndex(numFacingTypes, weight);
  }
}

#if defined(__OPENCL_C_VERSION__)
__kernel void run(__global i32 *rot, i32 x, i32 y, i32 z, i32 dataVersion) {
  *rot = DirtRotation(x, y, z, dataVersion);
}
#endif
