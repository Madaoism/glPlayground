/*
 * Window.cpp
 *
 *  Created on: Feb 25, 2020
 *      Author: ycai
 */

#include "Window.h"
#include <stdexcept>
#include "../utils/Logger.h"

void Window::onKeyCb(GLFWwindow* w, int key, int scancode, int action, int mods)
{
  static_cast<Window*>(glfwGetWindowUserPointer(w))->onKey(key, scancode, action, mods);
}

void Window::onCursorPosCb(GLFWwindow* w, double xPos, double yPos)
{
  static_cast<Window*>(glfwGetWindowUserPointer(w))->onCursorPos(xPos, yPos);
}

void Window::onMouseButtonCb(GLFWwindow* w, int key, int action, int mods)
{
  static_cast<Window*>(glfwGetWindowUserPointer(w))->onMouseButton(key, action, mods);
}

void Window::onResizeCb(GLFWwindow* w, int width, int height)
{
  static_cast<Window*>(glfwGetWindowUserPointer(w))->onResize(width, height);
}

Window::Window(int width, int height, std::string title):
  _mDefaultWidth(width), _mDefaultHeight(height), _mTitle(title), _mWindow(NULL)
{}

Window::~Window()
{
  destroy();
}

void APIENTRY glDebugOutput(GLenum source,
  GLenum type,
  GLuint id,
  GLenum severity,
  GLsizei length,
  const GLchar* message,
  const void* userParam)
{
  // ignore non-significant error/warning codes
  if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

  std::string strSource;
  std::string strType;

  switch (source)
  {
  case GL_DEBUG_SOURCE_API:             strSource = "Source: API"; break;
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   strSource = "Source: Window System"; break;
  case GL_DEBUG_SOURCE_SHADER_COMPILER: strSource = "Source: Shader Compiler"; break;
  case GL_DEBUG_SOURCE_THIRD_PARTY:     strSource = "Source: Third Party"; break;
  case GL_DEBUG_SOURCE_APPLICATION:     strSource = "Source: Application"; break;
  case GL_DEBUG_SOURCE_OTHER:           strSource = "Source: Other"; break;
  }

  switch (type)
  {
  case GL_DEBUG_TYPE_ERROR:               strType = "Type: Error"; break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: strType = "Type: Deprecated Behaviour"; break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  strType = "Type: Undefined Behaviour"; break;
  case GL_DEBUG_TYPE_PORTABILITY:         strType = "Type: Portability"; break;
  case GL_DEBUG_TYPE_PERFORMANCE:         strType = "Type: Performance"; break;
  case GL_DEBUG_TYPE_MARKER:              strType = "Type: Marker"; break;
  case GL_DEBUG_TYPE_PUSH_GROUP:          strType = "Type: Push Group"; break;
  case GL_DEBUG_TYPE_POP_GROUP:           strType = "Type: Pop Group"; break;
  case GL_DEBUG_TYPE_OTHER:               strType = "Type: Other"; break;
  }

  if (severity == GL_DEBUG_SEVERITY_HIGH) {
    Log.print<Severity::error>("OpenGL Debug Context values: ", strSource, " - ", strType);
    Log.print<Severity::error>("Debug message (", id, "): ", message);
  }
  else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
    Log.print<Severity::warning>("OpenGL Debug Context values: ", strSource, " - ", strType);
    Log.print<Severity::warning>("Debug message (", id, "): ", message);
  }
  else if (severity == GL_DEBUG_SEVERITY_LOW) {
    Log.print<Severity::info>("OpenGL Debug Context values: ", strSource, " - ", strType);
    Log.print<Severity::info>("Debug message (", id, "): ", message);
  }
  else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
    Log.print<Severity::debug>("OpenGL Debug Context values: ", strSource, " - ", strType);
    Log.print<Severity::debug>("Debug message (", id, "): ", message);
  }
}

void Window::setupFrameBuffer(int width, int height) {
  GLuint prevFb = fbo;
  GLuint prevColor = color;
  GLuint prevDepth = depthStencil;

  glGenTextures(1, &color);
  glBindTexture(GL_TEXTURE_2D, color);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_SRGB8_ALPHA8, width, height);
  glBindTexture(GL_TEXTURE_2D, 0);

  glGenRenderbuffers(1, &depthStencil);
  glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH32F_STENCIL8, width, height);
  glGenRenderbuffers(GL_RENDERBUFFER, 0);

  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencil);
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    Log.print<Severity::error>("glCheckFramebufferStatus: ", status);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if (prevFb) {
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &color);
    glDeleteRenderbuffers(1, &prevDepth);
  }
}

void Window::initialize(bool resizable)
{
  static bool firstTime = true;
  bool initGlfwAndGlad = firstTime;

  glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

  _mWindow = glfwCreateWindow(_mDefaultWidth, _mDefaultHeight, _mTitle.c_str(), NULL, NULL);
  if (_mWindow == NULL)
  {
    throw std::runtime_error("Failed to create GLFW window");
  }
  glfwSetWindowUserPointer(_mWindow, this);

  glfwSetKeyCallback(_mWindow, onKeyCb);
  glfwSetCursorPosCallback(_mWindow, onCursorPosCb);
  glfwSetMouseButtonCallback(_mWindow, onMouseButtonCb);
  glfwSetFramebufferSizeCallback(_mWindow, onResizeCb);

  if (initGlfwAndGlad)
  {
    firstTime = false;
    setContextCurrent();
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
      glfwTerminate();
      throw std::runtime_error("Failed to initialize GLAD");
    }
  }

#ifndef NDEBUG
  GLint flags;
  glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
  if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
  {
    // initialize debug output 
    Log.print<Severity::info>("Enabling Debug Context!");
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugOutput, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
  }
#endif

  Log.print<Severity::info>("OpenGL version supported by this platform: ", glGetString(GL_VERSION));

  // reverse z buffer code
  glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
  setupFrameBuffer(_mDefaultWidth, _mDefaultHeight);
}

void Window::addObservable(WindowObservable* observable)
{
  observables.insert(observable);
}

void Window::removeObservable(WindowObservable* observable)
{
  observables.erase(observable);
}

void Window::onKey(int key, int scancode, int action, int mods)
{
  for (auto it : observables)
  {
    it->onKey(key, scancode, action, mods);
  }
}

void Window::onCursorPos(double xPos, double yPos)
{
  for (auto it : observables)
  {
    it->onCursorPos(xPos, yPos);
  }
}

void Window::onMouseButton(int key, int action, int mods)
{
  for (auto it : observables)
  {
    it->onMouseButton(key, action, mods);
  }
}

void Window::onResize(int width, int height)
{
  setupFrameBuffer(width, height);
  for (auto it : observables)
  {
    it->onResize(width, height);
  }
}

void Window::destroy()
{
  if (_mWindow != NULL)
  {
    glfwDestroyWindow(_mWindow);
    _mWindow = NULL;
  }
}

void Window::nextFrame()
{
  glfwSwapBuffers(_mWindow);
  glfwPollEvents();
}

bool Window::shouldClose() const
{
  return glfwWindowShouldClose(_mWindow);
}

void Window::close()
{
  glfwSetWindowShouldClose(_mWindow, GLFW_TRUE);
}

void Window::setContextCurrent()
{
  glfwMakeContextCurrent(_mWindow);
}

void Window::setCursorMode(int mode)
{
  glfwSetInputMode(_mWindow, GLFW_CURSOR, mode);
}

bool Window::isContextCurrent() const
{
  return glfwGetCurrentContext() == _mWindow;
}

glm::dvec2 Window::getCursorPosition()
{
  glm::dvec2 ret;
  glfwGetCursorPos(_mWindow, &ret.x, &ret.y);
  return ret;
}