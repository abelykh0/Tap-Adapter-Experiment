#include "pch.h"
#include "tap.h"
#include <assert.h>
#include <iphlpapi.h>
#include <string>
#include <vector>

using namespace std;

bool GetInt64Parameter(napi_env env, napi_value parameter, int64_t* value);
bool GetBoolParameter(napi_env env, napi_value parameter, bool* value);
bool GetStringParameter(napi_env env, napi_value parameter, u16string value);

// returns HANDLE
napi_value OpenTap(napi_env env, napi_callback_info info) 
{
	napi_value result;

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

	napi_status status = napi_create_int64(env, (int64_t)handle, &result);
	assert(status == napi_ok);
	return result;
}

// int64_t  handle,
// LPWSTR   ipAddress,
// LPWSTR   mask,
// LPWSTR   dhcpServerIP,
// LPWSTR[] defaultGateways,
// LPWSTR[] dnsAddresses
// returns bool
napi_value ConfigDhcp(napi_env env, napi_callback_info info)
{
	// int64_t  handle,

	napi_status status;

	size_t argc = 5;
	napi_value args[5];
	status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
	assert(status == napi_ok);

	if (argc < 5) 
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

	u16string ipAddress;
	if (!GetStringParameter(env, args[1], ipAddress))
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}
	NET_ADDRESS_INFO ipAddressInfo;
	DWORD parseResult = ParseNetworkString((WCHAR*)ipAddress.data(), NET_STRING_IPV4_ADDRESS, &ipAddressInfo, nullptr, nullptr);
	if (parseResult != ERROR_SUCCESS) 
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	u16string mask;
	if (!GetStringParameter(env, args[2], mask))
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}
	NET_ADDRESS_INFO maskInfo;
	DWORD parseResult = ParseNetworkString((WCHAR*)ipAddress.data(), NET_STRING_IPV4_ADDRESS, &maskInfo, nullptr, nullptr);
	if (parseResult != ERROR_SUCCESS)
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}




	napi_value result;
	status = napi_get_boolean(env, true, &result);
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
// 
// returns bool
napi_value ConfigTun(napi_env env, napi_callback_info info)
{
	// int64_t  handle,

	napi_value result;

	HANDLE handle = CreateFile(
		TAP_FILE_NAME,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED,
		nullptr);

	napi_status status = napi_create_int64(env, (int64_t)handle, &result);
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

static bool GetStringParameter(napi_env env, napi_value parameter, u16string value)
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
	value.assign(result.data());
	return true;
}