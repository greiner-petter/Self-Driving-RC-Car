#pragma once

#include <string_view>

#include "ocRenderTarget.h"

namespace oc
{

enum class EventType
{
  None,
  Draw,
  Pointer,
  Key,
  Scroll,
  Zoom,
  Resize,
  Close
};

struct EventHeader
{
  EventType type;
  int64_t   time; // in ns
};

struct PointerEvent
{
  EventHeader header;
  // Mouse position is float, in case we need that because of non-integer display scaling.
  // The ~23bit integer resolution is more than enough to precisely locate the mouse even on high
  // resolutions.
  float old_x, old_y;
  float new_x, new_y;
  bool dragged;
};

enum class KeyCode
{
  Unknown = 0,

  // Try to map character keys to their ascii value. 

  Key_Space       = ' ', // 32
  Key_Exclamation = '!', // 33
  Key_Quote       = '"', // 34
  Key_Hash        = '#', // 35
  Key_Star        = '*', // 42
  Key_Plus        = '+', // 43
  Key_Comma       = ',', // 44
  Key_Minus       = '-', // 45
  Key_Period      = '.', // 46
  Key_Slash       = '/', // 47

  Key_0 = '0', // 48
  Key_1 = '1', // 49
  Key_2 = '2', // 50
  Key_3 = '3', // 51
  Key_4 = '4', // 52
  Key_5 = '5', // 53
  Key_6 = '6', // 54
  Key_7 = '7', // 55
  Key_8 = '8', // 56
  Key_9 = '9', // 57

  Key_Question = '?', // 63

  Key_A = 'a', // 65
  Key_B = 'b', // 66
  Key_C = 'c', // 67
  Key_D = 'd', // 68
  Key_E = 'e', // 69
  Key_F = 'f', // 70
  Key_G = 'g', // 71
  Key_H = 'h', // 72
  Key_I = 'i', // 73
  Key_J = 'j', // 74
  Key_K = 'k', // 75
  Key_L = 'l', // 76
  Key_M = 'm', // 77
  Key_N = 'n', // 78
  Key_O = 'o', // 79
  Key_P = 'p', // 80
  Key_Q = 'q', // 81
  Key_R = 'r', // 82
  Key_S = 's', // 83
  Key_T = 't', // 84
  Key_U = 'u', // 85
  Key_V = 'v', // 86
  Key_W = 'w', // 87
  Key_X = 'x', // 88
  Key_Y = 'y', // 89
  Key_Z = 'z', // 90

  // all key not representable in ascii get codes above 255

  Key_F1  = 256,
  Key_F2  = 257,
  Key_F3  = 258,
  Key_F4  = 259,
  Key_F5  = 260,
  Key_F6  = 261,
  Key_F7  = 262,
  Key_F8  = 263,
  Key_F9  = 264,
  Key_F10 = 265,
  Key_F11 = 266,
  Key_F12 = 267,
  Key_F13 = 268,
  Key_F14 = 269,
  Key_F15 = 260,
  Key_F16 = 271,
  Key_F17 = 272,
  Key_F18 = 273,
  Key_F19 = 274,
  Key_F20 = 275,
  Key_F21 = 276,
  Key_F22 = 277,
  Key_F23 = 278,
  Key_F24 = 279,


  Key_Tab       = 280,
  Key_Backspace = 281,
  Key_Delete    = 282,
  Key_Insert    = 283,
  Key_Enter     = 284,
  Key_Shift     = 285,
  Key_Control   = 286,
  Key_Alt       = 287,
  Key_Escape    = 288,
  Key_Page_Up   = 289,
  Key_Page_Down = 290,
  Key_Home      = 291,

  Key_Arrow_Up    = 292,
  Key_Arrow_Down  = 293,
  Key_Arrow_Left  = 294,
  Key_Arrow_Right = 295,

  Mouse_1 = 296,
  Mouse_2 = 297,
  Mouse_3 = 298,
  Mouse_4 = 299,
  Mouse_5 = 300,
};

struct KeyEvent
{
  EventHeader header;
  KeyCode code;
  bool down;
  bool doubleclick;
};

struct ScrollEvent
{
  EventHeader header;
  float x, y;
};

struct ZoomEvent
{
  EventHeader header;
  float center_x, center_y;
  float factor;
};

struct ResizeEvent
{
  EventHeader header;
  int old_width, old_height;
  float old_scaling;
  int new_width, new_height;
  float new_scaling;
};

union InputEvent
{
  EventType    type;
  EventHeader  header;
  PointerEvent pointer;
  KeyEvent     key;
  ScrollEvent  scroll;
  ZoomEvent    zoom;
  ResizeEvent  resize;
};


// Forward declaration, so we can define it in the .cpp file.
// This way we don't have to expose the horrendous X11 headers to anyone
// who includes this window header.
struct WindowDetails;

class Window
{
private:

  int     backbuffer_width;
  int     backbuffer_height;
  float   system_scaling;
  float   mouse_x;
  float   mouse_y;

  WindowDetails *details;

public:

  Window(
    int initial_width,
    int initial_height,
    std::string_view title,
    bool resizable    = true,
    bool auto_scaling = true);

  ~Window();

  void close();

  Window(const Window&) = delete;
  void operator=(const Window&) = delete;

  void set_size(int width, int height);

  [[nodiscard]] int get_width()  const {return backbuffer_width; }
  [[nodiscard]] int get_height() const {return backbuffer_height; }

  [[nodiscard]] float get_scaling() const {return system_scaling; }

  [[nodiscard]] float get_mouse_x() const { return mouse_x; }
  [[nodiscard]] float get_mouse_y() const { return mouse_y; }

  [[nodiscard]] InputEvent next_event(bool blocking = false);

  [[nodiscard]] bool is_key_down(KeyCode key) const;

  void draw_pixel(int x, int y, const Color& color);
  void draw_pixel(int x, int y, const Color& color, float mask);

  void commit();

  void fill(const Color& color);

  void fill_rect(
    int x0, int y0, int x1, int y1,
    const Color& color);
};

} // namespace oc
