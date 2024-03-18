#include "ocWindow.h"

#include "ocAssert.h"
#include "ocTime.h"

#include <unistd.h>
#include <EGL/egl.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// Fuck you, X
#undef None

#include <cstring>

namespace oc
{

typedef void (*GlBlitFramebuffer)(
  int32_t srcX0, int32_t srcY0,
  int32_t srcX1, int32_t srcY1,
  int32_t dstX0, int32_t dstY0,
  int32_t dstX1, int32_t dstY1,
  uint32_t mask,
  uint32_t filter);

typedef void (*GlGenFramebuffers)(
  uint32_t n,
  uint32_t *ids);

typedef void (*GlBindFramebuffer)(
  uint32_t target,
  uint32_t framebuffer);


typedef void (*GlFramebufferTexture2D)(
  uint32_t target,
  uint32_t attachment,
  uint32_t textarget,
  uint32_t texture,
  int32_t  level);

typedef int32_t (*GlCheckFramebufferStatus)(
  int32_t target);

typedef void (*GlGenBuffers)(
  uint32_t n,
  uint32_t *ids);

typedef void (*GlBindBuffer)(
  uint32_t target,
  uint32_t buffer);

typedef void *(*GlMapBuffer)(
  int32_t target,
  int32_t access);

typedef int32_t (*GlUnmapBuffer)(
  int32_t target);

typedef void (*GlBufferData)(
  int32_t     target,
  uint64_t    size,
  const void *data,
  int32_t     usage);

static struct
{
  GlBlitFramebuffer        glBlitFramebuffer;
  GlGenFramebuffers        glGenFramebuffers;
  GlBindFramebuffer        glBindFramebuffer;
  GlFramebufferTexture2D   glFramebufferTexture2D;
  GlCheckFramebufferStatus glCheckFramebufferStatus;
  GlGenBuffers             glGenBuffers;
  GlBindBuffer             glBindBuffer;
  GlMapBuffer              glMapBuffer;
  GlUnmapBuffer            glUnmapBuffer;
  GlBufferData             glBufferData;

  bool initialized = false;
} opengl;

struct Pixel
{
  Color color;
  float alpha;
};
static_assert(sizeof(Pixel) == 16);

struct Window::WindowDetails
{
  Display   *x11_display;
  ::Window   window;
  int        screen;
  Atom       wm_delete_window;
  XSizeHints hints;
  XIC        input_context;
  Time       last_button_time[10];
  Pixel     *backbuffer;
  int32_t    backbuffer_stride;
  int32_t    old_backbuffer_width;
  int32_t    old_backbuffer_height;
  EGLDisplay egl_display;
  EGLSurface egl_surface;
  uint32_t   fbo;
  uint32_t   pixelbuffer;
  int        num_keys_held;
  KeyCode    keys_held[16]; // Nobody can hold more than 16 keys!
  bool       resized;
  bool       minimized;
};

#define check_egl_error() check_egl_error_(__FILE__, __PRETTY_FUNCTION__, __LINE__);
static void check_egl_error_(const char *file, const char *function, int line) {
  int32_t error = eglGetError();
  while (EGL_SUCCESS != error)
  {
    printf("EGL error: %d Function: %s File: %s (%i)\n", error, function, file, line);
    error = eglGetError();
  }
}

#define check_gl_error() check_gl_error_(__FILE__, __PRETTY_FUNCTION__, __LINE__);
static void check_gl_error_(const char *file, const char *function, int line) {
  uint32_t error = glGetError();
  while (GL_NO_ERROR != error)
  {
    const char *message = (const char *)gluErrorString(error);
    printf("GL Error: %u Message: %s Function: %s File: %s (%i)\n", error, message, function, file, line);
    error = glGetError();
  }
}


static void push_key(Window::WindowDetails *details, KeyCode key)
{
  // When a key is held down, it'll be sent repeatedly. This condition
  // makes sure, that we don't completely fill the array because of that.
  if (1 <= details->num_keys_held && details->keys_held[0] == key) return;

  // shift all previous entries back by one (and drop the last) so that
  // the newest key down is always at the front.
  // This way, if we ever have to drop a key, it'll be the oldest one,
  // which the user hopefully doesn't notice.
  memmove(&details->keys_held[1], &details->keys_held[0], sizeof(KeyCode) * 15);
  details->keys_held[0] = key;
  if (details->num_keys_held < 16) ++details->num_keys_held;
}

static void pop_key(Window::WindowDetails *details, KeyCode key)
{
  for (int i = 0; i < details->num_keys_held; ++i)
  {
    if (key == details->keys_held[i])
    {
      memmove(&details->keys_held[i], &details->keys_held[i + 1], sizeof(KeyCode) * uint32_t(15 - i));
      --details->num_keys_held;
      --i;
    }
  }
}

Window::Window(
  int initial_width,
  int initial_height,
  std::string_view title,
  bool resizable)
{
  oc_assert(0 < initial_width);
  oc_assert(0 < initial_height);

  details = new Window::WindowDetails();

  details->x11_display = XOpenDisplay(nullptr);
  details->screen = DefaultScreen(details->x11_display);

  ::Window root = RootWindow(details->x11_display, details->screen);

  XSetWindowAttributes swa = {};
  swa.event_mask = KeyPressMask
                 | KeyReleaseMask
                 | ButtonPressMask
                 | ButtonReleaseMask
                 | PointerMotionMask
                 | StructureNotifyMask; // resize

  details->window = XCreateWindow(
    details->x11_display,
    root,
    10, 10,
    uint32_t(initial_width), uint32_t(initial_height),
    0,
    CopyFromParent,
    InputOutput,
    CopyFromParent,
    CWEventMask,
    &swa);

  // Because XStoreName doesn't support UTF8...
  Xutf8SetWMProperties(details->x11_display, details->window, title.data(), nullptr, nullptr, 0, nullptr, nullptr, nullptr);

  details->wm_delete_window = XInternAtom(details->x11_display, "WM_DELETE_WINDOW", 0);
  XSetWMProtocols(details->x11_display, details->window, &details->wm_delete_window, 1);

  if (!resizable)
  {
    details->hints.flags      = PMinSize | PMaxSize;
    details->hints.min_width  = initial_width;
    details->hints.max_width  = initial_width;
    details->hints.min_height = initial_height;
    details->hints.max_height = initial_height;
    XSetWMNormalHints(details->x11_display, details->window, &details->hints);
  }

  XMapWindow(details->x11_display, details->window);

  XIM input_method = XOpenIM(details->x11_display, nullptr, nullptr, nullptr);
  details->input_context = XCreateIC(input_method, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, details->window, nullptr);
  XSetICFocus(details->input_context);

  // ----------------------- EGL ----------------------------

  details->egl_display = eglGetDisplay((EGLNativeDisplayType)details->x11_display);
  oc_assert(details->egl_display);
  check_egl_error();

  EGLint version[2];
  eglInitialize(details->egl_display, &version[0], &version[1]);
  check_egl_error();

  eglBindAPI(EGL_OPENGL_API);
  check_egl_error();

  if (!opengl.initialized)
  {
    opengl.initialized = true;
    opengl.glBlitFramebuffer        = (GlBlitFramebuffer)eglGetProcAddress("glBlitFramebuffer");
    opengl.glGenFramebuffers        = (GlGenFramebuffers)eglGetProcAddress("glGenFramebuffers");
    opengl.glBindFramebuffer        = (GlBindFramebuffer)eglGetProcAddress("glBindFramebuffer");
    opengl.glFramebufferTexture2D   = (GlFramebufferTexture2D)eglGetProcAddress("glFramebufferTexture2D");
    opengl.glCheckFramebufferStatus = (GlCheckFramebufferStatus)eglGetProcAddress("glCheckFramebufferStatus");
    opengl.glGenBuffers             = (GlGenBuffers)eglGetProcAddress("glGenBuffers");
    opengl.glBindBuffer             = (GlBindBuffer)eglGetProcAddress("glBindBuffer");
    opengl.glMapBuffer         = (GlMapBuffer)eglGetProcAddress("glMapBuffer");
    opengl.glUnmapBuffer       = (GlUnmapBuffer)eglGetProcAddress("glUnmapBuffer");
    opengl.glBufferData        = (GlBufferData)eglGetProcAddress("glBufferData");

    oc_assert(opengl.glBlitFramebuffer);
    oc_assert(opengl.glGenFramebuffers);
    oc_assert(opengl.glBindFramebuffer);
    oc_assert(opengl.glFramebufferTexture2D);
    oc_assert(opengl.glCheckFramebufferStatus);
    oc_assert(opengl.glGenBuffers);
    oc_assert(opengl.glBindBuffer);
    oc_assert(opengl.glMapBuffer);
    oc_assert(opengl.glUnmapBuffer);
    oc_assert(opengl.glBufferData);
  }

  int config_attribs[] =
  {
    EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
    EGL_CONFORMANT,        EGL_OPENGL_BIT,
    EGL_CONFIG_CAVEAT,     EGL_NONE,
    EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,

    EGL_RED_SIZE  , 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE , 8,

    EGL_NONE
  };

  int config_count;
  //eglGetConfigs(details->egl_display, nullptr, 0, &config_count);
  eglChooseConfig(details->egl_display, config_attribs, nullptr, 1, &config_count);
  check_egl_error();

  oc_assert(0 < config_count);

  EGLConfig *configs = new EGLConfig[size_t(config_count)];
  //eglGetConfigs(details->egl_display, configs, config_count, &config_count);
  eglChooseConfig(details->egl_display, config_attribs, configs, config_count, &config_count);
  check_egl_error();

  // TODO: chose config
  EGLConfig config = configs[0];
  delete[] configs;

  int surface_attribs[] = {
    EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB, // or _SRGB, idk because my VM ignores this...
    EGL_RENDER_BUFFER, EGL_SINGLE_BUFFER,
    EGL_NONE,
  };
  details->egl_surface = eglCreateWindowSurface(details->egl_display, config, details->window, surface_attribs);
  check_egl_error();

  int context_attribs[] = {
    EGL_CONTEXT_MAJOR_VERSION, 3,
    EGL_CONTEXT_MINOR_VERSION, 0,
    EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
    EGL_NONE
  };

  EGLContext context = eglCreateContext(details->egl_display, config, EGL_NO_CONTEXT, context_attribs);
  XSync(details->x11_display, 0);
  eglMakeCurrent(details->egl_display, details->egl_surface, details->egl_surface, context);

  // This call fixes the broken color in Linux, but unfortunately causes a black screen in my virtual machine.
  glEnable(GL_FRAMEBUFFER_SRGB);
  check_gl_error();

  glViewport(0, 0, initial_width, initial_height);
  check_gl_error();

  uint32_t texture = 0;
  glGenTextures(1, &texture);
  check_gl_error();
  glBindTexture(GL_TEXTURE_2D, texture);
  check_gl_error();

  opengl.glGenFramebuffers(1, &details->fbo);
  check_gl_error();
  opengl.glBindFramebuffer(GL_READ_FRAMEBUFFER, details->fbo);
  check_gl_error();
  opengl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  check_gl_error();
  opengl.glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
  check_gl_error();

  // upload some dummy stuff to the texture, this way it has a size and
  // should satisfy the fbo completeness check below.
  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA32F,
    initial_width,
    initial_height,
    0,
    GL_RGBA,
    GL_FLOAT,
    nullptr);
  check_gl_error();

  int draw_fb_complete = opengl.glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
  oc_assert(GL_FRAMEBUFFER_COMPLETE == draw_fb_complete);

  int read_fb_complete = opengl.glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
  oc_assert(GL_FRAMEBUFFER_COMPLETE == read_fb_complete);

  eglSwapInterval(details->egl_display, 1);
  check_gl_error();

  size_t f32_size = size_t(initial_width * initial_height) * sizeof(Pixel);

  opengl.glGenBuffers(1, &details->pixelbuffer);
  check_gl_error();

  opengl.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, details->pixelbuffer);
  check_gl_error();
  opengl.glBufferData(GL_PIXEL_UNPACK_BUFFER, f32_size, nullptr, GL_STREAM_DRAW);
  check_gl_error();
  void *pixelbuffer_memory = opengl.glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_READ_WRITE);
  check_gl_error();

  details->backbuffer_stride = initial_width;
  backbuffer_width           = initial_width;
  backbuffer_height          = initial_height;

  details->backbuffer = (Pixel *)pixelbuffer_memory;
}

Window::~Window()
{
  // TODO: deallocate the buffers and the window
}

void Window::close()
{
  
}

void Window::set_size(int width, int height)
{
  XResizeWindow(details->x11_display, details->window, uint32_t(width), uint32_t(height));
}

static oc::KeyCode x11_keysym_to_keycode(KeySym x11key)
{
  switch (x11key)
  {
    case XK_space:      return oc::KeyCode::Key_Space;
    case XK_exclam:     return oc::KeyCode::Key_Exclamation;
    case XK_quotedbl:   return oc::KeyCode::Key_Quote;
    case XK_numbersign: return oc::KeyCode::Key_Hash;
    case XK_asterisk:   return oc::KeyCode::Key_Star;
    case XK_plus:       return oc::KeyCode::Key_Plus;
    case XK_comma:      return oc::KeyCode::Key_Comma;
    case XK_minus:      return oc::KeyCode::Key_Minus;
    case XK_period:     return oc::KeyCode::Key_Period;
    case XK_slash:      return oc::KeyCode::Key_Slash;
    case XK_0:          return oc::KeyCode::Key_0;
    case XK_1:          return oc::KeyCode::Key_1;
    case XK_2:          return oc::KeyCode::Key_2;
    case XK_3:          return oc::KeyCode::Key_3;
    case XK_4:          return oc::KeyCode::Key_4;
    case XK_5:          return oc::KeyCode::Key_5;
    case XK_6:          return oc::KeyCode::Key_6;
    case XK_7:          return oc::KeyCode::Key_7;
    case XK_8:          return oc::KeyCode::Key_8;
    case XK_9:          return oc::KeyCode::Key_9;
    case XK_question:   return oc::KeyCode::Key_Question;
    case XK_a:          return oc::KeyCode::Key_A;
    case XK_b:          return oc::KeyCode::Key_B;
    case XK_c:          return oc::KeyCode::Key_C;
    case XK_d:          return oc::KeyCode::Key_D;
    case XK_e:          return oc::KeyCode::Key_E;
    case XK_f:          return oc::KeyCode::Key_F;
    case XK_g:          return oc::KeyCode::Key_G;
    case XK_h:          return oc::KeyCode::Key_H;
    case XK_i:          return oc::KeyCode::Key_I;
    case XK_j:          return oc::KeyCode::Key_J;
    case XK_k:          return oc::KeyCode::Key_K;
    case XK_l:          return oc::KeyCode::Key_L;
    case XK_m:          return oc::KeyCode::Key_M;
    case XK_n:          return oc::KeyCode::Key_N;
    case XK_o:          return oc::KeyCode::Key_O;
    case XK_p:          return oc::KeyCode::Key_P;
    case XK_q:          return oc::KeyCode::Key_Q;
    case XK_r:          return oc::KeyCode::Key_R;
    case XK_s:          return oc::KeyCode::Key_S;
    case XK_t:          return oc::KeyCode::Key_T;
    case XK_u:          return oc::KeyCode::Key_U;
    case XK_v:          return oc::KeyCode::Key_V;
    case XK_w:          return oc::KeyCode::Key_W;
    case XK_x:          return oc::KeyCode::Key_X;
    case XK_y:          return oc::KeyCode::Key_Y;
    case XK_z:          return oc::KeyCode::Key_Z;
    case XK_F1:         return oc::KeyCode::Key_F1;
    case XK_F2:         return oc::KeyCode::Key_F2;
    case XK_F3:         return oc::KeyCode::Key_F3;
    case XK_F4:         return oc::KeyCode::Key_F4;
    case XK_F5:         return oc::KeyCode::Key_F5;
    case XK_F6:         return oc::KeyCode::Key_F6;
    case XK_F7:         return oc::KeyCode::Key_F7;
    case XK_F8:         return oc::KeyCode::Key_F8;
    case XK_F9:         return oc::KeyCode::Key_F9;
    case XK_F10:        return oc::KeyCode::Key_F10;
    case XK_F11:        return oc::KeyCode::Key_F11;
    case XK_F12:        return oc::KeyCode::Key_F12;
    case XK_Tab:        return oc::KeyCode::Key_Tab;
    case XK_BackSpace:  return oc::KeyCode::Key_Backspace;
    case XK_Delete:     return oc::KeyCode::Key_Delete;
    case XK_Return:     return oc::KeyCode::Key_Enter;
    case XK_Shift_L:    return oc::KeyCode::Key_Shift;
    case XK_Shift_R:    return oc::KeyCode::Key_Shift;
    case XK_Control_L:  return oc::KeyCode::Key_Control;
    case XK_Control_R:  return oc::KeyCode::Key_Control;
    case XK_Alt_L:      return oc::KeyCode::Key_Alt;
    case XK_Alt_R:      return oc::KeyCode::Key_Alt;
    case XK_Escape:     return oc::KeyCode::Key_Escape;
    case XK_Up:         return oc::KeyCode::Key_Arrow_Up;
    case XK_Down:       return oc::KeyCode::Key_Arrow_Down;
    case XK_Left:       return oc::KeyCode::Key_Arrow_Left;
    case XK_Right:      return oc::KeyCode::Key_Arrow_Right;
  }
  return oc::KeyCode::Unknown;
}

static oc::KeyCode x11_button_to_keycode(unsigned int button)
{
  switch (button)
  {
    case 1: return oc::KeyCode::Mouse_1;
    case 2: return oc::KeyCode::Mouse_3;
    case 3: return oc::KeyCode::Mouse_2;
    case 8: return oc::KeyCode::Mouse_4;
    case 9: return oc::KeyCode::Mouse_5;
  }
  return oc::KeyCode::Unknown;
}

InputEvent Window::next_event(bool blocking)
{
  // Loop in case we receive an event we don't understand.
  // When we get no event and don't block, or we get an event we
  // understand, this function breaks the loop by returning.
  while (true)
  {
    if (!XPending(details->x11_display))
    {
      if (details->minimized)
      {
        if (!blocking)
        {
          return {EventType::None};
        }
      }
      else
      {
        // Unfortunately, X11 doesn't give us a way to delay the drawing
        // as much as possible to reduce input-lag.
        return {EventType::Draw};
      }
    }

    XEvent current_event;
    XNextEvent(details->x11_display, &current_event);
    if (current_event.type == ClientMessage)
    {
      if (details->wm_delete_window == uint32_t(current_event.xclient.data.l[0]))
      {
        return {EventType::Close};
      }
    }

    if (current_event.type == ConfigureNotify) // resize & more
    {
      int width  = current_event.xconfigure.width;
      int height = current_event.xconfigure.height;
      details->minimized = false;
      if (0 == width && 0 == height)
      {
        details->minimized = true;
      }
      else if (width != backbuffer_width || height != backbuffer_height)
      {
        int old_width  = backbuffer_width;
        int old_height = backbuffer_height;

        opengl.glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        check_gl_error();

        size_t f32_size = size_t(width * height) * sizeof(Pixel);
        opengl.glBufferData(GL_PIXEL_UNPACK_BUFFER, f32_size, nullptr, GL_STREAM_DRAW);
        check_gl_error();

        void *data = opengl.glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_READ_WRITE);
        check_gl_error();

        details->backbuffer = (Pixel *)data;
        details->backbuffer_stride  = width;
        backbuffer_width  = width;
        backbuffer_height = height;

        return {
          .resize = {
            .header = {
              .type = EventType::Resize,
              .time = ocTime::now() // xcofigure doesn't have a time...
            },
            .old_width     = old_width,
            .old_height    = old_height,
            .old_scaling   = 1.0f,
            .old_minimized = false,
            .new_width     = width,
            .new_height    = height,
            .new_scaling   = 1.0f,
            .new_minimized = false // TODO: handle minization
          }
        };
      }
    }

    if (current_event.type == KeyPress)
    {
      KeyCode key = x11_keysym_to_keycode(XLookupKeysym(&current_event.xkey, 0));

      auto time = current_event.xkey.time;

      push_key(details, key);

      return {
        .key = {
          .header = {
            .type = EventType::Key,
            .time = ocTime::milliseconds(int64_t(time))
          },
          .code        = key,
          .down        = true
        }
      };
    }
    if (current_event.type == KeyRelease)
    {
      KeyCode key = x11_keysym_to_keycode(XLookupKeysym(&current_event.xkey, 0));
      pop_key(details, key);
      return {
        .key = {
          .header = {
            .type = EventType::Key,
            .time = ocTime::milliseconds(int64_t(current_event.xkey.time))
          },
          .code        = key,
          .down        = false
        }
      };
    }
    if (current_event.type == ButtonPress)
    {
      switch (current_event.xbutton.button)
      {
        case 1: // Mouse Left
        case 2: // Mouse Center
        case 3: // Mouse Right
        case 8: // Mouse Button 4
        case 9: // Mouse Button 5
        {
          KeyCode key = x11_button_to_keycode(current_event.xbutton.button);
          push_key(details, key);
          return {
            .key = {
              .header = {
                .type = EventType::Key,
                .time = ocTime::milliseconds(int64_t(current_event.xbutton.time))
              },
              .code        = key,
              .down        = true
            }
          };
        } break;
        case 4: // Mouse Scroll Up
        {
          if (current_event.xbutton.state & ControlMask)
          {
            return {
              .zoom = {
                .header = {
                  .type = EventType::Zoom,
                  .time = ocTime::milliseconds(int64_t(current_event.xbutton.time))
                },
                .center_x = mouse_x,
                .center_y = mouse_y,
                .factor = 1.1f
              }
            };
          }
          else
          {
            return {
              .scroll = {
                .header = {
                  .type = EventType::Scroll,
                  .time = ocTime::milliseconds(int64_t(current_event.xbutton.time))
                },
                .x = (current_event.xbutton.state & ShiftMask) ? 10.0f :  0.0f,
                .y = (current_event.xbutton.state & ShiftMask) ?  0.0f : 10.0f,
                .mouse_x = mouse_x,
                .mouse_y = mouse_y
              }
            };
          }
        } break;
        case 5: // Mouse Scroll Down
        {
          if (current_event.xbutton.state & ControlMask)
          {
            return {
              .zoom = {
                .header = {
                  .type = EventType::Zoom,
                  .time = ocTime::milliseconds(int64_t(current_event.xbutton.time))
                },
                .center_x = mouse_x,
                .center_y = mouse_y,
                .factor = 1.0f/1.1f
              }
            };
          }
          else
          {
            return {
              .scroll = {
                .header = {
                  .type = EventType::Scroll,
                  .time = ocTime::milliseconds(int64_t(current_event.xbutton.time))
                },
                .x = (current_event.xbutton.state & ShiftMask) ? -10.0f :   0.0f,
                .y = (current_event.xbutton.state & ShiftMask) ?   0.0f : -10.0f,
                .mouse_x = mouse_x,
                .mouse_y = mouse_y
              }
            };
          }
        } break;
      }
    }
    if (current_event.type == ButtonRelease)
    {
      switch (current_event.xbutton.button)
      {
        case 1: // Mouse Left
        case 2: // Mouse Center
        case 3: // Mouse Right
        case 8: // Mouse Button 4
        case 9: // Mouse Button 5
        {
          KeyCode key = x11_button_to_keycode(current_event.xbutton.button);
          pop_key(details, key);
          return {
            .key = {
              .header = {
                .type = EventType::Key,
                .time = ocTime::milliseconds(int64_t(current_event.xbutton.time))
              },
              .code        = key,
                .down        = false
            }
          };
        } break;
      }
    }
    if (current_event.type == MotionNotify)
    {
      float old_x = mouse_x;
      float old_y = mouse_y;
      mouse_x = float(current_event.xmotion.x);
      mouse_y = float(current_event.xmotion.y);
      return {
        .pointer = {
          .header = {
            .type = EventType::Pointer,
            .time = ocTime::milliseconds(int64_t(current_event.xmotion.time))
          },
          .old_x = old_x,
          .old_y = old_y,
          .new_x = mouse_x,
          .new_y = mouse_y,
          .dragged = (0 != (current_event.xmotion.state & 0x0700))
        }
      };
    }
  }
}

bool Window::is_key_down(KeyCode keycode) const
{
  for (int i = 0; i < details->num_keys_held; ++i)
  {
    if (keycode == details->keys_held[i])
    {
      return true;
    }
  }
  return false;
}

void Window::draw_pixel(int x, int y, const Color& color)
{
  if (0 <= x && 0 <= y && x < backbuffer_width && y < backbuffer_height)
    details->backbuffer[y * details->backbuffer_stride + x].color = color;
}

void Window::draw_pixel(int x, int y, const Color& color, float opacity)
{
  if (0 <= x && 0 <= y && x < backbuffer_width && y < backbuffer_height)
  {
    Color c = details->backbuffer[y * details->backbuffer_stride + x].color;
    details->backbuffer[y * details->backbuffer_stride + x].color = c * (1.0f - opacity) + color;
  }
}

void Window::commit()
{
  if (details->minimized) return;

  glViewport(0, 0, backbuffer_width, backbuffer_height);
  check_gl_error();

  opengl.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, details->pixelbuffer);
  check_gl_error();

  opengl.glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
  check_gl_error();

  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA32F,
    backbuffer_width,
    backbuffer_height,
    0,
    GL_RGBA,
    GL_FLOAT,
    0);
  check_gl_error();

  opengl.glBlitFramebuffer(
    0, backbuffer_height, backbuffer_width, 0,
    0, 0, backbuffer_width, backbuffer_height,
    GL_COLOR_BUFFER_BIT,
    GL_NEAREST);
  check_gl_error();

  eglSwapBuffers(details->egl_display, details->egl_surface);

  details->backbuffer = (Pixel *)opengl.glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_READ_WRITE);
  check_gl_error();
}

void Window::fill(const Color& color)
{
  Pixel *first_line = details->backbuffer;
  for (int x = 0; x < backbuffer_width; ++x)
  {
    first_line[x] = {color, 1.0f};
  }
  Pixel *line = first_line;
  for (int y = 1; y < backbuffer_height; ++y)
  {
    line += details->backbuffer_stride;
    memcpy(line, first_line, (size_t)backbuffer_width * sizeof(Pixel));
  }
}

void Window::fill_rect(
  int x0, int y0,
  int x1, int y1,
  const Color& color)
{
  int start_x = std::clamp(std::min(x0, x1), 0, backbuffer_width);
  int start_y = std::clamp(std::min(y0, y1), 0, backbuffer_height);
  int end_x   = std::clamp(std::max(x0, x1), 0, backbuffer_width)  - start_x;
  int end_y   = std::clamp(std::max(y0, y1), 0, backbuffer_height) - start_y;
  Pixel *first_line = &details->backbuffer[start_y * details->backbuffer_stride + start_x];
  for (int x = 0; x < end_x; ++x)
  {
    first_line[x] = {color, 1.0f};
  }
  Pixel *line = first_line;
  for (int y = 1; y < end_y; ++y)
  {
    line += details->backbuffer_stride;
    memcpy(line, first_line, size_t(end_x) * sizeof(Pixel));
  }
}

} // namespace oc
