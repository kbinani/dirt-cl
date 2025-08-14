#include <CL/cl.hpp>
#include <stdio.h>

#include <iostream>
#include <vector>
#include <string>

#include "./kernel.cl"

using namespace std;

int main(int argc, char* argv[]) {
  string cl;
  for (int i = 1; i < argc; i++) {
    string v = argv[i];
    if (v == "-i") {
      i++;
      if (i >= argc) {
        return 1;
      }
      cl = string(argv[i]);
    }
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
      int read = fread(buffer, 1, sizeof(buffer), file);
      if (read < 0) {
        fclose(file);
        return 1;
      }
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
  if (program.build() != CL_SUCCESS) {
    string log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
    cout << log << endl;
    return 1;
  }
  cl::Kernel kernel(program, "run");

  cl::Buffer mem(ctx, CL_MEM_READ_WRITE, sizeof(int), nullptr);
  int x = 0;
  int y = 62;
  int z = 0;
  int dataVersion = 4063;
  kernel.setArg<cl::Buffer>(0, mem);
  kernel.setArg<int>(1, x);
  kernel.setArg<int>(2, y);
  kernel.setArg<int>(3, z);
  kernel.setArg<int>(4, dataVersion);

  queue.enqueueNDRangeKernel(kernel, cl::NDRange(0), cl::NDRange(1));

  vector<int> buffer(1);
  queue.enqueueReadBuffer(mem, CL_TRUE, 0, buffer.size(), buffer.data());
  std::cout << "[" << x << ", " << y << ", " << z << "]: " << buffer[0] << endl;
  std::cout << "expected: " << DirtRotation(x, y, z, dataVersion) << endl;

  queue.flush();
  queue.finish();

  return 0;
}
