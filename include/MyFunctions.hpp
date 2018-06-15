#ifndef MYFUNCTIONS_HPP
#define MYFUNCTIONS_HPP 1

template <class T>
void DelPointer(T *&pointer)
{
  delete pointer;
  pointer = nullptr;
}

#endif
