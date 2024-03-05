#pragma once

#include <winioctl.h>

using namespace Napi;

constexpr auto UsermodeDeviceSpace = L"\\\\.\\Global\\";
constexpr auto AdapterKey = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}";
constexpr auto TapName = L"TAP0901";

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

constexpr auto ARGUMENT_ERROR = "Wrong arguments";
constexpr auto CANNOT_OPEN_TAP = "Error opening tap file";

Object Init(Env env, Object exports);

Value OpenTap(const CallbackInfo& info);
Value ConfigDhcp(const CallbackInfo& info);
Value DhcpSetOptions(const CallbackInfo& info);
Value ConfigTun(const CallbackInfo& info);
Value SetMediaStatus(const CallbackInfo& info);
Value Read(const CallbackInfo& info);
Value ReadSync(const CallbackInfo& info);
Value Write(const CallbackInfo& info);
Value WriteSync(const CallbackInfo& info);
Value Close(const CallbackInfo& info);
