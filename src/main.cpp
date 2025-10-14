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
#include <chrono>
#include <string_view>

#include "./kernel.hpp"
#include "./kernel.cl"
#include "./predicate.hpp"

template<class... T>
static bool SetArgsFrom(cl::Kernel& kernel, cl_uint start, T const&... args) {
  cl_uint i = start;
  return ((kernel.setArg(i++, args) == CL_SUCCESS) && ...);
}

template<class... T>
static bool SetArgs(cl::Kernel& kernel, T const&... args) {
  return SetArgsFrom(kernel, 0, args...);
}

int main(int argc, char* argv[]) {
  using namespace std;
  using namespace dirt;

  optional<string> kernelFile;
  Facing facing = FACING_UNKNOWN;
  optional<int32_t> minX;
  optional<int32_t> maxX;
  optional<int32_t> minY;
  optional<int32_t> maxY;
  optional<int32_t> minZ;
  optional<int32_t> maxZ;
  int dataVersion = INT_MAX;
  vector<Predicate> predicates;
  size_t platformIndex = 0;
  size_t deviceIndex = 0;
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
    if (v == "--kernel") {
      kernelFile = string(argv[i]);
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
      int32_t t;
      if (sscanf(argv[i], "%d", &t) != 1) {
        cerr << "cannot parse " << v << endl;
        return 1;
      }
      minX = t;
    } else if (v == "-X") {
      int32_t t;
      if (sscanf(argv[i], "%d", &t) != 1) {
        cerr << "cannot parse " << v << endl;
        return 1;
      }
      maxX = t;
    } else if (v == "-y") {
      int32_t t;
      if (sscanf(argv[i], "%d", &t) != 1) {
        cerr << "cannot parse " << v << endl;
        return 1;
      }
      minY = t;
    } else if (v == "-Y") {
      int32_t t;
      if (sscanf(argv[i], "%d", &t) != 1) {
        cerr << "cannot parse " << v << endl;
        return 1;
      }
      maxY = t;
    } else if (v == "-z") {
      int32_t t;
      if (sscanf(argv[i], "%d", &t) != 1) {
        cerr << "cannot parse " << v << endl;
        return 1;
      }
      minZ = t;
    } else if (v == "-Z") {
      int32_t t;
      if (sscanf(argv[i], "%d", &t) != 1) {
        cerr << "cannot parse " << v << endl;
        return 1;
      }
      maxZ = t;
    } else if (v == "-r") {
      if (!predicates.empty()) {
        cerr << "-p option and -r option are specified at the same time" << endl;
        return 1;
      }
      v = string(argv[i]);
      size_t offset = 0;
      int32_t y = 0;
      while (true) {
        auto found = v.find(',', offset);
        if (found == string::npos) {
          break;
        }
        auto sub = v.substr(offset, found - offset);
        int32_t t;
        if (sscanf(sub.c_str(), "%d", &t) != 1) {
          cerr << "invalid value for -r option: " << argv[i] << endl;
          return 1;
        }
        if (t < 0 || 3 < t) {
          cerr << "invalid range for -r option: " << argv[i] << " (must be 0 <= value <= 3)" << endl;
          return 1;
        }
        Predicate p;
        p.dx = 0;
        p.dy = y;
        p.dz = 0;
        p.r = t;
        predicates.push_back(p);
        y++;
        offset = found + 1;
      }
    } else if (v == "-v") {
      if (sscanf(argv[i], "%d", &dataVersion) != 1) {
        cerr << "cannot parse -v option: " << argv[i] << endl;
        return 1;
      }
    } else if (v == "--platform") {
      int32_t t;
      if (sscanf(argv[i], "%d", &t) != 1) {
        cerr << "cannot parse " << v << " option: " << argv[i] << endl;
        return 1;
      }
      if (t < 0) {
        cerr << "invalid value for " << v << " option: " << argv[i] << endl;
        return 1;
      }
      platformIndex = t;
    } else if (v == "--device") {
      int32_t t;
      if (sscanf(argv[i], "%d", &t) != 1) {
        cerr << "cannot parse " << v << " option: " << argv[i] << endl;
        return 1;
      }
      if (t < 0) {
        cerr << "invalid value for " << v << " option: " << argv[i] << endl;
        return 1;
      }
      deviceIndex = t;
    } else if (v == "-p") {
      if (!predicates.empty()) {
        cerr << "-p option and -r option are specified at the same time" << endl;
        return 1;
      }
      v = argv[i];
      while (!v.empty()) {
        auto begin = v.find('{');
        if (begin == string::npos) {
          break;
        }
        auto end = v.find('}', begin + 1);
        if (end == string::npos) {
          cerr << "invalid json format" << endl;
          return 1;
        }
        auto json = v.substr(begin, end - begin + 1);
        auto p = Predicate::FromJSON(json);
        if (!p) {
          cerr << "failed to parse as json: " << json << endl;
          return 1;
        }
        predicates.push_back(*p);
        v = v.substr(end + 1);
        if (!v.empty()) {
          if (v.starts_with(',')) {
            v = v.substr(1);
          } else {
            cerr << "failed to parse as json" << endl;
            return 1;
          }
        }
      }
      if (!v.empty()) {
        cerr << "failed to parse as json: " << argv[i] << endl;
        return 1;
      }
    } else {
      cerr << "unknown option: " << v << endl;
      return 1;
    }
  }
  if (!minX || !maxX || !minY || !maxY || !minZ || !maxZ) {
    return 1;
  }
  if (*minX > *maxX || *minY > *maxY || *minZ > *maxZ) {
    return 1;
  }
  string src;
  if (kernelFile) {
    FILE* file = fopen(kernelFile->c_str(), "rb");
    if (!file) {
      return 1;
    }
    while (!feof(file)) {
      char buffer[512];
      size_t read = fread(buffer, 1, sizeof(buffer), file);
      src.append(buffer, read);
    }
    fclose(file);
    cout << "kernel:" << endl;
    cout << "  type: file" << endl;
    cout << "  path: " << *kernelFile << endl;
  } else {
    src = dirt::res::kernel;
    cout << "kernel:" << endl;
    cout << "  type: embedded" << endl;
  }
  cout << "  size: " << src.size() << " bytes" << endl;
  src = "#define DATA_VERSION (" + std::to_string(dataVersion) + ")\n" + src;

  vector<cl::Platform> platforms;
  if (cl::Platform::get(&platforms) != CL_SUCCESS) {
    return 1;
  }
  if (platforms.empty()) {
    cerr << "no platform found" << endl;
    return 1;
  }
  cout << "platforms:" << endl;
  for (size_t i = 0; i < platforms.size(); i++) {
    cout << "  #" << i << ": " << platforms[i].getInfo<CL_PLATFORM_NAME>() << endl;
  }

  cl::Platform platform;
  if (platformIndex >= platforms.size()) {
    cerr << "--platform option is out of range" << endl;
    return 1;
  }
  platform = platforms[platformIndex];
  cout << "selected platform:" << endl;
  cout << "  name: " << platform.getInfo<CL_PLATFORM_NAME>() << endl;
  cout << "  version: " << platform.getInfo<CL_PLATFORM_VERSION>() << endl;
  vector<cl::Device> devices;
  if (platform.getDevices(CL_DEVICE_TYPE_GPU, &devices) != CL_SUCCESS) {
    return 1;
  }
  if (devices.empty()) {
    cerr << "no device found" << endl;
    return 1;
  }
  cout << "devices:" << endl;
  for (size_t i = 0; i < devices.size(); i++) {
    cout << "  #" << i << ": " << devices[i].getInfo<CL_DEVICE_NAME>() << endl;
  }

  cl::Device device;
  if (deviceIndex >= devices.size()) {
    cerr << "--device option is out of range" << endl;
    return 1;
  }
  device = devices[deviceIndex];
  cout << "selected device: " << endl;
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

  cl::Buffer xPredicate(ctx, CL_MEM_READ_WRITE, sizeof(int32_t) * predicates.size(), nullptr);
  cl::Buffer yPredicate(ctx, CL_MEM_READ_WRITE, sizeof(int32_t) * predicates.size(), nullptr);
  cl::Buffer zPredicate(ctx, CL_MEM_READ_WRITE, sizeof(int32_t) * predicates.size(), nullptr);
  cl::Buffer rotationPredicate(ctx, CL_MEM_READ_WRITE, sizeof(int32_t) * predicates.size(), nullptr);
  {
    vector<int32_t> x;
    vector<int32_t> y;
    vector<int32_t> z;
    vector<int32_t> r;
    for (auto const& p : predicates) {
      x.push_back(p.dx);
      y.push_back(p.dy);
      z.push_back(p.dz);
      r.push_back(p.r);
    }
    if (queue.enqueueWriteBuffer(xPredicate, CL_TRUE, 0, sizeof(int32_t) * predicates.size(), x.data()) != CL_SUCCESS) {
      return 1;
    }
    if (queue.enqueueWriteBuffer(yPredicate, CL_TRUE, 0, sizeof(int32_t) * predicates.size(), y.data()) != CL_SUCCESS) {
      return 1;
    }
    if (queue.enqueueWriteBuffer(zPredicate, CL_TRUE, 0, sizeof(int32_t) * predicates.size(), z.data()) != CL_SUCCESS) {
      return 1;
    }
    if (queue.enqueueWriteBuffer(rotationPredicate, CL_TRUE, 0, sizeof(int32_t) * predicates.size(), r.data()) != CL_SUCCESS) {
      return 1;
    }
  }
  cl::Buffer result(ctx, CL_MEM_READ_WRITE, sizeof(int32_t) * 4, nullptr);
  cl::Buffer count(ctx, CL_MEM_READ_WRITE, sizeof(uint32_t), nullptr);
  if (queue.enqueueFillBuffer<int32_t>(count, 0, 0, sizeof(int32_t)) != CL_SUCCESS) {
    return 1;
  }
  if (!SetArgs(kernel, xPredicate, yPredicate, zPredicate, rotationPredicate, (uint32_t)predicates.size(), facing, *minX, *minY, *minZ, result, count)) {
    return 1;
  }

  auto start = chrono::high_resolution_clock::now();
  uint32_t dx = *maxX - *minX + 1;
  uint32_t dy = *maxY - *minY + 1;
  uint32_t dz = *maxZ - *minZ + 1;
  if (queue.enqueueNDRangeKernel(kernel, cl::NDRange(0, 0, 0), cl::NDRange(dx, dy, dz), cl::NullRange) != CL_SUCCESS) {
    return 1;
  }

  vector<int32_t> readResult(4);
  uint32_t readCount;
  if (queue.enqueueReadBuffer(result, CL_TRUE, 0, sizeof(int32_t) * readResult.size(), readResult.data()) != CL_SUCCESS) {
    return 1;
  }
  if (queue.enqueueReadBuffer(count, CL_TRUE, 0, sizeof(uint32_t), &readCount) != CL_SUCCESS) {
    return 1;
  }
  queue.flush();
  queue.finish();

  auto elapsed = chrono::high_resolution_clock::now() - start;
  double seconds = chrono::duration_cast<chrono::milliseconds>(elapsed).count() / 1000.0;

  cout << "result:" << endl;
  cout << "  " << seconds << " seconds elapsed" << endl;
  if (readCount == 1) {
    cout << "  x=" << readResult[0] << ", y=" << readResult[1] << ", z=" << readResult[2];
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
  } else if (readCount == 0) {
    cout << "  No coordinates matching the conditions were found" << endl;
  } else {
    cout << "  Multiple coordinates matching the conditions were found" << endl;
  }

  return 0;
}
