#ifndef LOWLEVELIO_HPP
#define LOWLEVELIO_HPP
#include <system_error>
#include <unistd.h>
using namespace std;

class LLIOError : public system_error
{
public:
  explicit LLIOError(int ev);
};
class LLIOEOF : public exception
{
public:
  explicit LLIOEOF();
};
class  LowLevelIO
{
public:
  static void xpread(int fd, void *buf, size_t count,
                     off_t offset) throw(LLIOError, LLIOEOF);
  static void xpwrite(int fd, void *buf, size_t count,
                      off_t offset) throw(LLIOError);
};
#endif// LowLevelIO_HPP
