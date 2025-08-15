# dirt-cl

A tool that uses OpenCL to identify block positions from multiple Minecraft dirt block texture orientations.

## Features

- Search and predict dirt block texture orientation patterns
- High-speed computation with GPU parallel processing
- Search with specified coordinate ranges and facing directions

## Build

```sh
cmake -B ./build
cmake --build ./build
```

## Usage

```sh
./dirt-cl -i <src/kernel.cl> -f <facing> -x <minX> -X <maxX> -y <minY> -Y <maxY> -z <minZ> -Z <maxZ> -r <rotations>
```

### Options

- `-i`: Path to src/kernel.cl file
- `-f`: Facing direction (`north`, `east`, `south`, `west`)
- `-x`, `-X`: X coordinate range (min, max)
- `-y`, `-Y`: Y coordinate range (min, max)
- `-z`, `-Z`: Z coordinate range (min, max)
- `-r`: Rotation values list (comma-separated, values 0-3)

```
    rotation = 0   rotation = 1   rotation = 2   rotation = 3
    _____________  _____________  _____________  _____________
    |         ==|  |           |  |           |  | I         |
    |       ==  |  |           |  |           |  |  I        |
    |           |  |           |  |           |  |           |
    |           |  |        I  |  |  ==       |  |           |
    |___________|  |_________I_|  |==_________|  |___________|
```

### Example

```sh
./dirt-cl -i src/kernel.cl -f north -x 0 -X 100 -y 60 -Y 70 -z 0 -Z 100 -r 0,1,2,3
```
