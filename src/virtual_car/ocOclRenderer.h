#pragma once

#ifndef CL_TARGET_OPENCL_VERSION
#define CL_TARGET_OPENCL_VERSION 120
#endif

#include "../common/ocAssert.h"
#include "../common/ocLogger.h"
#include "../common/ocMat.h"
#include "../common/ocPose.h"
#include "../common/ocVec.h"
#include "ocRenderCamera.h"
#include "ocSimulationWorld.h"

#include <CL/cl.h>
#include <fstream>
#include <string>

#include <cerrno>
#include <sys/eventfd.h>
#include <unistd.h> // read, write

#define KERNEL_FILE "../src/virtual_car/raytracer.cl"

#define F3(x, y, z) cl_float3{{(x), (y), (z)}}

struct oclVirtualObject
{
  cl_int type;
  cl_float3 pos;
  cl_float3 facing;
  cl_float3 right;
  cl_float3 up;
  cl_float3 size;
};

class ocOclRenderer
{
private:

  cl_platform_id   _platform_id = nullptr;
  cl_device_id     _device_id   = nullptr;
  cl_context       _context     = nullptr;
  cl_command_queue _queue       = nullptr;
  cl_program       _program     = nullptr;
  cl_kernel        _kernel      = nullptr;
  size_t           _allocated_memory_size = 0;
  size_t           _image_memory_size = 0;
  cl_image_format  _image_format = {};
  cl_image_desc    _image_description = {};
  void            *_image_memory = nullptr;
  cl_mem           _image_object = nullptr;
  cl_mem           _world_tiles  = nullptr;
  cl_mem           _objects      = nullptr;
  cl_mem           _cam_normals  = nullptr;
  uint32_t         _image_num = 0;
  bool             _running = false;
  uint32_t         _image_width = 0;
  uint32_t         _image_height = 0;
  uint32_t         _image_channels = 0;
  size_t           _image_stride = 0;
  bool             _is_float = false;

  ocLogger logger;

  static std::string _loadKernel(const char *name)
  {
      std::ifstream in(name);
      std::string result((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
      return result;
  }

  static void CL_CALLBACK _kernel_finished_callback(
    cl_event event,
    cl_int event_command_exec_status,
    void *user_data)
  {
    (void) event_command_exec_status; // don't care about this param

    ocOclRenderer *me = (ocOclRenderer *)user_data;

    cl_int clresult = clReleaseEvent(event);
    if (CL_SUCCESS != clresult)
    {
      me->logger.warn("Error while releasing rendering event: (%i) %s", clresult, _ocl_error_string(clresult));
    }

    uint64_t data = 1;
    ssize_t result = write(me->wait_fd, &data, sizeof(data));
    if (result < 0)
    {
      me->logger.error("Error writing to kernel wait fd: (%i) %s", errno, strerror(errno));
    }
  }

  // https://stackoverflow.com/questions/24326432/convenient-way-to-show-opencl-error-codes
  static const char *_ocl_error_string(cl_int error)
  {
    switch(error){
    // run-time and JIT compiler errors
    case 0: return "CL_SUCCESS";
    case -1: return "CL_DEVICE_NOT_FOUND";
    case -2: return "CL_DEVICE_NOT_AVAILABLE";
    case -3: return "CL_COMPILER_NOT_AVAILABLE";
    case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case -5: return "CL_OUT_OF_RESOURCES";
    case -6: return "CL_OUT_OF_HOST_MEMORY";
    case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case -8: return "CL_MEM_COPY_OVERLAP";
    case -9: return "CL_IMAGE_FORMAT_MISMATCH";
    case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case -11: return "CL_BUILD_PROGRAM_FAILURE";
    case -12: return "CL_MAP_FAILURE";
    case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    case -15: return "CL_COMPILE_PROGRAM_FAILURE";
    case -16: return "CL_LINKER_NOT_AVAILABLE";
    case -17: return "CL_LINK_PROGRAM_FAILURE";
    case -18: return "CL_DEVICE_PARTITION_FAILED";
    case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

    // compile-time errors
    case -30: return "CL_INVALID_VALUE";
    case -31: return "CL_INVALID_DEVICE_TYPE";
    case -32: return "CL_INVALID_PLATFORM";
    case -33: return "CL_INVALID_DEVICE";
    case -34: return "CL_INVALID_CONTEXT";
    case -35: return "CL_INVALID_QUEUE_PROPERTIES";
    case -36: return "CL_INVALID_COMMAND_QUEUE";
    case -37: return "CL_INVALID_HOST_PTR";
    case -38: return "CL_INVALID_MEM_OBJECT";
    case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case -40: return "CL_INVALID_IMAGE_SIZE";
    case -41: return "CL_INVALID_SAMPLER";
    case -42: return "CL_INVALID_BINARY";
    case -43: return "CL_INVALID_BUILD_OPTIONS";
    case -44: return "CL_INVALID_PROGRAM";
    case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
    case -46: return "CL_INVALID_KERNEL_NAME";
    case -47: return "CL_INVALID_KERNEL_DEFINITION";
    case -48: return "CL_INVALID_KERNEL";
    case -49: return "CL_INVALID_ARG_INDEX";
    case -50: return "CL_INVALID_ARG_VALUE";
    case -51: return "CL_INVALID_ARG_SIZE";
    case -52: return "CL_INVALID_KERNEL_ARGS";
    case -53: return "CL_INVALID_WORK_DIMENSION";
    case -54: return "CL_INVALID_WORK_GROUP_SIZE";
    case -55: return "CL_INVALID_WORK_ITEM_SIZE";
    case -56: return "CL_INVALID_GLOBAL_OFFSET";
    case -57: return "CL_INVALID_EVENT_WAIT_LIST";
    case -58: return "CL_INVALID_EVENT";
    case -59: return "CL_INVALID_OPERATION";
    case -60: return "CL_INVALID_GL_OBJECT";
    case -61: return "CL_INVALID_BUFFER_SIZE";
    case -62: return "CL_INVALID_MIP_LEVEL";
    case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
    case -64: return "CL_INVALID_PROPERTY";
    case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
    case -66: return "CL_INVALID_COMPILER_OPTIONS";
    case -67: return "CL_INVALID_LINKER_OPTIONS";
    case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

    // extension errors
    case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
    case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
    case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
    case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
    case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
    case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
    default: return "Unknown OpenCL error";
    }
  }

  bool _init_opencl(int platform_number, int device_number)
  {
    if (platform_number < 0 || device_number < 0)
    {
      logger.warn("No platform or device number provided. Will use the first one. These would be your options:");
      print_opencl_devices();
      platform_number = 0;
      device_number = 0;
    }

    cl_uint platform_id_count = 0;
    clGetPlatformIDs(0, nullptr, &platform_id_count);

    if (0 == platform_id_count)
    {
      logger.error("No platforms found.");
      return false;
    }

    if ((int)platform_id_count <= platform_number)
    {
      logger.error("Provided platform number (%i) out of range (%i)", platform_number, platform_id_count);
      return false;
    }

    ocArray<cl_platform_id> platform_ids(platform_id_count);
    clGetPlatformIDs(platform_id_count, platform_ids.all(), nullptr);
    _platform_id = platform_ids[(size_t)platform_number];

    size_t platform_name_length = 0;
    clGetPlatformInfo(_platform_id, CL_PLATFORM_NAME, 0, nullptr, &platform_name_length);
    char *platform_name = (char *)malloc(sizeof(char) * platform_name_length);
    clGetPlatformInfo(_platform_id, CL_PLATFORM_NAME, platform_name_length, platform_name, nullptr);
    logger.log("Using OpenCL platform: %s", platform_name);
    free(platform_name);

    cl_uint device_id_count = 0;
    clGetDeviceIDs(_platform_id, CL_DEVICE_TYPE_ALL, 0, nullptr, &device_id_count);

    if (0 == device_id_count)
    {
      logger.error("No devices found.");
      return false;
    }

    if ((int)device_id_count <= device_number)
    {
      logger.error("Provided device number (%i) out of range (%i)", device_number, device_id_count);
      return false;
    }

    ocArray<cl_device_id> device_ids(device_id_count);
    clGetDeviceIDs(_platform_id, CL_DEVICE_TYPE_ALL, device_id_count, device_ids.all(), nullptr);
    _device_id = device_ids[(size_t)device_number];

    size_t device_name_length = 0;
    clGetDeviceInfo(_device_id, CL_DEVICE_NAME, 0, nullptr, &device_name_length);
    char *device_name = (char *)malloc(sizeof(char) * device_name_length);
    clGetDeviceInfo(_device_id, CL_DEVICE_NAME, device_name_length, device_name, nullptr);
    logger.log("Using OpenCL device: %s", device_name);
    free(device_name);

    const cl_context_properties context_properties[] = {
      CL_CONTEXT_PLATFORM, (cl_context_properties) _platform_id, 0, 0
    };

    cl_int result = 0;
    _context = clCreateContext(context_properties, 1, &_device_id, nullptr, nullptr, &result);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not create OpenCL context: (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    _queue = clCreateCommandQueue(_context, _device_id, 0, &result);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not create OpenCL command queue: (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    return true;
  }

  bool _init_kernel(const char *source_path)
  {
    std::string source = _loadKernel(source_path);
    if (0 == source.size())
    {
      logger.error("Could not load OpenCL kernel source.");
      return false;
    }

    cl_int result;
    size_t lengths[1] = {source.size()};
    const char *sources[1] = {source.c_str()};
    _program = clCreateProgramWithSource(_context, 1, sources, lengths, &result);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not create OpenCL program: (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    result = clBuildProgram(_program, 1, &_device_id, nullptr, nullptr, nullptr);
    if (CL_SUCCESS != result)
    {
      if (CL_BUILD_PROGRAM_FAILURE == result)
      {
        size_t log_size;
        clGetProgramBuildInfo(_program, _device_id, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);

        char *log = (char *)malloc(log_size);
        clGetProgramBuildInfo(_program, _device_id, CL_PROGRAM_BUILD_LOG, log_size, log, nullptr);
        logger.error(log);
        free(log);
      }

      logger.error("Could not build OpenCL program: (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    _kernel = clCreateKernel(_program, "MAIN", &result);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not create OpenCL kernel: (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    return true;
  }

  bool _set_image_size(uint32_t width, uint32_t height, uint32_t channels, bool is_float)
  {
    oc_assert(!_running);
    cl_int result;
    if (_image_width    == width &&
        _image_height   == height &&
        _image_channels == channels &&
        _is_float == is_float)
    {
      return true;
    }
    _image_width    = width;
    _image_height   = height;
    _image_channels = channels;
    _image_stride   = channels * width * (is_float ? sizeof(float) : sizeof(uint8_t));
    _image_memory_size = height * _image_stride;

    switch (channels)
    {
      case 1:
        _image_format.image_channel_order = CL_INTENSITY;
        break;
      case 4:
        _image_format.image_channel_order = (is_float) ? CL_RGBA : CL_BGRA;
        break;
    }
    _image_format.image_channel_data_type = (is_float) ? CL_FLOAT : CL_UNORM_INT8;

    _image_description.image_type = CL_MEM_OBJECT_IMAGE2D;
    _image_description.image_width = width;
    _image_description.image_height = height;
    _image_description.image_depth = 1;
    _image_description.image_array_size = 1;
    _image_description.image_row_pitch = width * channels * (is_float ? sizeof(float) : sizeof(uint8_t));
    _image_description.image_slice_pitch = 0;
    _image_description.num_mip_levels = 0;
    _image_description.num_samples = 0;
    _image_description.buffer = nullptr;

    if (_allocated_memory_size < _image_memory_size)
    {
      _image_memory = realloc(_image_memory, _image_memory_size);
      if (nullptr == _image_memory)
      {
        logger.error("Could not allocate image memory.");
        return false;
      }
      _allocated_memory_size = _image_memory_size;
    }

    if (_image_object)
    {
      result = clReleaseMemObject(_image_object);
      if (CL_SUCCESS != result)
      {
        logger.warn("Could not release old OpenCL image buffer: (%i) %s", result, _ocl_error_string(result));
      }
    }
    _image_object = clCreateImage(_context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, &_image_format, &_image_description, _image_memory, &result);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not create OpenCL image buffer: (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    result = clSetKernelArg(_kernel, 0, sizeof(cl_mem), &_image_object);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not set kernel argument 0 (result): (%i) %s", result, _ocl_error_string(result));
      return false;
    }
    return true;
  }

public:

  int wait_fd = -1;

  ocOclRenderer() : logger("Ocl Renderer")
  {
  }

  bool init(int platform_number, int device_number)
  {
    wait_fd = eventfd(0, 0);
    if (wait_fd < 0)
    {
      logger.error("Could not create kernel wait fd: (%i) %s", errno, strerror(errno));
      return false;
    }
    return _init_opencl(platform_number, device_number) &&
           _init_kernel(KERNEL_FILE);
  }

  void print_opencl_devices() const
  {
    cl_uint platform_id_count = 0;
    clGetPlatformIDs(0, nullptr, &platform_id_count);

    if (0 == platform_id_count)
    {
      logger.error("No platforms found.");
      return;
    }

    ocArray<cl_platform_id> platform_ids(platform_id_count);
    clGetPlatformIDs(platform_id_count, platform_ids.all(), nullptr);
    int platform_number = 0;
    for (auto &platform_id : platform_ids)
    {
      size_t platform_name_length = 0;
      clGetPlatformInfo(platform_id, CL_PLATFORM_NAME, 0, nullptr, &platform_name_length);
      char *platform_name = (char*)malloc(sizeof(char) * platform_name_length);
      clGetPlatformInfo(platform_id, CL_PLATFORM_NAME, platform_name_length, platform_name, nullptr);
      logger.log("[%i] Platform: %s", platform_number++, platform_name);
      free(platform_name);

      cl_uint device_id_count = 0;
      clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, 0, nullptr, &device_id_count);
      if (0 == device_id_count) {
        logger.log("    No devices found.");
        continue;
      }

      ocArray<cl_device_id> device_ids(device_id_count);
      clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, device_id_count, device_ids.all(), nullptr);
      int device_number = 0;
      for (auto &device_id: device_ids)
      {
        size_t device_name_length = 0;
        clGetDeviceInfo(device_id, CL_DEVICE_NAME, 0, nullptr, &device_name_length);
        char *device_name = (char*)malloc(sizeof(char) * device_name_length);
        clGetDeviceInfo(device_id, CL_DEVICE_NAME, device_name_length, device_name, nullptr);
        logger.log("    [%i] Device: %s", device_number++, device_name);
        free(device_name);
      }
    }
  }

  bool set_world(
    const ocRoadTile *world_tiles,
    uint32_t world_width,
    uint32_t world_height,
    const ocArray<ocVirtualObject> &objects,
    Vec3 sun_dir)
  {
    oc_assert(!_running);
    cl_int result;
    uint32_t world_size[] = { world_width, world_height };
    uint32_t num_tiles = world_width * world_height;

    // copy the tiles, because we want the parameter to be const, but
    // opencl won*t take a const pointer for clCreateBuffer.
    size_t tiles_size = num_tiles * sizeof(uint8_t);
    uint8_t *tiles = (uint8_t *)malloc(tiles_size);
    for (uint32_t i = 0; i < world_width * world_height; ++i)
    {
      tiles[i] = (uint8_t)world_tiles[i].type;
    }

    if (_world_tiles)
    {
      result = clReleaseMemObject(_world_tiles);
      if (CL_SUCCESS != result)
      {
        logger.warn("Could not release previous world_tiles buffer: (%i) %s", result, _ocl_error_string(result));
      }
    }

    _world_tiles = clCreateBuffer(_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, tiles_size, tiles, &result);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not create world tile buffer: (%i), %s", result, _ocl_error_string(result));
      return false;
    }
    free(tiles);

    result = clSetKernelArg(_kernel, 2, sizeof(cl_mem), &_world_tiles);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not set kernel argument 2 (map): (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    result = clSetKernelArg(_kernel, 3, sizeof(cl_int2), &world_size[0]);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not set kernel argument 3 (map_size): (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    size_t mem_size = (1UL + objects.get_length()) * sizeof(oclVirtualObject);
    oclVirtualObject *ocl_objects = (oclVirtualObject *)malloc(mem_size);
    if (!ocl_objects)
    {
      logger.error("Could not allocate object buffer");
      return false;
    }

    ocl_objects[0] = {
      .type   = (cl_int)ocObjectType::Road_Markings,
      .pos    = F3((float)(world_width - 1) * 100.0f, (float)(world_height - 1) * 100.0f, -1.0f),
      .facing = F3(1.0f, 0.0f, 0.0f),
      .right  = F3(0.0f, 1.0f, 0.0f),
      .up     = F3(0.0f, 0.0f, 1.0f),
      .size   = F3((float)world_width * 200.0f, (float)world_height * 200.0f, 2.0f)
    };

    for (size_t i = 1; auto &obj : objects)
    {
      auto x = obj.pose.x_axis();
      auto y = obj.pose.y_axis();
      auto z = obj.pose.z_axis();
      ocl_objects[i] = {
        .type   = (cl_int)obj.type,
        .pos    = F3(obj.pose.pos.x, obj.pose.pos.y, obj.pose.pos.z),
        .facing = F3(x.x, x.y, x.z),
        .right  = F3(y.x, y.y, y.z),
        .up     = F3(z.x, z.y, z.z),
        .size   = F3(obj.size.x, obj.size.y, obj.size.z)
      };
      ++i;
    }

    if (_objects)
    {
      result = clReleaseMemObject(_objects);
      if (CL_SUCCESS != result)
      {
        logger.warn("Could not release previous objects buffer: (%i) %s", result, _ocl_error_string(result));
      }
    }

    _objects = clCreateBuffer(_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, mem_size, ocl_objects, &result);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not create object buffer: (%i), %s", result, _ocl_error_string(result));
      return false;
    }

    free(ocl_objects);

    result = clSetKernelArg(_kernel, 4, sizeof(cl_mem), &_objects);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not set kernel argument 4 (objects): (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    cl_int num_objs = (cl_int)objects.get_length() + 1;
    result = clSetKernelArg(_kernel, 5, sizeof(cl_int), &num_objs);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not set kernel argument 5 (num_objects): (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    Vec4 sun_dir_v(sun_dir, 0);
    result = clSetKernelArg(_kernel, 13, sizeof(cl_float3), &sun_dir_v);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not set kernel argument 13 (sun_dir): (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    return true;
  }

  void init_ortho_camera(
    ocRenderCamera *camera,
    int32_t image_width,
    int32_t image_height,
    int32_t image_channels,
    float   target_width,
    float   target_height)
  {
    camera->mem_size  = (size_t)(image_width * image_height) * sizeof(cl_float3);
    camera->mem       = (cl_float3 *)realloc(camera->mem, camera->mem_size);
    camera->is_ortho  = 1;
    camera->is_linear = 0;
    camera->width     = (uint32_t)image_width;
    camera->height    = (uint32_t)image_height;
    camera->channels  = (uint32_t)image_channels;
    cl_float3 *cursor = (cl_float3 *)camera->mem;
    for (int v = 0; v < image_height; ++v)
    for (int u = 0; u < image_width; ++u)
    {
      *cursor++ = F3(
        0.0f,
        ((float)v - (float)image_height * 0.5f) / (float)image_height * target_height,
        ((float)u - (float)image_width * 0.5f) / (float)image_width * target_width);
    }
  }

  void init_perspective_camera(
    ocRenderCamera *camera,
    int32_t image_width,
    int32_t image_height,
    int32_t image_channels,
    float   sensor_offset_x,
    float   sensor_offset_y,
    float   fov,
    float   distortion)
  {
    camera->mem_size  = (size_t)(image_width * image_height) * sizeof(cl_float3);
    camera->mem       = (cl_float3 *)realloc(camera->mem, camera->mem_size);
    camera->is_ortho  = 0;
    camera->is_linear = (0.0f == distortion);
    camera->width     = (uint32_t)image_width;
    camera->height    = (uint32_t)image_height;
    camera->channels  = (uint32_t)image_channels;

    ocCameraProjector projector(
      camera->width, camera->height,
      sensor_offset_x, sensor_offset_y,
      ocPose(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
      fov,
      distortion);

    cl_float3 *cursor = (cl_float3 *)camera->mem;
    for (int v = 0; v < image_height; ++v)
    for (int u = 0; u < image_width; ++u)
    {
      projector.ego_to_world((float)u, (float)v, &cursor->s[0], &cursor->s[1], &cursor->s[2]);
      ++cursor;
    }
  }

  bool set_camera_properties(ocRenderCamera *camera, float noise_strength, float brightness, bool is_float)
  {
    oc_assert(!_running);
    cl_int result;

    if (!_set_image_size(camera->width, camera->height, camera->channels, is_float))
    {
      return false;
    }
    _is_float = is_float; 

    if (_cam_normals)
    {
      result = clReleaseMemObject(_cam_normals);
      if (CL_SUCCESS != result)
      {
        logger.warn("Could not release previous cam_normals buffer: (%i) %s", result, _ocl_error_string(result));
      }
    }

    _cam_normals = clCreateBuffer(_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, camera->mem_size, camera->mem, &result);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not create camera normal buffer: (%i), %s", result, _ocl_error_string(result));
      return false;
    }

    result = clSetKernelArg(_kernel, 1, sizeof(cl_mem), &_cam_normals);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not set kernel argument 1 (cam_normals): (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    cl_int is_ortho = camera->is_ortho;
    result = clSetKernelArg(_kernel, 10, sizeof(cl_int), &is_ortho);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not set kernel argument 10 (cam_ortho): (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    cl_int is_linear = camera->is_linear;
    result = clSetKernelArg(_kernel, 11, sizeof(cl_int), &is_linear);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not set kernel argument 11 (cam_linear): (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    result = clSetKernelArg(_kernel, 14, sizeof(cl_float), &noise_strength);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not set kernel argument 14 (noise_strength): (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    result = clSetKernelArg(_kernel, 15, sizeof(cl_float), &brightness);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not set kernel argument 15 (brightness): (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    return true;
  }

  bool set_camera_pose(ocPose pose)
  {
    oc_assert(!_running);
    cl_int result;
    Mat4 transformation = pose.get_generalize_mat();
    Vec4 cam_dir_forward = transformation.col(0);
    Vec4 cam_dir_right   = transformation.col(1);
    Vec4 cam_dir_up      = transformation.col(2);
    Vec4 cam_pos         = transformation.col(3);

    result = clSetKernelArg(_kernel, 6, sizeof(cl_float3), &cam_pos);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not set kernel argument 6 (cam_pos): (%i) %s", result, _ocl_error_string(result));
      return false;
    }
    result = clSetKernelArg(_kernel, 7, sizeof(cl_float3), &cam_dir_forward);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not set kernel argument 7 (cam_dir_forward): (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    result = clSetKernelArg(_kernel, 8, sizeof(cl_float3), &cam_dir_right);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not set kernel argument 8 (cam_dir_right): (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    result = clSetKernelArg(_kernel, 9, sizeof(cl_float3), &cam_dir_up);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not set kernel argument 9 (cam_dir_up): (%i) %s", result, _ocl_error_string(result));
      return false;
    }

    return true;
  }

  bool start_rendering()
  {
    _running = true;
    const size_t globalWorkSize[] = { (size_t)(_image_width * _image_height), 0, 0};

    cl_int result = clSetKernelArg(_kernel, 12, sizeof(cl_uint), &_image_num);
    if (CL_SUCCESS != result)
    {
      logger.error("Could not set kernel argument 12 (image_num): (%i) %s", result, _ocl_error_string(result));
      return false;
    }
    _image_num++;

    cl_event event;
    result = clEnqueueNDRangeKernel(_queue, _kernel, 1, nullptr, globalWorkSize, nullptr, 0, nullptr, &event);
    if (CL_SUCCESS != result)
    {
      logger.error("Error while kicking off rendering: %i %s", result, _ocl_error_string(result));
      return false;
    }

    result = clSetEventCallback(event, CL_COMPLETE, _kernel_finished_callback, this);
    if (CL_SUCCESS != result)
    {
      logger.error("Error setting render callback: %i %s", result, _ocl_error_string(result));
      return false;
    }

    return true;
  }

  bool get_rendered_image(void *target, size_t target_stride)
  {
    uint64_t tmp;
    if (read(wait_fd, &tmp, sizeof(tmp)) < 0)
    {
      logger.error("Error reading from renderer wait_fd: (%i) %s", errno, strerror(errno));
      return false;
    }

    cl_int result = clFinish(_queue);
    if (CL_SUCCESS != result)
    {
      logger.error("Error while waiting for image to finish: %i %s", result, _ocl_error_string(result));
      _running = false;
      return false;
    }

    for (size_t i = 0; i < _image_height; ++i)
    {
      void       *dst = (std::byte *)target + i * target_stride;
      const void *src = (std::byte *)_image_memory + i * _image_stride;
      memcpy(dst, src, _image_stride);
    }
    _running = false;
    return true;
  }
};
