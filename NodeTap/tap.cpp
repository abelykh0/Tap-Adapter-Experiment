#include "pch.h"
#include "tap.h"
#include <assert.h>
#include <iphlpapi.h>
#include <string>
#include <vector>

using namespace std;

bool GetInt64Parameter(napi_env env, napi_value parameter, int64_t* value);
bool GetBoolParameter(napi_env env, napi_value parameter, bool* value);
bool GetIPv4AddressParameter(napi_env env, napi_value parameter, IN_ADDR* value);

// returns HANDLE
napi_value OpenTap(napi_env env, napi_callback_info info) 
{
	HANDLE handle = CreateFile(
		TAP_FILE_NAME,
		FILE_WRITE_ACCESS | FILE_READ_ACCESS,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH,
		nullptr);

	if (handle == INVALID_HANDLE_VALUE)
	{
		napi_throw_type_error(env, nullptr, "Error opening tap file");
		return nullptr;
	}

	napi_value result;
	napi_status status = napi_create_int64(env, (int64_t)handle, &result);
	assert(status == napi_ok);
	return result;
}

// int64_t  handle,
// LPWSTR   ipAddress,
// LPWSTR   mask,
// LPWSTR   dhcpServerIP
// returns bool
napi_value ConfigDhcp(napi_env env, napi_callback_info info)
{
	napi_status status;

	size_t argc = 4;
	napi_value args[4];
	status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
	assert(status == napi_ok);

	if (argc < 4) 
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	int64_t handle;
	if (!GetInt64Parameter(env, args[0], &handle))
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	IN_ADDR ipAddress;
	if (!GetIPv4AddressParameter(env, args[1], &ipAddress))
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	IN_ADDR mask;
	if (!GetIPv4AddressParameter(env, args[2], &mask))
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	IN_ADDR dhcpServerIP;
	if (!GetIPv4AddressParameter(env, args[3], &dhcpServerIP))
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	u_long buffer[4];
	buffer[0] = htonl(ipAddress.S_un.S_addr);
	buffer[1] = htonl(mask.S_un.S_addr);
	buffer[2] = htonl(dhcpServerIP.S_un.S_addr);
	buffer[3] = 3600; // TTL
	DWORD bytesReturned;
	BOOL success = DeviceIoControl(
		(HANDLE)handle,
		TAP_IOCTL_CONFIG_DHCP_MASQ,
		buffer, 16,
		nullptr, 0, &bytesReturned, nullptr);

	napi_value result;
	status = napi_get_boolean(env, true, &result);
	assert(status == napi_ok);
	return result;
}

// int64_t handle,
// BYTE    optionID,
// LPWSTR  address1,
// LPWSTR  address2, (optional)
// LPWSTR  address3, (optional)
// LPWSTR  address4  (optional)
// returns bool
napi_value DhcpSetOptions(napi_env env, napi_callback_info info)
{
	napi_status status;

	size_t argc = 6;
	napi_value args[6];
	status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
	assert(status == napi_ok);

	if (argc < 3)
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	int64_t handle;
	if (!GetInt64Parameter(env, args[0], &handle))
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	int64_t optionID;
	if (!GetInt64Parameter(env, args[1], &optionID))
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	int addressCount = (int)argc - 2;
	int bufferSize = 2 + (4 * addressCount);

	uint8_t buffer[2 + (4 * 4)];

	buffer[0] = (uint8_t)optionID;
	buffer[1] = 4 * addressCount; // size

	for (int i = 0; i < addressCount; i++)
	{
		IN_ADDR address;
		if (!GetIPv4AddressParameter(env, args[2 + i], &address))
		{
			napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
			return nullptr;
		}

		u_long addressLE = htonl(address.S_un.S_addr);
		memcpy(&buffer[2 + (4 * i)], &addressLE, sizeof(u_long));
	}

	DWORD bytesReturned;
	BOOL success = DeviceIoControl(
		(HANDLE)handle,
		TAP_IOCTL_CONFIG_DHCP_SET_OPT,
		buffer, bufferSize,
		nullptr, 0, &bytesReturned, nullptr);

	napi_value result;
	status = napi_get_boolean(env, success == TRUE, &result);
	assert(status == napi_ok);
	return result;
}

// int64_t handle,
// bool    connected
// returns bool
napi_value SetMediaStatus(napi_env env, napi_callback_info info)
{
	napi_status status;

	size_t argc = 2;
	napi_value args[2];
	status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
	assert(status == napi_ok);

	if (argc < 2) 
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	int64_t handle;
	if (!GetInt64Parameter(env, args[0], &handle))
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	bool connected;
	if (!GetBoolParameter(env, args[1], &connected))
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	int32_t setStatus = connected ? TRUE : FALSE;
	DWORD bytesReturned;
	BOOL success = DeviceIoControl(
		(HANDLE)handle,
		TAP_IOCTL_SET_MEDIA_STATUS,
		&setStatus, sizeof(int32_t),
		nullptr, 0, &bytesReturned, nullptr);

	napi_value result;
	status = napi_get_boolean(env, success == TRUE, &result);
	assert(status == napi_ok);
	return result;
}

// int64_t handle,
// LPWSTR  localIP,
// LPWSTR  remoteIP,
// LPWSTR  remoteMask
// returns bool
napi_value ConfigTun(napi_env env, napi_callback_info info)
{
	napi_status status;

	size_t argc = 4;
	napi_value args[4];
	status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
	assert(status == napi_ok);

	if (argc < 4)
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	int64_t handle;
	if (!GetInt64Parameter(env, args[0], &handle))
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	IN_ADDR localIP;
	if (!GetIPv4AddressParameter(env, args[1], &localIP))
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	IN_ADDR remoteIP;
	if (!GetIPv4AddressParameter(env, args[2], &remoteIP))
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	IN_ADDR remoteMask;
	if (!GetIPv4AddressParameter(env, args[3], &remoteMask))
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	u_long buffer[3];
	buffer[0] = htonl(localIP.S_un.S_addr);
	buffer[1] = htonl(remoteIP.S_un.S_addr);
	buffer[2] = htonl(remoteMask.S_un.S_addr);
	DWORD bytesReturned;
	BOOL success = DeviceIoControl(
		(HANDLE)handle,
		TAP_IOCTL_CONFIG_TUN,
		buffer, 12,
		nullptr, 0, &bytesReturned, nullptr);

	napi_value result;
	status = napi_get_boolean(env, true, &result);
	assert(status == napi_ok);
	return result;
}

// int64_t handle
// returns bool
napi_value CloseHandle(napi_env env, napi_callback_info info)
{
	napi_status status;

	size_t argc = 1;
	napi_value args[1];
	status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
	assert(status == napi_ok);

	if (argc < 1) 
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	int64_t handle;
	if (!GetInt64Parameter(env, args[0], &handle))
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	BOOL success = CloseHandle((HANDLE)handle);

	napi_value result;
	status = napi_get_boolean(env, success == TRUE, &result);
	assert(status == napi_ok);
	return result;
}

static bool GetInt64Parameter(napi_env env, napi_value parameter, int64_t* value)
{
	napi_status status;
	napi_valuetype valuetype;
	status = napi_typeof(env, parameter, &valuetype);
	assert(status == napi_ok);
	if (valuetype != napi_bigint)
	{
		return false;
	}

	status = napi_get_value_int64(env, parameter, value);
	assert(status == napi_ok);
	return true;
}

static bool GetBoolParameter(napi_env env, napi_value parameter, bool* value)
{
	napi_status status;
	napi_valuetype valuetype;
	status = napi_typeof(env, parameter, &valuetype);
	assert(status == napi_ok);
	if (valuetype != napi_boolean)
	{
		return false;
	}

	status = napi_get_value_bool(env, parameter, value);
	assert(status == napi_ok);
	return true;
}

static bool GetIPv4AddressParameter(napi_env env, napi_value parameter, IN_ADDR* value)
{
	napi_status status;
	napi_valuetype valuetype;
	status = napi_typeof(env, parameter, &valuetype);
	assert(status == napi_ok);
	if (valuetype != napi_string)
	{
		return false;
	}

	size_t strSize;
	status = napi_get_value_string_utf16(env, parameter, nullptr, 0, &strSize);
	assert(status == napi_ok);
	vector<char16_t> result(strSize);
	status = napi_get_value_string_utf16(env, parameter, result.data(), strSize, &strSize);
	assert(status == napi_ok);
	NET_ADDRESS_INFO ipAddressInfo;
	DWORD parseResult = ParseNetworkString((WCHAR*)result.data(), NET_STRING_IPV4_ADDRESS, &ipAddressInfo, nullptr, nullptr);
	if (parseResult != ERROR_SUCCESS)
	{
		return false;
	}

	*value = ipAddressInfo.Ipv4Address.sin_addr;
	return true;
}