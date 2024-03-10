#ifndef PCH_H
#define PCH_H

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winsock2.h>
#include <ws2ipdef.h>
#include <windns.h>

#else
#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>

typedef int                 HANDLE;
typedef unsigned long       DWORD;
typedef int                 BOOL;
constexpr int TRUE = 1;
#endif

#define NODE_ADDON_API_DISABLE_DEPRECATED
#include "napi.h"

#endif //PCH_H
