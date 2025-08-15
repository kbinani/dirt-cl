#if defined(__APPLE__)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include <CL/opencl.hpp>
#include <stdio.h>

#include <iostream>
#include <vector>
#include <string>
#include <optional>

#include "./kernel.cl"

using namespace std;

int main(int argc, char* argv[]) {
  string cl;
  Facing facing = FACING_UNKNOWN;
  optional<i32> minX;
  optional<i32> maxX;
  optional<i32> minY;
  optional<i32> maxY;
  optional<i32> minZ;
  optional<i32> maxZ;
  int dataVersion = 4063;
  vector<i32> simple;
  for (int i = 1; i < argc; i++) {
    string v = argv[i];
    if (!v.starts_with("-")) {
      cerr << "argument parse error" << endl;
      return 1;
    }
    i++;
    if (i >= argc) {
      cerr << "too few arguments" << endl;
      return 1;
    }
    if (v == "-i") {
      cl = string(argv[i]);
    } else if (v == "-f") {
      v = argv[i];
      if (v == "north") {
        facing = FACING_NORTH;
      } else if (v == "east") {
        facing = FACING_EAST;
      } else if (v == "south") {
        facing = FACING_SOUTH;
      } else if (v == "west") {
        facing = FACING_WEST;
      } else {
        cerr << "unknown facing: " << v << endl;
        return 1;
      }
    } else if (v == "-x") {
      i32 t;
      if (sscanf(argv[i], "%d", &t) != 1) {
        cerr << "cannot parse " << v << endl;
        return 1;
      }
      minX = t;
    } else if (v == "-X") {
      i32 t;
      if (sscanf(argv[i], "%d", &t) != 1) {
        cerr << "cannot parse " << v << endl;
        return 1;
      }
      maxX = t;
    } else if (v == "-y") {
      i32 t;
      if (sscanf(argv[i], "%d", &t) != 1) {
        cerr << "cannot parse " << v << endl;
        return 1;
      }
      minY = t;
    } else if (v == "-Y") {
      i32 t;
      if (sscanf(argv[i], "%d", &t) != 1) {
        cerr << "cannot parse " << v << endl;
        return 1;
      }
      maxY = t;
    } else if (v == "-z") {
      i32 t;
      if (sscanf(argv[i], "%d", &t) != 1) {
        cerr << "cannot parse " << v << endl;
        return 1;
      }
      minZ = t;
    } else if (v == "-Z") {
      i32 t;
      if (sscanf(argv[i], "%d", &t) != 1) {
        cerr << "cannot parse " << v << endl;
        return 1;
      }
      maxZ = t;
    } else if (v == "-r") {
      v = string(argv[i]);
      size_t offset = 0;
      while (true) {
        auto found = v.find(',', offset);
        if (found == string::npos) {
          break;
        }
        auto sub = v.substr(offset, found - offset);
        i32 t;
        if (sscanf(sub.c_str(), "%d", &t) != 1) {
          cerr << "invalid value for -r option: " << argv[i] << endl;
          return 1;
        }
        if (t < 0 || 3 < t) {
          cerr << "invalid range for -r option: " << argv[i] << " (must be 0 <= value <= 3)" << endl;
          return 1;
        }
        simple.push_back(t);
        offset = found + 1;
      }
    }
  }
  if (!minX || !maxX || !minY || !maxY || !minZ || !maxZ) {
    return 1;
  }
  if (*minX > *maxX || *minY > *maxY || *minZ > *maxZ) {
    return 1;
  }
  if (cl.empty()) {
    return 1;
  }
  string src;
  {
    FILE* file = fopen(cl.c_str(), "rb");
    if (!file) {
      return 1;
    }
    while (!feof(file)) {
      char buffer[512];
      size_t read = fread(buffer, 1, sizeof(buffer), file);
      src.append(buffer, read);
    }
    fclose(file);
  }
  cout << "src:" << endl;
  cout << "  path: " << cl << endl;
  cout << "  size: " << src.size() << endl;

  cl::Platform platform = cl::Platform::getDefault();
  cout << "platform:" << endl;
  cout << "  name: " << platform.getInfo<CL_PLATFORM_NAME>() << endl;
  cout << "  version: " << platform.getInfo<CL_PLATFORM_VERSION>() << endl;
  vector<cl::Device> devices;
  platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

  if (devices.empty()) {
    return 1;
  }
  cl::Device device = devices[0];
  cout << "device: " << endl;
  cout << "  name: " << device.getInfo<CL_DEVICE_NAME>() << endl;
  cout << "  version: " << device.getInfo<CL_DEVICE_VERSION>() << endl;
  cl::Context ctx(device);
  cl::CommandQueue queue(ctx, device);
  cl::Program program(ctx, src);
  if (program.build("-cl-std=CL2.0") != CL_SUCCESS) {
    string log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
    cout << log << endl;
    return 1;
  }
  cl::Kernel kernel(program, "run");

  cl::Buffer xPredicate(ctx, CL_MEM_READ_WRITE, sizeof(i32) * simple.size(), nullptr);
  cl::Buffer yPredicate(ctx, CL_MEM_READ_WRITE, sizeof(i32) * simple.size(), nullptr);
  cl::Buffer zPredicate(ctx, CL_MEM_READ_WRITE, sizeof(i32) * simple.size(), nullptr);
  cl::Buffer rotationPredicate(ctx, CL_MEM_READ_WRITE, sizeof(i32) * simple.size(), nullptr);
  {
    vector<i32> y;
    vector<i32> r;
    for (i32 i = 0; i < (i32)simple.size(); i++) {
      y.push_back(i);
      r.push_back(simple[i]);
    }
    if (queue.enqueueWriteBuffer(yPredicate, CL_TRUE, 0, sizeof(i32) * simple.size(), y.data()) != CL_SUCCESS) {
      return 1;
    }
    if (queue.enqueueWriteBuffer(rotationPredicate, CL_TRUE, 0, sizeof(i32) * simple.size(), r.data()) != CL_SUCCESS) {
      return 1;
    }
  }
  if (queue.enqueueFillBuffer<i32>(xPredicate, 0, 0, sizeof(i32) * simple.size()) != CL_SUCCESS) {
    return 1;
  }
  if (queue.enqueueFillBuffer<i32>(zPredicate, 0, 0, sizeof(i32) * simple.size()) != CL_SUCCESS) {
    return 1;
  }
  cl::Buffer result(ctx, CL_MEM_READ_WRITE, sizeof(i32) * 4, nullptr);
  cl::Buffer count(ctx, CL_MEM_READ_WRITE, sizeof(i32), nullptr);
  if (queue.enqueueFillBuffer<i32>(count, 0, 0, sizeof(i32)) != CL_SUCCESS) {
    return 1;
  }

  if (kernel.setArg<cl::Buffer>(0, xPredicate) != CL_SUCCESS) {
    return 1;
  }
  if (kernel.setArg<cl::Buffer>(1, yPredicate) != CL_SUCCESS) {
    return 1;
  }
  if (kernel.setArg<cl::Buffer>(2, zPredicate) != CL_SUCCESS) {
    return 1;
  }
  if (kernel.setArg<cl::Buffer>(3, rotationPredicate) != CL_SUCCESS) {
    return 1;
  }
  if (kernel.setArg<u32>(4, (u32)simple.size()) != CL_SUCCESS) {
    return 1;
  }
  if (kernel.setArg<i32>(5, facing) != CL_SUCCESS) {
    return 1;
  }
  if (kernel.setArg<i32>(6, dataVersion) != CL_SUCCESS) {
    return 1;
  }
  if (kernel.setArg<i32>(7, *minX) != CL_SUCCESS) {
    return 1;
  }
  if (kernel.setArg<i32>(8, *maxX) != CL_SUCCESS) {
    return 1;
  }
  if (kernel.setArg<i32>(9, *minY) != CL_SUCCESS) {
    return 1;
  }
  if (kernel.setArg<i32>(10, *maxY) != CL_SUCCESS) {
    return 1;
  }
  if (kernel.setArg<i32>(11, *minZ) != CL_SUCCESS) {
    return 1;
  }
  if (kernel.setArg<i32>(12, *maxZ) != CL_SUCCESS) {
    return 1;
  }
  if (kernel.setArg<cl::Buffer>(13, result) != CL_SUCCESS) {
    return 1;
  }
  if (kernel.setArg<cl::Buffer>(14, count) != CL_SUCCESS) {
    return 1;
  }

  u32 dx = *maxX - *minX + 1;
  u32 dy = *maxY - *minY + 1;
  u32 dz = *maxZ - *minZ + 1;
  if (queue.enqueueNDRangeKernel(kernel, cl::NDRange(0, 0, 0), cl::NDRange(dx, dy, dz), cl::NullRange) != CL_SUCCESS) {
    return 1;
  }

  vector<i32> readResult(4);
  vector<u32> readCount(1);
  if (queue.enqueueReadBuffer(result, CL_TRUE, 0, sizeof(i32) * readResult.size(), readResult.data()) != CL_SUCCESS) {
    return 1;
  }
  if (queue.enqueueReadBuffer(count, CL_TRUE, 0, sizeof(u32) * readCount.size(), readCount.data()) != CL_SUCCESS) {
    return 1;
  }

  if (readCount[0] == 1) {
    cout << "x=" << readResult[0] << ", y=" << readResult[1] << ", z=" << readResult[2];
    if (facing < 0) {
      cout << ", facing=";
      switch (readResult[3]) {
      case 1:
        cout << "north";
        break;
      case 2:
        cout << "east";
        break;
      case 3:
        cout << "south";
        break;
      case 4:
        cout << "west";
        break;
      default:
        cerr << "unknown facing: " << readResult[3] << endl;
      }
    }
    cout << endl;
  } else if (readCount[0] == 0) {
    cout << "No coordinates matching the conditions were found" << endl;
  } else {
    cout << "Multiple coordinates matching the conditions were found" << endl;
  }

  queue.flush();
  queue.finish();

  return 0;
}
