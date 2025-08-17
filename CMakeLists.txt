cmake_minimum_required(VERSION 3.28)
project(dirt-cl)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (EXISTS "$ENV{CUDA_PATH}")
  list(APPEND dirt_cl_include_directories $ENV{CUDA_PATH}/include)
  list(APPEND dirt_cl_link_directories $ENV{CUDA_PATH}/lib/x64)
endif()
list(APPEND dirt_cl_include_directories ${CMAKE_CURRENT_SOURCE_DIR}/deps/OpenCL-CLHPP/include)

function(resource input output varname)
  file(READ ${CMAKE_CURRENT_SOURCE_DIR}/${input} content)

  set(chunk_size 16000) # workaround for MSVC C2026
  string(LENGTH "${content}" len)
  set(chunks "")
  set(offset 0)
  set(variable_name "${varname}")

  while(offset LESS len)
    math(EXPR remaining "${len}-${offset}")
    if(remaining GREATER chunk_size)
      set(size ${chunk_size})
    else()
      set(size ${remaining})
    endif()
    string(SUBSTRING "${content}" ${offset} ${size} part)
    string(APPEND chunks "R\"(${part})\"\n")
    math(EXPR offset "${offset}+${size}")
  endwhile()

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel.hpp.in
    ${CMAKE_CURRENT_SOURCE_DIR}/${output}
    @ONLY)
endfunction()

resource(src/kernel.cl src/kernel.hpp kernel)

add_executable(dirt-cl src/main.cpp src/kernel.cl src/kernel.hpp src/predicate.hpp)

if (APPLE)
  add_compile_definitions(CL_HPP_TARGET_OPENCL_VERSION=120 CL_HPP_MINIMUM_OPENCL_VERSION=120)
  set(CMAKE_EXE_LINKER_FLAGS "-framework OpenCL ${CMAKE_EXE_LINKER_FLAGS}")
else()
  list(APPEND dirt_cl_link_libraries OpenCL)
endif()

target_link_libraries(dirt-cl ${dirt_cl_link_libraries})
target_include_directories(dirt-cl PRIVATE ${dirt_cl_include_directories})
target_link_directories(dirt-cl PRIVATE ${dirt_cl_link_directories})
