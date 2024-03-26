#include "ocWindow.h"

#include "ocAssert.h"

#include <unistd.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// Fuck you, X
#undef None

#include <cstring>

namespace oc
{

typedef GLXContext (*GlXCreateContextAttribsARB)(
  Display *display,
  GLXFBConfig fb_conf,
  GLXContext shared_context,
  int direct,
  const int32_t *attrib_list);

typedef int (*GlXSwapIntervalEXT)(
  Display    *display,
  GLXDrawable drawable,
  int32_t     interval);

typedef void (*GlBlitFramebuffer)(
  int32_t srcX0, int32_t srcY0,
  int32_t srcX1, int32_t srcY1,
  int32_t dstX0, int32_t dstY0,
  int32_t dstX1, int32_t dstY1,
  uint32_t mask,
  uint32_t filter);

typedef void (*GlBlitNamedFramebuffer)(
  uint32_t readFramebuffer,
  uint32_t drawFramebuffer,
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

typedef void (*GlNamedFramebufferTexture)(
  uint32_t framebuffer,
  uint32_t attachment,
  uint32_t texture,
  int32_t  level);

typedef int32_t (*GlCheckFramebufferStatus)(
  int32_t target);

static struct
{
  GlXSwapIntervalEXT         glXSwapIntervalEXT;
  GlXCreateContextAttribsARB glXCreateContextAttribsARB;
  GlBlitFramebuffer          glBlitFramebuffer;
  GlBlitNamedFramebuffer     glBlitNamedFramebuffer;
  GlGenFramebuffers          glGenFramebuffers;
  GlNamedFramebufferTexture  glNamedFramebufferTexture;
  GlBindFramebuffer          glBindFramebuffer;
  GlFramebufferTexture2D     glFramebufferTexture2D;
  GlCheckFramebufferStatus   glCheckFramebufferStatus;

  bool initialized = false;
} opengl;

struct ColorPlusAlpha
{
  Color color;
  float alpha;
};
static_assert(sizeof(ColorPlusAlpha) == 16);

struct WindowDetails
{
  Display *display;
  ::Window window;
  GLXContext ogl_context;
  int screen;
  Atom wm_delete_window;
  Time last_button_time[10];
  ColorPlusAlpha *backbuffer;
  int32_t  backbuffer_stride;
  int32_t  old_backbuffer_width;
  int32_t  old_backbuffer_height;
  uint32_t fbo;
  int num_keys_held;
  KeyCode keys_held[16]; // Nobody can hold more than 16 keys!
  bool resized;
};

static void check_gl_error() {
  uint32_t error = glGetError();
  if (GL_NO_ERROR != error)
  {
    const char *message = (const char *)gluErrorString(error);
    puts(message);
    oc_assert(!message);
  }
}


static void push_key(WindowDetails *details, KeyCode key)
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

static void pop_key(WindowDetails *details, KeyCode key)
{
  for (int i = 0; i < details->num_keys_held; ++i)
  {
    if (key == details->keys_held[i])
    {
      memmove(&details->keys_held[i], &details->keys_held[i + 1], sizeof(KeyCode) * (uint32_t)(15 - i));
      --details->num_keys_held;
      --i;
    }
  }
}

Window::Window(
  int initial_width,
  int initial_height,
  std::string_view title,
  bool resizable,
  bool /*auto_scaling*/)
{
  oc_assert(0 < initial_width);
  oc_assert(0 < initial_height);

  details = new WindowDetails();

  details->backbuffer_stride = initial_width;
  backbuffer_width           = initial_width;
  backbuffer_height          = initial_height;

  details->display = XOpenDisplay(nullptr);
  details->screen = DefaultScreen(details->display);

  int visual_attribs[] =
  {
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE  , GLX_RGBA_BIT,
    GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
    GLX_RED_SIZE     , 8,
    GLX_GREEN_SIZE   , 8,
    GLX_BLUE_SIZE    , 8,
    GLX_DOUBLEBUFFER , 1,
    GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB, 1,
    0
  };

  int fb_count = 0;
  GLXFBConfig *fb_confs = glXChooseFBConfig(details->display, details->screen, visual_attribs, &fb_count);
  oc_assert(fb_confs);

  GLXFBConfig fb_conf = fb_confs[0];
  XVisualInfo *vi = glXGetVisualFromFBConfig(details->display, fb_conf);
  // TODO: check that the VI has all features we want
  oc_assert(vi);

  XFree(fb_confs);

  ::Window root = RootWindow(details->display, details->screen);

  XSetWindowAttributes swa = {};
  swa.colormap   = XCreateColormap(details->display, root, vi->visual, AllocNone);
  swa.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;

  details->window = XCreateWindow(
    details->display,
    root,
    10, 10,
    (uint32_t)initial_width, (uint32_t)initial_height,
    0,
    vi->depth,
    InputOutput,
    vi->visual,
    CWColormap | CWEventMask,
    &swa);

  XFree(vi);

  XStoreName(details->display, details->window, title.data());
  XMapWindow(details->display, details->window);

  if (!opengl.initialized)
  {
    opengl.initialized = true;
    opengl.glXCreateContextAttribsARB = (GlXCreateContextAttribsARB)glXGetProcAddressARB((const unsigned char *)"glXCreateContextAttribsARB");
    opengl.glXSwapIntervalEXT         = (GlXSwapIntervalEXT)glXGetProcAddressARB((const unsigned char *)"glXSwapIntervalEXT");
    opengl.glBlitFramebuffer          = (GlBlitFramebuffer)glXGetProcAddressARB((const unsigned char *)"glBlitFramebuffer");
    opengl.glBlitNamedFramebuffer     = (GlBlitNamedFramebuffer)glXGetProcAddressARB((const unsigned char *)"glBlitNamedFramebuffer");
    opengl.glGenFramebuffers          = (GlGenFramebuffers)glXGetProcAddressARB((const unsigned char *)"glGenFramebuffers");
    opengl.glNamedFramebufferTexture  = (GlNamedFramebufferTexture)glXGetProcAddressARB((const unsigned char *)"glNamedFramebufferTexture");
    opengl.glBindFramebuffer          = (GlBindFramebuffer)glXGetProcAddressARB((const unsigned char *)"glBindFramebuffer");
    opengl.glFramebufferTexture2D     = (GlFramebufferTexture2D)glXGetProcAddressARB((const unsigned char *)"glFramebufferTexture2D");
    opengl.glCheckFramebufferStatus   = (GlCheckFramebufferStatus)glXGetProcAddressARB((const unsigned char *)"glCheckFramebufferStatus");

    oc_assert(opengl.glXCreateContextAttribsARB);
    oc_assert(opengl.glXSwapIntervalEXT);
    oc_assert(opengl.glBlitFramebuffer);
    oc_assert(opengl.glBlitNamedFramebuffer);
    oc_assert(opengl.glGenFramebuffers);
    oc_assert(opengl.glNamedFramebufferTexture);
    oc_assert(opengl.glBindFramebuffer);
    oc_assert(opengl.glFramebufferTexture2D);
    oc_assert(opengl.glCheckFramebufferStatus);
  }

  int attribs[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
    GLX_CONTEXT_MINOR_VERSION_ARB, 0,
    GLX_CONTEXT_FLAGS_ARB, 0 /*| GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB*/ | GLX_CONTEXT_DEBUG_BIT_ARB,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    0
  };

  details->ogl_context = opengl.glXCreateContextAttribsARB(details->display, fb_conf, 0, 1, attribs);
  XSync(details->display, 0);
  glXMakeCurrent(details->display, details->window, details->ogl_context);

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

  size_t num_pixels = (size_t)(details->backbuffer_stride * backbuffer_height);

  // Can't use new[] here, because we need to realloc the buffer when the window is resized.
  details->backbuffer = (ColorPlusAlpha *)malloc(num_pixels * sizeof(ColorPlusAlpha));
  for (size_t i = 0; i < num_pixels; ++i) details->backbuffer[i] = {{1.0f, 1.0f, 1.0f}, 1.0f};

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
    details->backbuffer);
  check_gl_error();

  int draw_fb_complete = opengl.glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
  oc_assert(GL_FRAMEBUFFER_COMPLETE == draw_fb_complete, draw_fb_complete);

  int read_fb_complete = opengl.glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
  oc_assert(GL_FRAMEBUFFER_COMPLETE == read_fb_complete, read_fb_complete);

  // doesn't work in my VM at the moment, which makes me super sad
  // glEnable(GL_FRAMEBUFFER_SRGB);
  // check_gl_error();

  opengl.glXSwapIntervalEXT(details->display, details->window, 1);
  check_gl_error();

  details->wm_delete_window = XInternAtom(details->display, "WM_DELETE_WINDOW", 0);
  XSetWMProtocols(details->display, details->window, &details->wm_delete_window, 1);

  if (!resizable)
  {
    XSizeHints hints = {};
    hints.flags      = PMinSize | PMaxSize;
    hints.min_width  = initial_width;
    hints.max_width  = initial_width;
    hints.min_height = initial_height;
    hints.max_height = initial_height;
    XSetWMNormalHints(details->display, details->window, &hints);
  }

  mouse_x = 0.0f;
  mouse_y = 0.0f;
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
  glXMakeCurrent(details->display, details->window, details->ogl_context);
  check_gl_error();
  backbuffer_width  = width;
  backbuffer_height = height;
  details->backbuffer_stride = backbuffer_width;
  size_t num_pixels  = (size_t)(details->backbuffer_stride * backbuffer_height);
  details->backbuffer = (ColorPlusAlpha *)realloc(details->backbuffer, num_pixels * sizeof(ColorPlusAlpha));
  glViewport(0, 0, width, height);
  check_gl_error();

  XResizeWindow(details->display, details->window, (uint32_t)width, (uint32_t)height);
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
    if (details->resized)
    {
      details->resized = false;
      return {
        .resize = {
          .header = {
            .type = EventType::Resize,
            .time = 0LL // TODO: get current tile
          },
          .old_width   = details->old_backbuffer_width,
          .old_height  = details->old_backbuffer_height,
          .old_scaling = 1.0f,
          .new_width   = backbuffer_width,
          .new_height  = backbuffer_height,
          .new_scaling = 1.0f,
        }
      };
    }

    if (!blocking && !XPending(details->display))
    {
      return {EventType::Draw};
    }

    XEvent current_event; 
    XNextEvent(details->display, &current_event);
    if (current_event.type == ClientMessage)
    {
      if (details->wm_delete_window == (uint32_t)current_event.xclient.data.l[0])
      {
        return {EventType::Close};
      }
    }
    if (current_event.type == KeyPress)
    {
      KeyCode key = x11_keysym_to_keycode(XLookupKeysym(&current_event.xkey, 0));
      push_key(details, key);
      return {
        .key = {
          .header = {
            .type = EventType::Key,
            .time = (int64_t)current_event.xkey.time * 1000000LL
          },
          .code        = key,
          .down        = true,
          .doubleclick = false
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
            .time = (int64_t)current_event.xkey.time * 1000000LL
          },
          .code = key,
          .down = false,
          .doubleclick = false
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
          Time last_time = details->last_button_time[current_event.xbutton.button];
          details->last_button_time[current_event.xbutton.button] = current_event.xbutton.time;
          KeyCode key = x11_button_to_keycode(current_event.xbutton.button);
          if (KeyCode::Unknown == key) continue;
          push_key(details, key);
          return {
            .key = {
              .header = {
                .type = EventType::Key,
                .time = (int64_t)current_event.xbutton.time * 1000000LL
              },
              .code        = key,
              .down        = true,
              .doubleclick = current_event.xbutton.time - last_time <= 1000
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
                  .time = (int64_t)current_event.xbutton.time * 1000000LL
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
                  .time = (int64_t)current_event.xbutton.time * 1000000LL
                },
                .x = (current_event.xbutton.state & ShiftMask) ? 10.0f :  0.0f,
                .y = (current_event.xbutton.state & ShiftMask) ?  0.0f : 10.0f
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
                  .time = (int64_t)current_event.xbutton.time * 1000000LL
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
                  .time = (int64_t)current_event.xbutton.time * 1000000LL
                },
                .x = (current_event.xbutton.state & ShiftMask) ? -10.0f :   0.0f,
                .y = (current_event.xbutton.state & ShiftMask) ?   0.0f : -10.0f
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
          if (KeyCode::Unknown == key) continue;
          pop_key(details, key);
          return {
            .key = {
              .header = {
                .type = EventType::Key,
                .time = (int64_t)current_event.xbutton.time * 1000000LL
              },
              .code        = key,
              .down        = false,
              .doubleclick = false
            }
          };
        } break;
      }
    }
    if (current_event.type == MotionNotify)
    {
      float old_x = mouse_x;
      float old_y = mouse_y;
      mouse_x = (float)current_event.xmotion.x;
      mouse_y = (float)current_event.xmotion.y;
      return {
        .pointer = {
          .header = {
            .type = EventType::Pointer,
            .time = (int64_t)current_event.xmotion.time * 1000000LL
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
  ::Window bla_window;
  int bla;
  int32_t window_width, window_height;
  XGetGeometry(
    details->display,
    details->window,
    &bla_window,
    &bla, &bla,
    (uint32_t *)&window_width, (uint32_t *)&window_height,
    (uint32_t *)&bla, (uint32_t *)&bla);

  if (window_width  != backbuffer_width ||
      window_height != backbuffer_height)
  {
    details->resized = true;
    details->old_backbuffer_width  = backbuffer_width;
    details->old_backbuffer_height = backbuffer_height;
  }

  glXMakeCurrent(details->display, details->window, details->ogl_context);
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
    details->backbuffer);
  check_gl_error();

  opengl.glBlitFramebuffer(
      0, backbuffer_height, backbuffer_width, 0,
      0, 0, backbuffer_width, backbuffer_height,
      GL_COLOR_BUFFER_BIT,
      GL_NEAREST);
    check_gl_error();

  glXSwapBuffers(details->display, details->window);
  check_gl_error();

  if (details->resized)
  {
    backbuffer_width  = window_width;
    backbuffer_height = window_height;
    details->backbuffer_stride = backbuffer_width;
    size_t num_pixels  = (size_t)(details->backbuffer_stride * backbuffer_height);
    details->backbuffer = (ColorPlusAlpha *)realloc(details->backbuffer, num_pixels * sizeof(ColorPlusAlpha));
    glViewport(0, 0, window_width, window_height);
    check_gl_error();
  }
}

void Window::fill(const Color& color)
{
  ColorPlusAlpha *first_line = details->backbuffer;
  for (int x = 0; x < backbuffer_width; ++x)
  {
    first_line[x] = {color, 1.0f};
  }
  ColorPlusAlpha *line = first_line;
  for (int y = 1; y < backbuffer_height; ++y)
  {
    line += details->backbuffer_stride;
    memcpy(line, first_line, (size_t)backbuffer_width * sizeof(ColorPlusAlpha));
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
  ColorPlusAlpha *first_line = &details->backbuffer[start_y * details->backbuffer_stride + start_x];
  for (int x = 0; x < end_x; ++x)
  {
    first_line[x] = {color, 1.0f};
  }
  ColorPlusAlpha *line = first_line;
  for (int y = 1; y < end_y; ++y)
  {
    line += details->backbuffer_stride;
    memcpy(line, first_line, (size_t)end_x * sizeof(ColorPlusAlpha));
  }
}

} // namespace oc
