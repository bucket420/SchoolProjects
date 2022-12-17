#include <pwd.h>
#include <unistd.h>

char *cwd() {
  static char buffer[256];
  getcwd(buffer, 256); // ignore size error.
  return buffer;
}
