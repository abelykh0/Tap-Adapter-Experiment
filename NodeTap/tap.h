#pragma once

#include <winioctl.h>

#define UsermodeDeviceSpace L"\\\\.\\Global\\"
#define AdapterKey          L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}"
#define TapName             L"TAP0901"

#define TAP_CONTROL_CODE(request,method) \
  CTL_CODE (FILE_DEVICE_UNKNOWN, request, method, FILE_ANY_ACCESS)

#define TAP_IOCTL_GET_MAC               TAP_CONTROL_CODE (1, METHOD_BUFFERED)
#define TAP_IOCTL_GET_VERSION           TAP_CONTROL_CODE (2, METHOD_BUFFERED)
#define TAP_IOCTL_GET_MTU               TAP_CONTROL_CODE (3, METHOD_BUFFERED)
#define TAP_IOCTL_GET_INFO              TAP_CONTROL_CODE (4, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_POINT_TO_POINT TAP_CONTROL_CODE (5, METHOD_BUFFERED)
#define TAP_IOCTL_SET_MEDIA_STATUS      TAP_CONTROL_CODE (6, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_MASQ      TAP_CONTROL_CODE (7, METHOD_BUFFERED)
#define TAP_IOCTL_GET_LOG_LINE          TAP_CONTROL_CODE (8, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_SET_OPT   TAP_CONTROL_CODE (9, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_TUN            TAP_CONTROL_CODE(10, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCPV6_MASQ    TAP_CONTROL_CODE(11, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCPV6_SET_OPT TAP_CONTROL_CODE(12, METHOD_BUFFERED)

#define ARGUMENT_ERROR "Wrong arguments"
#define CANNOT_OPEN_TAP "Error opening tap file"

napi_value OpenTap(napi_env env, napi_callback_info info);
napi_value ConfigDhcp(napi_env env, napi_callback_info info);
napi_value DhcpSetOptions(napi_env env, napi_callback_info info);
napi_value ConfigTun(napi_env env, napi_callback_info info);
napi_value SetMediaStatus(napi_env env, napi_callback_info info);
napi_value CloseHandle(napi_env env, napi_callback_info info);
