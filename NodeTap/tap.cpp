#include "pch.h"
#include <assert.h>
#include <string>
#include <vector>

#include "tap.h"
#include "readasyncworker.h"
#include "writeasyncworker.h"

#ifdef _WIN32
#include <winioctl.h>
#include <iphlpapi.h>
#else
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
typedef struct in_addr IN_ADDR;
#endif

using namespace std;

static bool GetInt64Parameter(Env env, Value parameter, int64_t* value);
static bool GetIPv4AddressParameter(Env env, Value parameter, IN_ADDR* value);
static bool GetBoolParameter(Env env, Value parameter, bool* value);
static bool GetBufferParameter(Env env, Value parameter, Buffer<uint8_t>* value);
#ifdef _WIN32
static bool GetDeviceGuid(wstring& deviceGuid);
#endif

// ()
// returns: HANDLE
Value OpenTap(const CallbackInfo& info)
{
	Env env = info.Env();

#ifdef _WIN32
	wstring deviceGuid;
	if (!GetDeviceGuid(deviceGuid))
	{
		TypeError::New(env, CANNOT_OPEN_TAP).ThrowAsJavaScriptException();
		return env.Null();
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
	bool success = handle != INVALID_HANDLE_VALUE;
#else
	int handle = open("/dev/net/tap", O_RDWR);
	bool success = handle != -1;
#endif

	if (!success)
	{
		TypeError::New(env, CANNOT_OPEN_TAP).ThrowAsJavaScriptException();
		return env.Null();
	}

	return BigInt::New(env, (int64_t)handle);
}

// int64_t  handle,
// LPWSTR   ipAddress,
// LPWSTR   mask,
// LPWSTR   dhcpServerIP
// returns: bool
Value ConfigDhcp(const CallbackInfo& info)
{
	Env env = info.Env();

	if (info.Length() < 4) 
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	int64_t handle;
	if (!GetInt64Parameter(env, info[0], &handle))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	IN_ADDR ipAddress;
	if (!GetIPv4AddressParameter(env, info[1], &ipAddress))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	IN_ADDR mask;
	if (!GetIPv4AddressParameter(env, info[2], &mask))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	IN_ADDR dhcpServerIP;
	if (!GetIPv4AddressParameter(env, info[3], &dhcpServerIP))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	u_long buffer[4];
	buffer[0] = ipAddress.s_addr;
	buffer[1] = mask.s_addr;
	buffer[2] = dhcpServerIP.s_addr;
	buffer[3] = 3600; // TTL
	DWORD bytesReturned;
	BOOL success = DeviceIoControl(
		(HANDLE)handle,
		TAP_IOCTL_CONFIG_DHCP_MASQ,
		buffer, 16,
		nullptr, 0, &bytesReturned, nullptr);

	return Boolean::New(env, success == TRUE);
}

// int64_t handle,
// LPWSTR  defaultGateway,
// LPWSTR  dns1, (optional)
// LPWSTR  dns2, (optional)
// returns bool
Value DhcpSetOptions(const CallbackInfo& info)
{
	Env env = info.Env();

	if (info.Length() < 2)
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	int64_t handle;
	if (!GetInt64Parameter(env, info[0], &handle))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	IN_ADDR defaultGateway;
	if (!GetIPv4AddressParameter(env, info[1], &defaultGateway))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	uint8_t buffer[16];

	// Default gateway
	buffer[0] = 3;
	buffer[1] = 4;
	memcpy(&buffer[2], &defaultGateway.s_addr, sizeof(u_long));

	int bufferSize = 6;

	// DNS addresses
	int dnsAddressCount = (int)info.Length() - 2;
	if (dnsAddressCount > 0)
	{
		buffer[6] = 6;
		buffer[7] = (uint8_t)(4 * dnsAddressCount);

		bufferSize += 2 + (4 * dnsAddressCount);
		for (int i = 0; i < dnsAddressCount; i++)
		{
			IN_ADDR address;
			if (!GetIPv4AddressParameter(env, info[2 + static_cast<size_t>(i)], &address))
			{
				TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
				return env.Null();
			}

			memcpy(&buffer[8 + (4 * i)], &address.s_addr, sizeof(u_long));
		}
	}

	DWORD bytesReturned;
	BOOL success = DeviceIoControl(
		(HANDLE)handle,
		TAP_IOCTL_CONFIG_DHCP_SET_OPT,
		buffer, bufferSize,
		nullptr, 0, &bytesReturned, nullptr);

	return Boolean::New(env, success == TRUE);
}

// int64_t  handle,
// LPWSTR   localIP,
// LPWSTR   remoteIP,
// LPWSTR   remoteMask
// returns: bool
Value ConfigTun(const CallbackInfo& info)
{
	Env env = info.Env();

	if (info.Length() < 4)
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	int64_t handle;
	if (!GetInt64Parameter(env, info[0], &handle))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	IN_ADDR localIP;
	if (!GetIPv4AddressParameter(env, info[1], &localIP))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	IN_ADDR remoteIP;
	if (!GetIPv4AddressParameter(env, info[2], &remoteIP))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	IN_ADDR remoteMask;
	if (!GetIPv4AddressParameter(env, info[3], &remoteMask))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	u_long buffer[3];
	buffer[0] = localIP.s_addr;
	buffer[1] = remoteIP.s_addr;
	buffer[2] = remoteMask.s_addr;

#ifdef _WIN32
	DWORD bytesReturned;
	BOOL success = DeviceIoControl(
		(HANDLE)handle,
		TAP_IOCTL_CONFIG_TUN,
		buffer, 12,
		nullptr, 0, &bytesReturned, nullptr);
#else
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
	strncpy(ifr.ifr_name, "tun", IFNAMSIZ);
	BOOL success = ioctl((int)handle, TUNSETIFF, &ifr) >= 0;
#endif

	return Boolean::New(env, success == TRUE);
}

// int64_t  handle,
// bool     connected
// returns: bool
Value SetMediaStatus(const CallbackInfo& info)
{
	Env env = info.Env();

	if (info.Length() < 2)
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	int64_t handle;
	if (!GetInt64Parameter(env, info[0], &handle))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	bool connected;
	if (!GetBoolParameter(env, info[1], &connected))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	int32_t setStatus = connected ? TRUE : FALSE;
	DWORD bytesReturned;
	BOOL success = DeviceIoControl(
		(HANDLE)handle,
		TAP_IOCTL_SET_MEDIA_STATUS,
		&setStatus, sizeof(int32_t),
		nullptr, 0, &bytesReturned, nullptr);

	return Boolean::New(env, success == TRUE);
}

// int64_t handle
// TBuffer buffer,
// int32_t length,
// callback : (err : NodeJS.ErrnoException | null, bytesRead : number, buffer : TBuffer) = > void,
Value Read(const CallbackInfo& info)
{
	Env env = info.Env();

	if (info.Length() < 4)
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	int64_t handle;
	if (!GetInt64Parameter(env, info[0], &handle))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	Buffer<uint8_t> buffer;
	if (!GetBufferParameter(env, info[1], &buffer))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	int64_t length;
	if (!GetInt64Parameter(env, info[2], &length))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	if ((int64_t)buffer.ByteLength() < length)
	{
		length = buffer.ByteLength();
	}

	Function callback = info[3].As<Function>();

	ReadAsyncWorker* worker = new ReadAsyncWorker(callback, (HANDLE)handle, buffer, (int)length);
	worker->Queue();

	return env.Null();
}

// int64_t  handle
// TBuffer  buffer,
// int64_t  length,
// returns: int64_t (number of bytesRead)
Value ReadSync(const CallbackInfo& info)
{
	Env env = info.Env();

	if (info.Length() < 3)
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	int64_t handle;
	if (!GetInt64Parameter(env, info[0], &handle))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	Buffer<uint8_t> buffer;
	if (!GetBufferParameter(env, info[1], &buffer))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	int64_t length;
	if (!GetInt64Parameter(env, info[2], &length))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	if ((int64_t)buffer.ByteLength() < length)
	{
		length = buffer.ByteLength();
	}

#ifdef _WIN32
	DWORD bytesRead;
	BOOL success = ReadFile((HANDLE)handle, buffer.Data(), (DWORD)length, &bytesRead, nullptr);
#else
	ssize_t bytesRead = read((int)handle, buffer.Data(), (DWORD)length);
	BOOL success = bytesRead != -1;
#endif

	if (success != TRUE)
	{
		Error::New(env, "File read error").ThrowAsJavaScriptException();
		return env.Null();
	}

	return BigInt::New(env, (int64_t)bytesRead);
}

// int64_t handle
// TBuffer buffer,
// int64_t length,
// callback : (err : NodeJS.ErrnoException | null, bytesRead : number, buffer : TBuffer) = > void,
Value Write(const CallbackInfo& info)
{
	Env env = info.Env();

	if (info.Length() < 4)
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	int64_t handle;
	if (!GetInt64Parameter(env, info[0], &handle))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	Buffer<uint8_t> buffer;
	if (!GetBufferParameter(env, info[1], &buffer))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	int64_t length;
	if (!GetInt64Parameter(env, info[2], &length))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	if ((int64_t)buffer.ByteLength() < length)
	{
		length = buffer.ByteLength();
	}

	Function callback = info[3].As<Function>();

	WriteAsyncWorker* worker = new WriteAsyncWorker(callback, (HANDLE)handle, buffer, (int)length);
	worker->Queue();

	return env.Null();
}

// int64_t  handle
// TBuffer  buffer,
// int64_t  length,
// returns: int64_t (number of bytesWritten)
Value WriteSync(const CallbackInfo& info)
{
	Env env = info.Env();

	if (info.Length() < 3)
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	int64_t handle;
	if (!GetInt64Parameter(env, info[0], &handle))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	Buffer<uint8_t> buffer;
	if (!GetBufferParameter(env, info[1], &buffer))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	int64_t length;
	if (!GetInt64Parameter(env, info[2], &length))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	if ((int64_t)buffer.ByteLength() < length)
	{
		length = buffer.ByteLength();
	}

#ifdef _WIN32
	DWORD bytesWritten;
	BOOL success = WriteFile((HANDLE)handle, buffer.Data(), (DWORD)length, &bytesWritten, nullptr);
#else
	ssize_t bytesWritten = write((int)handle, buffer.Data(), (DWORD)length);
	BOOL success = bytesWritten >= 0;
#endif

	if (success != TRUE)
	{
		Error::New(env, "File write error").ThrowAsJavaScriptException();
		return env.Null();
	}

	return BigInt::New(env, (int64_t)bytesWritten);
}

// int64_t handle
// returns bool
Value Close(const CallbackInfo& info)
{
	Env env = info.Env();

	if (info.Length() < 1)
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

	int64_t handle;
	if (!GetInt64Parameter(env, info[0], &handle))
	{
		TypeError::New(env, ARGUMENT_ERROR).ThrowAsJavaScriptException();
		return env.Null();
	}

#ifdef _WIN32
	BOOL success = CloseHandle((HANDLE)handle);
#else
	BOOL success = close((int)handle) == 0;
#endif

	return Boolean::New(env, success == TRUE);
}

static bool GetIPv4AddressParameter(Env env, Value parameter, IN_ADDR* value)
{
	if (!parameter.IsString())
	{
		return false;
	}

#ifdef _WIN32
	auto parameterValue = parameter.As<String>().Utf16Value();

	NET_ADDRESS_INFO ipAddressInfo;
	DWORD parseResult = ParseNetworkString((WCHAR*)parameterValue.data(), NET_STRING_IPV4_ADDRESS, &ipAddressInfo, nullptr, nullptr);
	if (parseResult != ERROR_SUCCESS)
	{
		return false;
	}

	*value = ipAddressInfo.Ipv4Address.sin_addr;
#else
	auto parameterValue = parameter.As<String>().Utf8Value();

	struct addrinfo hints;
	hints.ai_flags = 0;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	struct addrinfo* res;

	int parseResult = getaddrinfo(parameterValue.data(), nullptr, &hints, &res);
	if (parseResult != 0)
	{
		return false;
	}

	*value = ((struct sockaddr_in*)res->ai_addr)->sin_addr;
	freeaddrinfo(res);
#endif

	return true;
}

static bool GetInt64Parameter(Env env, Value parameter, int64_t* value)
{
	if (parameter.IsNumber())
	{
		*value = parameter.As<Number>().Int64Value();
		return true;
	}

	if (!parameter.IsBigInt())
	{
		return false;
	}

	bool lossless;
	*value = parameter.As<BigInt>().Int64Value(&lossless);

	return true;
}

static bool GetBoolParameter(Env env, Value parameter, bool* value)
{
	if (!parameter.IsBoolean())
	{
		return false;
	}

	*value = parameter.As<Boolean>().Value();

	return true;
}

static bool GetBufferParameter(Env env, Value parameter, Buffer<uint8_t>* value)
{
	if (!parameter.IsBuffer())
	{
		return false;
	}

	*value = parameter.As<Buffer<uint8_t>>();

	return true;
}

#ifdef _WIN32
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
#endif