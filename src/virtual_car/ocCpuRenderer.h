#pragma once

#include "../common/ocAssert.h"
#include "../common/ocLogger.h"
#include "../common/ocMat.h"
#include "../common/ocPose.h"
#include "../common/ocVec.h"
#include "ocRenderCamera.h"
#include "ocSimulationWorld.h"

#include <cstdint>
#include <cerrno>
#include <sys/eventfd.h>

class ocCpuRenderer
{
private:
  ocLogger logger;

  bool _running = false;

  ocRoadTileType *_world_tiles  = nullptr;
  uint32_t      _world_width  = 0;
  uint32_t      _world_height = 0;

  ocVirtualObject *_objects   = nullptr;
  size_t           _obj_count = 0;

  ocRenderCamera *_camera        = nullptr;
  Mat4            _cam_transform = {};

  Vec3     _sun_dir        = {};

  uint32_t _image_width    = 0;
  uint32_t _image_height   = 0;
  uint32_t _image_channels = 0;
  size_t   _image_memory_size    = 0;
  size_t   _reserved_memory_size = 0;
  uint8_t *_image_memory         = nullptr;

  struct Plane
  {
    Vec3  normal;
    float distance;
  };

  struct Ray
  {
    Vec3 origin;
    Vec3 normal;
  };

  bool _intersect_ray_plane(const Ray *r, const Plane *p, float *result)
  {
    float d = dot(p->normal, r->normal);
    float t = -(dot(p->normal, r->origin) + p->distance) / d;
    if (d < 0.0f && 0.00001f < t) {
      *result = t;
      return true;
    }
    return false;
  }

  void _render(size_t img_x, size_t img_y, size_t img_w, size_t img_h)
  {
    for (size_t iy = 0; iy < img_h; ++iy)
    for (size_t ix = 0; ix < img_w; ++ix)
    {
      size_t index = (img_x + ix) + (img_y + iy) * _image_width;
      Ray ray;
      if (_camera->is_ortho)
      {
        ray.origin = (_cam_transform * Vec4(((Vec3 *)_camera->mem)[index], 1)).xyz();
        ray.normal = (_cam_transform * Vec4(1, 0, 0, 0)).xyz();
      }
      else
      {
        ray.origin = (_cam_transform * Vec4(0, 0, 0, 1)).xyz();
        ray.normal = (_cam_transform * Vec4(((Vec3 *)_camera->mem)[index], 0)).xyz();
      }

      Plane plane;
      plane.normal   = Vec3(0, 0, 1);
      plane.distance = 0;

      float dist = 100000000.0f;
      if (_intersect_ray_plane(&ray, &plane, &dist))
      {
        for (size_t c = 0; c < _image_channels; ++c)
        {
          _image_memory[index * _image_channels + c] = 0x00;
        }
      }
      else
      {
        for (size_t c = 0; c < _image_channels; ++c)
        {
          _image_memory[index * _image_channels + c] = 0xCC;
        }
      }
    }
  }

  bool _set_image_size(uint32_t width, uint32_t height, uint32_t channels)
  {
    oc_assert(!_running);

    if (_image_width == width &&
        _image_height == height &&
        _image_channels == channels)
    {
      return true;
    }
    _image_width = width;
    _image_height = height;
    _image_channels = channels;
    _image_memory_size = channels * width * height * sizeof(uint8_t);

    if (_reserved_memory_size < _image_memory_size)
    {
      _image_memory = (uint8_t *)realloc((void *)_image_memory, _image_memory_size);
      if (nullptr == _image_memory)
      {
        logger.error("Could not allocate image memory.");
        return false;
      }
      _reserved_memory_size = _image_memory_size;
    }

    return true;
  }

public:

  int wait_fd = -1;

  ocCpuRenderer() : logger("Cpu Renderer")
  {
  }

  bool init()
  {
    wait_fd = eventfd(0, 0);
    if (wait_fd < 0)
    {
      logger.error("Could not create kernel wait fd: (%i) %s", errno, strerror(errno));
      return false;
    }
    return true;
  }

  bool set_world(
    const ocRoadTile *world_tiles,
    uint32_t world_width,
    uint32_t world_height,
    const ocArray<ocVirtualObject> &objects,
    Vec3 sun_dir)
  {
    oc_assert(!_running);

    _world_width  = world_width;
    _world_height = world_height;
    size_t world_size = world_height * world_width * sizeof(ocRoadTileType);
    _world_tiles  = (ocRoadTileType *)realloc(_world_tiles, world_size);
    if (nullptr == _world_tiles) return false;
    for (uint32_t i = 0; i < world_width * world_height; ++i)
    {
      _world_tiles[i] = world_tiles[i].type;
    }

    _obj_count = objects.get_size();
    size_t objects_size = _obj_count * sizeof(ocVirtualObject);
    _objects = (ocVirtualObject *)realloc(_objects, objects_size);
    if (nullptr == _objects) return false;
    memcpy(_objects, objects.all(), objects_size);

    _sun_dir = sun_dir;

    return true;
  }

  void init_ortho_camera(
    ocRenderCamera *camera,
    uint32_t image_width,
    uint32_t image_height,
    uint32_t image_channels,
    float target_width,
    float target_height)
  {
    camera->mem_size = image_width * image_height * sizeof(Vec3);
    camera->mem      = (Vec3 *)realloc(camera->mem, camera->mem_size);
    camera->is_ortho = 1;
    camera->width    = image_width;
    camera->height   = image_height;
    camera->channels = image_channels;
    Vec3 *cursor = (Vec3 *)camera->mem;
    for (uint32_t v = 0; v < image_height; ++v)
    for (uint32_t u = 0; u < image_width; ++u)
    {
      *cursor++ = {
        0.0f,
        ((float)(v - image_height) * 0.5f) / (float)image_height * target_height,
        ((float)(u - image_width)  * 0.5f) / (float)image_width * target_width
      };
    }
  }

  void init_perspective_camera(
    ocRenderCamera *camera,
    uint32_t image_width,
    uint32_t image_height,
    uint32_t image_channels,
    float sensor_offset_x,
    float sensor_offset_y,
    float fov,
    float distortion)
  {
    camera->mem_size = image_width * image_height * sizeof(Vec3);
    camera->mem      = (Vec3 *)realloc(camera->mem, camera->mem_size);
    camera->is_ortho = 0;
    camera->width    = image_width;
    camera->height   = image_height;
    camera->channels = image_channels;

    ocCameraProjector projector(
      image_width, image_height,
      sensor_offset_x, sensor_offset_y,
      ocPose(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
      fov,
      distortion);

    Vec3 *cursor = (Vec3 *)camera->mem;
    for (uint32_t v = 0; v < image_height; ++v)
    for (uint32_t u = 0; u < image_width; ++u)
    {
      *cursor++ = projector.ego_to_world(Vec2((float)u, (float)v));
    }
  }

  bool set_camera_properties(ocRenderCamera *camera, float /*noise_strength*/, bool /*fp*/)
  {
    oc_assert(!_running);

    if (!_set_image_size(camera->width, camera->height, camera->channels))
    {
      return false;
    }

    _camera = camera;

    return true;
  }

  bool set_camera_pose(ocPose pose)
  {
    oc_assert(!_running);

    _cam_transform = pose.get_generalize_mat();

    return true;
  }

  bool start_rendering()
  {
    oc_assert(!_running);
    oc_assert(_camera);
    _running = true;

    _render(0, 0, _image_width, _image_height);

    uint64_t data = 1;
    ssize_t result = write(wait_fd, &data, sizeof(data));
    if (result < 0)
    {
      logger.error("Error writing to kernel wait fd: (%i) %s", errno, strerror(errno));
    }

    return true;
  }

  bool get_rendered_image(uint8_t *target)
  {
    uint64_t tmp;
    if (read(wait_fd, &tmp, sizeof(tmp)) < 0)
    {
      logger.error("Error reading from renderer wait_fd: (%i) %s", errno, strerror(errno));
      return false;
    }

    memcpy((void *)target, _image_memory, _image_memory_size);
    _running = false;
    return true;
  }
};
