#ifndef PCH_H
#define PCH_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winsock2.h>
#include <ws2ipdef.h>
#include <windns.h>

#define NODE_ADDON_API_DISABLE_DEPRECATED
#include "napi.h"

#endif //PCH_H
