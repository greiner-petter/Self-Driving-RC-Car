#pragma once

#include "../common/ocProfiler.h"
#include "../common/ocTime.h"
#include "../common/ocTypes.h" // ocPixelLayout

#include <cstdint> // int32_t
#include <ueye.h> // is_*

#include <sys/mman.h> // mlock

struct ocCameraSettings
{
    uint32_t      binning_h;
    uint32_t      binning_v;
    uint32_t      subsampling_h;
    uint32_t      subsampling_v;
    ocPixelFormat pixel_format;
    float         frame_rate;
    float         exposure_time_ms;
    int           gain_percent;
    bool          flip_left_right;
    bool          flip_up_down;
    bool          auto_shutter;
    bool          white_balance;
    bool          gain_boost;
};

class ocCamera
{
private:

    HIDS _device;
    int32_t  _pids[OC_NUM_CAM_BUFFERS];
    uint32_t _width = 0;
    uint32_t _height = 0;
    ocCameraSettings _settings;

    uint32_t _image_number = 0;
    ocTime   _image_time;

    ocLogger _logger;

public:

    ocCamera() : _logger("Camera") { }

    bool set_binning(uint32_t binning_h, uint32_t binning_v)
    {
        // reduce to the values the UEye API allows
        if (7 == binning_h) binning_h = 6;
        if (8 < binning_h && binning_h < 16) binning_h = 8;
        if (16 < binning_h) binning_h = 16;

        if (7 == binning_v) binning_v = 6;
        if (8 < binning_v && binning_v < 16) binning_v = 8;
        if (16 < binning_v) binning_v = 16;

        int binning = 0;
        switch (binning_h)
        {
        case  2: binning |= IS_BINNING_2X_HORIZONTAL; break;
        case  3: binning |= IS_BINNING_3X_HORIZONTAL; break;
        case  4: binning |= IS_BINNING_4X_HORIZONTAL; break;
        case  5: binning |= IS_BINNING_5X_HORIZONTAL; break;
        case  6: binning |= IS_BINNING_6X_HORIZONTAL; break;
        case  8: binning |= IS_BINNING_8X_HORIZONTAL; break;
        case 16: binning |= IS_BINNING_16X_HORIZONTAL; break;
        default: break;
        }
        switch (binning_v)
        {
        case  2: binning |= IS_BINNING_2X_VERTICAL; break;
        case  3: binning |= IS_BINNING_3X_VERTICAL; break;
        case  4: binning |= IS_BINNING_4X_VERTICAL; break;
        case  5: binning |= IS_BINNING_5X_VERTICAL; break;
        case  6: binning |= IS_BINNING_6X_VERTICAL; break;
        case  8: binning |= IS_BINNING_8X_VERTICAL; break;
        case 16: binning |= IS_BINNING_16X_VERTICAL; break;
        default: break;
        }

        int ret_val = is_SetBinning(_device, binning);
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_SetBinning ERROR: %i", ret_val);
            return false;
        }
        _settings.binning_h = binning_h;
        _settings.binning_v = binning_v;
        return true;
    }

    bool set_subsampling(uint32_t subsampling_h, uint32_t subsampling_v)
    {
        // reduce to the values the UEye API allows
        if (7 == subsampling_h) subsampling_h = 6;
        if (8 < subsampling_h && subsampling_h < 16) subsampling_h = 8;
        if (16 < subsampling_h) subsampling_h = 16;

        if (7 == subsampling_v) subsampling_v = 6;
        if (8 < subsampling_v && subsampling_v < 16) subsampling_v = 8;
        if (16 < subsampling_v) subsampling_v = 16;

        int subsampling = 0;
        switch (subsampling_h)
        {
        case  2: subsampling |= IS_SUBSAMPLING_2X_HORIZONTAL; break;
        case  3: subsampling |= IS_SUBSAMPLING_3X_HORIZONTAL; break;
        case  4: subsampling |= IS_SUBSAMPLING_4X_HORIZONTAL; break;
        case  5: subsampling |= IS_SUBSAMPLING_5X_HORIZONTAL; break;
        case  6: subsampling |= IS_SUBSAMPLING_6X_HORIZONTAL; break;
        case  8: subsampling |= IS_SUBSAMPLING_8X_HORIZONTAL; break;
        case 16: subsampling |= IS_SUBSAMPLING_16X_HORIZONTAL; break;
        default: break;
        }
        switch (subsampling_v)
        {
        case  2: subsampling |= IS_SUBSAMPLING_2X_VERTICAL; break;
        case  3: subsampling |= IS_SUBSAMPLING_3X_VERTICAL; break;
        case  4: subsampling |= IS_SUBSAMPLING_4X_VERTICAL; break;
        case  5: subsampling |= IS_SUBSAMPLING_5X_VERTICAL; break;
        case  6: subsampling |= IS_SUBSAMPLING_6X_VERTICAL; break;
        case  8: subsampling |= IS_SUBSAMPLING_8X_VERTICAL; break;
        case 16: subsampling |= IS_SUBSAMPLING_16X_VERTICAL; break;
        default: break;
        }

        int ret_val = is_SetSubSampling(_device, subsampling);
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_SetSubSampling ERROR: %i", ret_val);
            return false;
        }
        _settings.subsampling_h = subsampling_h;
        _settings.subsampling_v = subsampling_v;
        return true;
    }

    bool set_pixel_format(ocPixelFormat pixel_format)
    {
        SENSORINFO cam_sensor;
        int ret_val = is_GetSensorInfo(_device, &cam_sensor);
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_GetSensorInfo ERROR: %i", ret_val);
            return false;
        }

        if (IS_COLORMODE_MONOCHROME == cam_sensor.nColorMode)
        {
            ret_val = is_SetColorMode(_device, IS_CM_MONO8);
            pixel_format = ocPixelFormat::Gray_U8;
        }
        else
        {
            switch (pixel_format)
            {
            case ocPixelFormat::Gray_U8:
            {
                ret_val = is_SetColorMode(_device, IS_CM_MONO8);
            } break;
            case ocPixelFormat::Bgr_U8:
            {
                ret_val = is_SetColorMode(_device, IS_CM_BGR8_PACKED);
            } break;
            case ocPixelFormat::Bgra_U8:
            {
                ret_val = is_SetColorMode(_device, IS_CM_BGRA8_PACKED);
            } break;
            default:
            {
                _logger.error("Error, unsupported pixel layout: %s", to_string(pixel_format));
                return false;
            }
            }
        }
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_SetColorMode ERROR: %i", ret_val);
            _logger.error("For pixel layout: %s", to_string(pixel_format));
            return false;
        }
        _settings.pixel_format = pixel_format;
        return true;
    }

    bool set_flip(bool flip_left_right, bool flip_up_down)
    {
        int ret_val = is_SetRopEffect(_device, IS_SET_ROP_MIRROR_LEFTRIGHT, flip_left_right ? 1 : 0, 0);
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_SetColorMode IS_SET_ROP_MIRROR_LEFTRIGHT ERROR: %i", ret_val);
            return false;
        }
        _settings.flip_left_right = flip_left_right;

        ret_val = is_SetRopEffect(_device, IS_SET_ROP_MIRROR_UPDOWN, flip_up_down ? 1 : 0, 0);
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_SetColorMode IS_SET_ROP_MIRROR_UPDOWN ERROR: %i", ret_val);
            return false;
        }
        _settings.flip_up_down = flip_up_down;
        return true;
    }

    bool set_framerate(float fps)
    {
        double real_framerate = 0;
        int ret_val = is_SetFrameRate(_device, (double)fps, &real_framerate);
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_SetFrameRate ERROR: %i", ret_val);
            return false;
        }
        _settings.frame_rate = (float) real_framerate;
        return true;
    }

    bool set_auto_shutter(bool is_auto)
    {
        double val1 = is_auto ? 1 : 0, val2 = 0;
        int ret_val = is_SetAutoParameter(_device, IS_SET_ENABLE_AUTO_SHUTTER, &val1, &val2);
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_SetAutoParameter IS_SET_ENABLE_AUTO_SHUTTER ERROR: %i", ret_val);
            return false;
        }
        _settings.auto_shutter = is_auto;
        return true;
    }

    bool set_white_balance(bool white_balance)
    {
        double val1 = white_balance ? WB_MODE_AUTO : WB_MODE_DISABLE, val2 = 0;
        int ret_val = is_SetAutoParameter(_device, IS_SET_ENABLE_AUTO_WHITEBALANCE, &val1, &val2);
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_SetAutoParameter IS_SET_ENABLE_AUTO_SENSOR_WHITEBALANCE ERROR: %i", ret_val);
            return false;
        }
        _settings.white_balance = white_balance;
        return true;
    }

    bool set_gain_percent(int gain_percent)
    {
        int ret_val = is_SetHardwareGain(_device, gain_percent, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_SetHardwareGain ERROR: %i", ret_val);
            return false;
        }
        _settings.gain_percent = gain_percent;
        return true;
    }

    bool set_gain_boost(bool gain_boost)
    {
        int boost_val = gain_boost ? IS_SET_GAINBOOST_ON : IS_SET_GAINBOOST_OFF;
        int ret_val = is_SetGainBoost(_device, boost_val);
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_SetGainBoost ERROR: %i", ret_val);
            return false;
        }
        _settings.gain_boost = gain_boost;
        return true;
    }

    bool set_exposure_time(float exposure_time)
    {
        double value = (double)exposure_time;
        int ret_val = is_Exposure(_device, IS_EXPOSURE_CMD_SET_EXPOSURE, &value, sizeof(value));
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_SetGainBoost ERROR: %i", ret_val);
            return false;
        }
        _settings.exposure_time_ms = exposure_time;
        return true;
    }

    bool init(
        ocCameraSettings settings,
        ocCamData *cam_data,
        int32_t cam_data_count)
    {
        int ret_val;

        int num_cameras;
        ret_val = is_GetNumberOfCameras(&num_cameras);
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_GetNumberOfCameras ERROR: %i", ret_val);
            return false;
        }

        if (0 == num_cameras)
        {
            _logger.warn("No camera detected, check the connection.");
            _logger.warn("You probably forgot to start the driver. run: '/etc/init.d/ueyeusbdrc start'");
        }

        _device = 1;

        ret_val = is_InitCamera(&_device, nullptr);
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_InitCamera ERROR: %i", ret_val);
            return false;
        }

        SENSORINFO cam_sensor;
        ret_val = is_GetSensorInfo(_device, &cam_sensor);
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_GetSensorInfo ERROR: %i", ret_val);
            return false;
        }

        if (!set_binning(settings.binning_h, settings.binning_v)) return false;

        if (!set_subsampling(settings.subsampling_h, settings.subsampling_v)) return false;

        if (!set_pixel_format(settings.pixel_format)) return false;

        if (!set_flip(settings.flip_left_right, settings.flip_up_down)) return false;

        //if (!set_gain_boost(settings.gain_boost)) return false;

        if (!set_gain_percent(settings.gain_percent)) return false;

        _width  = cam_sensor.nMaxWidth  / _settings.binning_h / _settings.subsampling_h;
        _height = cam_sensor.nMaxHeight / _settings.binning_v / _settings.subsampling_v;
        ocPixelFormat pixel_format = _settings.pixel_format;
        uint32_t pixel_bytes = bytes_per_pixel(pixel_format);
        uint32_t memory_size = _width * _height * pixel_bytes;

        if (OC_CAM_BUFFER_SIZE < memory_size)
        {
            _logger.error("Error: Camera image is larger than the space in the shared memory!");
            return false;
        }

        // Make the image memory from the shared memory known to the camera driver
        for (int i = 0; i < cam_data_count; ++i)
        {
            char *memory = (char *)cam_data[i].img_buffer;
            mlock(memory, memory_size);

            ret_val = is_SetAllocatedImageMem(_device, (int)_width, (int)_height, (int)pixel_bytes * 8, memory, &_pids[i]);
            if (ret_val != IS_SUCCESS)
            {
                _logger.error("is_AllocImageMem ERROR %i in buffer %i", ret_val, i);
                return false;
            }
            if (_pids[i] != i + 1)
            {
                _logger.error("Error: PID of image buffer %i is not %i", i, i + 1);
                return false;
            }

            ret_val = is_AddToSequence(_device, memory, _pids[i]);
            if (ret_val != IS_SUCCESS)
            {
                _logger.error("is_AddToSequence ERROR: %i in buffer %i", ret_val, i);
                return false;
            }

            cam_data[i].width  = _width;
            cam_data[i].height = _height;
            cam_data[i].pixel_format = pixel_format;
        }

        if (!set_framerate(settings.frame_rate)) return false;
        _logger.log("Expected framerate: %f", _settings.frame_rate);

        if (!set_auto_shutter(settings.auto_shutter)) return false;

        if (ocPixelFormat::Gray_U8 != settings.pixel_format) // TODO: check if camera supports color
        {
            set_white_balance(settings.white_balance);
        }

        ret_val = is_CaptureVideo(_device, IS_DONT_WAIT);
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_CaptureVideo ERROR: %i", ret_val);
            return false;
        }

        ret_val = is_EnableEvent(_device, IS_SET_EVENT_FIRST_PACKET_RECEIVED);
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_EnableEvent(IS_SET_EVENT_FIRST_PACKET_RECEIVED) ERROR: %i", ret_val);
            return false;
        }

        ret_val = is_EnableEvent(_device, IS_SET_EVENT_FRAME);
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_EnableEvent(IS_SET_EVENT_FRAME) ERROR: %i", ret_val);
            return false;
        }

        return true;
    }

    bool exit()
    {
        int ret_val  = is_ExitCamera(_device);
        if (ret_val != IS_SUCCESS)
        {
            _logger.error("is_ExitCamera ERROR: %i", ret_val);
            return false;
        }
        return true;
    }

    bool read_frame()
    {
        int ret;

        // TODO: move these waits into one epoll together with the ipc socket
        // wait up to 100ms for a new frame
        ret = is_WaitEvent(_device, IS_SET_EVENT_FIRST_PACKET_RECEIVED, 100);
        _image_time = ocTime::now();

        if (ret != IS_SUCCESS)
        {
            _logger.error("is_WaitEvent(IS_SET_EVENT_FIRST_PACKET_RECEIVED) ERROR: %i", ret);
            return false;
        }

        BEGIN_TIMED_BLOCK("Wait for driver");
        ret = is_WaitEvent(_device, IS_SET_EVENT_FRAME, 100);

        if (ret != IS_SUCCESS)
        {
            _logger.error("is_WaitEvent(IS_SET_EVENT_FRAME) ERROR: %i", ret);
            return false;
        }

        END_TIMED_BLOCK();

        _image_number++;
        return true;
    }

    int set_parameter(int32_t param_id, double val1, double val2)
    {
        return is_SetAutoParameter(_device, param_id, &val1, &val2);
    }

    uint32_t get_image_number() const
    {
        return _image_number;
    }

    uint32_t get_image_index() const
    {
        // get the image buffer where the image will be saved
        // The IDS-API indexes the image buffers from 1 to 3 in _pids[],
        // but we want the array-index, so we have to decrement by 1

        int32_t image_index;
        int ret = is_GetActiveImageMem(_device, nullptr, &image_index);

        if (ret != IS_SUCCESS)
        {
            _logger.error("is_GetActiveImageMem _next_image_index ERROR: %i", ret);
            return (uint32_t)-1;
        }

        return (uint32_t)(image_index - 1);
    }

    ocTime get_image_time() const
    {
        return _image_time;
    }

    float get_expected_framerate() const
    {
        return _settings.frame_rate;
    }
};
