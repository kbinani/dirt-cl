# dirt-cl

A tool that uses OpenCL to identify block positions from multiple Minecraft dirt block texture orientations.

## Features

- Search and predict dirt block texture orientation patterns
- High-speed computation with GPU parallel processing
- Search with specified coordinate ranges and facing directions

## Build

```sh
git submodule update --init --recursive
cmake -B ./build
cmake --build ./build
```

## Usage

```sh
./dirt-cl -i <src/kernel.cl> -f <facing> -x <minX> -X <maxX> -y <minY> -Y <maxY> -z <minZ> -Z <maxZ> -r <rotations> -v <dataVersion>
```

### Options

- `-i`: Path to src/kernel.cl file (required)
- `-f`: Facing direction (optional, `north`, `east`, `south`, `west`)
- `-x`, `-X`: X coordinate range (required, set minimum by -x, maximum by -X)
- `-y`, `-Y`: Y coordinate range (required, set minimum by -y, maximum by -Y)
- `-z`, `-Z`: Z coordinate range (required, set minimum by -z, maximum by -Z)
- `-r`: Rotation values list (required, comma-separated, values 0-3)
- `-v`: Data version specifying the client version (optional, default INT_MAX, means latest version)

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
./dirt-cl -i src/kernel.cl -v 4440 -f north -x 0 -X 100 -y 60 -Y 70 -z 0 -Z 100 -r 0,1,2,3
```
