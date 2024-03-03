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
static bool GetDeviceGuid(wstring& deviceGuid);

// returns HANDLE
napi_value OpenTap(napi_env env, napi_callback_info info) 
{
	wstring deviceGuid;
	if (!GetDeviceGuid(deviceGuid))
	{
		napi_throw_type_error(env, nullptr, CANNOT_OPEN_TAP);
		return nullptr;
	}

	wstring tapFileName = UsermodeDeviceSpace;
	tapFileName += deviceGuid;
	tapFileName += L".tap";

	HANDLE handle = CreateFile(
		tapFileName.data(),
		FILE_WRITE_ACCESS | FILE_READ_ACCESS,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH,
		nullptr);

	if (handle == INVALID_HANDLE_VALUE)
	{
		napi_throw_type_error(env, nullptr, CANNOT_OPEN_TAP);
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
	buffer[0] = ipAddress.S_un.S_addr;
	buffer[1] = mask.S_un.S_addr;
	buffer[2] = dhcpServerIP.S_un.S_addr;
	buffer[3] = 3600; // TTL
	DWORD bytesReturned;
	BOOL success = DeviceIoControl(
		(HANDLE)handle,
		TAP_IOCTL_CONFIG_DHCP_MASQ,
		buffer, 16,
		nullptr, 0, &bytesReturned, nullptr);

	napi_value result;
	status = napi_get_boolean(env, success == TRUE, &result);
	assert(status == napi_ok);
	return result;
}

// int64_t handle,
// LPWSTR  defaultGateway,
// LPWSTR  dns1, (optional)
// LPWSTR  dns2, (optional)
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

	IN_ADDR defaultGateway;
	if (!GetIPv4AddressParameter(env, args[1], &defaultGateway))
	{
		napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
		return nullptr;
	}

	uint8_t buffer[16];

	// Default gateway
	buffer[0] = 3;
	buffer[1] = 4;
	memcpy(&buffer[2], &defaultGateway.S_un.S_addr, sizeof(u_long));

	int bufferSize = 6;

	// DNS addresses
	int dnsAddressCount = (int)argc - 2;
	if (dnsAddressCount > 0)
	{
		buffer[6] = 6;
		buffer[7] = 4 * dnsAddressCount;

		bufferSize += 2 + (4 * dnsAddressCount);
		for (int i = 0; i < dnsAddressCount; i++)
		{
			IN_ADDR address;
			if (!GetIPv4AddressParameter(env, args[2 + i], &address))
			{
				napi_throw_type_error(env, nullptr, ARGUMENT_ERROR);
				return nullptr;
			}

			memcpy(&buffer[8 + (4 * i)], &address.S_un.S_addr, sizeof(u_long));
		}
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
	if (valuetype != napi_number)
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
	strSize++;
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

static bool GetDeviceGuid(wstring& deviceGuid)
{
	HKEY keyHandle;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, AdapterKey, 0, KEY_READ, &keyHandle) != ERROR_SUCCESS)
	{
		return false;
	}

	DWORD numberOfSubkeys;
	DWORD maxSubkeyLen;
	LSTATUS success = RegQueryInfoKey(
		keyHandle,    
		nullptr,
		nullptr,
		nullptr,      
		&numberOfSubkeys,      
		&maxSubkeyLen,      
		nullptr,      
		nullptr,     
		nullptr,      
		nullptr,
		nullptr,		
		nullptr);
	if (success != ERROR_SUCCESS)
	{
		RegCloseKey(keyHandle);
		return false;
	}

	for (DWORD i = 0; i < numberOfSubkeys; i++)
	{
		vector<TCHAR> subKey(maxSubkeyLen);
		DWORD subKeySize = maxSubkeyLen;
		success = RegEnumKeyEx(
			keyHandle, 
			i,
			subKey.data(),
			&subKeySize,
			nullptr,
			nullptr,
			nullptr,
			nullptr);
		if (success != ERROR_SUCCESS)
		{
			continue;
		}

		DWORD dataSize = 20;
		vector<TCHAR> componentId(dataSize);
		success = RegGetValue(
			keyHandle,
			subKey.data(),
			L"ComponentId",
			RRF_RT_REG_SZ,
			nullptr,
			componentId.data(),
			&dataSize);
		if (success != ERROR_SUCCESS)
		{
			continue;
		}

		if (dataSize > 0 && wcsncmp(TapName, componentId.data(), 10) == 0)
		{
			DWORD dataSize = 80;
			vector<TCHAR> instanceId(dataSize);
			success = RegGetValue(
				keyHandle,
				subKey.data(),
				L"NetCfgInstanceId",
				RRF_RT_REG_SZ,
				nullptr,
				instanceId.data(),
				&dataSize);
			if (success == ERROR_SUCCESS)
			{
				RegCloseKey(keyHandle);
				deviceGuid.assign(instanceId.data());
				return true;
			}
		}
	}

	RegCloseKey(keyHandle);
	return false;
}