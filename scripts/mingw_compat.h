/*
 * Minimal POSIX shims for building New_Tools/imagewty-tool with mingw-w64.
 *
 * The tool is written against POSIX (lstat, pathconf, mkdir(path, mode)); the
 * equivalents used here are the small well-defined Windows replacements. This
 * header is only used by scripts/build_tools.sh for the Windows build – the
 * upstream sources stay untouched.
 */
#ifndef CT_MINGW_COMPAT_H
#define CT_MINGW_COMPAT_H

#ifdef _WIN32

#include <direct.h>
#include <sys/stat.h>

/* lstat: mingw has no symlinks to follow, a plain stat() is equivalent. */
#define lstat stat

/* mkdir(path, mode): the mode argument is meaningless on Windows. */
#define mkdir(path, mode) _mkdir(path)

/* pathconf(path, _PC_NAME_MAX): report a conservative NTFS name length. */
#ifndef _PC_NAME_MAX
#define _PC_NAME_MAX 0
#endif
#define pathconf(path, name) ((void)(path), (void)(name), 255)

#endif /* _WIN32 */

#endif /* CT_MINGW_COMPAT_H */
