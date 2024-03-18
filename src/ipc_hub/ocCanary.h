#pragma once

template<typename T>
class ocCanary
{
private:
  volatile T *_suspect;
  T           _reference;

public:
  ocCanary() = default;

  ocCanary(T *suspect, T reference)
  {
    init(suspect, reference);
  }

  void init(T *suspect, T reference)
  {
    _suspect = suspect;
    _reference = reference;
    *_suspect = _reference;
  }

  bool check()
  {
    return *_suspect == _reference;
  }
};
