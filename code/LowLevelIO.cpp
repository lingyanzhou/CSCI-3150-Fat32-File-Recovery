#include <system_error>
#include <stdexcept>
#include <unistd.h>
#include "LowLevelIO.hpp"
using namespace std;
LLIOError::LLIOError(int ev) : system_error(ev, system_category()) {}
LLIOEOF::LLIOEOF() : exception() {}
void LowLevelIO::xpread(int fd, void *buf, size_t count,
                        off_t offset) throw(LLIOError, LLIOEOF)
{
  while (count != 0) {
    ssize_t readCount = pread(fd, buf, count, offset);

    if (-1 == readCount) {
      throw LLIOError(errno);
    } else if (0 == readCount) {
      throw LLIOEOF();
    } else {
      buf = (unsigned char *)buf + readCount;
      count -= readCount;
      offset += readCount;
    }
  }
}
void LowLevelIO::xpwrite(int fd, void *buf, size_t count,
                         off_t offset) throw(LLIOError)
{
  while (count != 0) {
    ssize_t writeCount = pwrite(fd, buf, count, offset);

    if (-1 == writeCount) {
      throw LLIOError(errno);
    } else {
      buf = (unsigned char *)buf + writeCount;
      count -= writeCount;
      offset += writeCount;
    }
  }
}
