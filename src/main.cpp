#include <CL/cl.h>

#include <iostream>
#include <vector>
#include <string>
#include <string_view>

using namespace std;

int main(int argc, char* argv[]) {
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

  return 0;
}
