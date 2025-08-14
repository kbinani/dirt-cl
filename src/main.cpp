#include <CL/cl.h>
#include <stdio.h>

#include <iostream>
#include <vector>
#include <string>
#include <string_view>

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

  cout << "Platforms:" << endl;
  cl_uint numPlatforms;
  if (clGetPlatformIDs(0, nullptr, &numPlatforms) != 0) {
    return 1;
  }
  if (numPlatforms < 1) {
    return 1;
  }
  vector<cl_platform_id> platforms(numPlatforms);
  if (clGetPlatformIDs(numPlatforms, platforms.data(), nullptr) != 0) {
    return 1;
  }
  for (size_t i = 0; i < platforms.size(); i++) {
    cl_platform_id id = platforms[i];
    size_t size = 0;
    if (clGetPlatformInfo(id, CL_PLATFORM_NAME, 0, nullptr, &size) != 0) {
      return 1;
    }
    vector<char> name(size + 1, '0');
    if (clGetPlatformInfo(id, CL_PLATFORM_NAME, name.size(), name.data(), nullptr) != 0) {
      return 1;
    }
    cout << "  #" << i << " " << name.data() << endl;
  }
  cl_platform_id platform = platforms[0];

  cout << "Devices:" << endl;
  cl_uint numDevices;
  if (clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &numDevices) != 0) {
    return 1;
  }
  if (numDevices < 1) {
    return 1;
  }
  vector<cl_device_id> devices(numDevices);
  if (clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices.data(), nullptr) != 0) {
    return 1;
  }
  for (size_t i = 0; i < devices.size(); i++) {
    cl_device_id id = devices[i];
    size_t size;
    if (clGetDeviceInfo(id, CL_DEVICE_NAME, 0, nullptr, &size) != 0) {
      return 1;
    }
    vector<char> name(size);
    if (clGetDeviceInfo(id, CL_DEVICE_NAME, size, name.data(), nullptr) != 0) {
      return 1;
    }
    cout << "  #" << i << " " << name.data() << endl;
  }
  cl_device_id device = devices[0];

  cl_int err;
  cl_context ctx = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
  if (err != 0) {
    return 1;
  }

  err = 0;
  cl_command_queue queue = clCreateCommandQueue(ctx, device, 0, &err);
  if (err != 0) {
    return 1;
  }

  vector<char const*> list;
  list.push_back(src.c_str());
  vector<size_t> sizes;
  sizes.push_back(src.size());
  err = 0;
  cl_program program = clCreateProgramWithSource(ctx, 1, list.data(), sizes.data(), &err);
  if (err != 0) {
    return 1;
  }

  if (clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr) != 0) {
    return 1;
  }

  cl_int ret = 0;
  cl_kernel kernel = clCreateKernel(program, "run", &ret);
  if (ret != 0) {
    return 1;
  }

  if (clEnqueueTask(queue, kernel, 0, nullptr, nullptr) != 0) {
    return 1;
  }

  clFlush(queue);
  clFinish(queue);
  clReleaseKernel(kernel);
  clReleaseProgram(program);
  clReleaseCommandQueue(queue);
  clReleaseContext(ctx);
  return 0;
}
