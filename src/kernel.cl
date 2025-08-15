#if defined(__OPENCL_C_VERSION__)
typedef long i64;
typedef int i32;
typedef uint u32;
#define LONG(v) (v##L)
#else
using i64 = int64_t;
using i32 = int32_t;
using u32 = uint32_t;
#define __global
#define __kernel
#define LONG(v) (v##LL)
#endif

#define MULTIPLIER (LONG(0x5DEECE66D))
#define ADDEND (LONG(0xB))
#define MASK ((LONG(1) << 48) - LONG(1))

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
  if ((n & -n) == n) {
    // n is a power of 2
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

static i64 GetPositionSeed_v0(i32 x, i32 y, i32 z) {
  i64 i = (i64)(x * 3129871) ^ (i64)z * LONG(116129781) ^ (i64)y;
  i = i * i * LONG(42317861) + i * LONG(11);
  return i;
}

static i64 GetPositionSeed_v1(i32 x, i32 y, i32 z) {
  i64 i = (i64)(x * 3129871) ^ (i64)z * LONG(116129781) ^ (i64)y;
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
  i32 const numFacingTypes = 4;
  if (dataVersion >= 4063 /* 24w36a */) {
    // 1.21.2
    // 24w36a
    i64 seed = GetPositionSeed_v1(x, y, z);
    i64 state = Rand(seed);
    return NextIntBounded(&state, numFacingTypes);
  } else if (dataVersion >= 1459 /* 18w01a */) {
    // 24w35a
    // 1.21.1
    // 1.13
    // 18w11a
    // 18w07a
    // 18w03a
    // 18w01a
    i64 seed = GetPositionSeed_v1(x, y, z);
    i64 state = Rand(seed);
    i32 weight = Abs((i32)NextLong(&state)) % numFacingTypes;
    return GetRandomItemIndex(numFacingTypes, weight);
  } else if (dataVersion >= 0) {
    // TODO: Between 17w47a and 17w50a, the texture rotation method changed drastically and differs in each version, so further investigation is needed.

    // 17w46a
    // 1.12.2
    // 1.12
    // 1.9
    // 1.8.9
    // 14w31a
    // 14w30a
    // 14w29a
    // 14w28a
    // 14w27b
    i64 s = GetPositionSeed_v0(x, y, z);
    i32 v = ((i32)s) >> 16;
    i32 weight = Abs(v) % numFacingTypes;
    return GetRandomItemIndex(numFacingTypes, weight);
  } else {
    // 14w27a
    // 1.7.10
    return 0;
  }
}

enum Facing {
  FACING_UNKNOWN = -1,
  FACING_NORTH = 1,
  FACING_EAST = 2,
  FACING_SOUTH = 3,
  FACING_WEST = 4,
};

static i32 SatisfiesPredicates(__global i32 const* xPredicate, __global i32 const* yPredicate, __global i32 const* zPredicate, __global i32 const* rotationPredicate, u32 numPredicates,
                               i32 facing, i32 x, i32 y, i32 z, i32 dataVersion) {
  if (facing == FACING_UNKNOWN) {
    for (i32 offset = 0; offset < 4; offset++) {
      bool ok = true;
      for (u32 i = 0; i < numPredicates; i++) {
        i32 bx = x + xPredicate[i];
        i32 by = y + yPredicate[i];
        i32 bz = z + zPredicate[i];
        i32 actual = DirtRotation(bx, by, bz, dataVersion);
        i32 expected = rotationPredicate[i] + offset;
        if (expected > 3) {
          expected -= 4;
        }
        if (actual != expected) {
          ok = false;
          break;
        }
      }
      if (!ok) {
        continue;
      }
      switch (offset) {
      case 0:
        return FACING_NORTH;
      case 1:
        return FACING_EAST;
      case 2:
        return FACING_SOUTH;
      case 3:
        return FACING_WEST;
      default:
        return -1;
      }
    }
    return -1;
  } else {
    i32 offset = 0;
    if (facing == FACING_EAST) {
      offset = 1;
    } else if (facing == FACING_SOUTH) {
      offset = 2;
    } else if (facing == FACING_WEST) {
      offset = 3;
    }
    for (u32 i = 0; i < numPredicates; i++) {
      i32 bx = x + xPredicate[i];
      i32 by = y + yPredicate[i];
      i32 bz = z + zPredicate[i];
      i32 actual = DirtRotation(bx, by, bz, dataVersion);
      i32 expected = rotationPredicate[i] + offset;
      if (expected > 3) {
        expected -= 4;
      }
      if (actual != expected) {
        return -1;
      }
    }
    return facing;
  }
}

#if defined(__OPENCL_C_VERSION__)
__kernel void run(__global i32 const* xPredicate, __global i32 const* yPredicate, __global i32 const* zPredicate, __global i32 const* rotationPredicate, u32 numPredicates,
                  i32 facing, i32 dataVersion,
                  i32 minX, i32 maxX, i32 minY, i32 maxY, i32 minZ, i32 maxZ,
                  __global i32 *result, __global u32 *count) {
  i32 x = minX + get_global_id(0);
  i32 y = minY + get_global_id(1);
  i32 z = minZ + get_global_id(2);
  i32 f = SatisfiesPredicates(xPredicate, yPredicate, zPredicate, rotationPredicate, numPredicates, facing, x, y, z, dataVersion);
  if (f > 0) {
    u32 c = atomic_inc(count) + 1;
    if (c == 1) {
      result[0] = x;
      result[1] = y;
      result[2] = z;
      result[3] = f;
    }
  }
}
#endif
