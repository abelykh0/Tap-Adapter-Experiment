#include "pch.h"
#include "tap.h"
#include <assert.h>
#include <vector>

using namespace std;

#define DECLARE_NAPI_METHOD(name, func) { name, 0, func, 0, 0, 0, napi_default, 0 }

static napi_value Init(napi_env env, napi_value exports) 
{
	napi_status status;

	vector<napi_property_descriptor> descriptors =
	{
		DECLARE_NAPI_METHOD("openTap", OpenTap),
		DECLARE_NAPI_METHOD("configDhcp", ConfigDhcp),
		DECLARE_NAPI_METHOD("dhcpSetOptions", DhcpSetOptions),
		DECLARE_NAPI_METHOD("configTun", ConfigTun),
		DECLARE_NAPI_METHOD("setMediaStatus", SetMediaStatus),
		DECLARE_NAPI_METHOD("closeHandle", CloseHandle),
	};

	status = napi_define_properties(env, exports, descriptors.size(), descriptors.data());
	assert(status == napi_ok);

	return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)