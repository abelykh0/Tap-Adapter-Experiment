#include "pch.h"
#include "tap.h"

Object Init(Env env, Object exports) 
{
    exports.Set(String::New(env, "openTap"), Function::New(env, OpenTap));
    exports.Set(String::New(env, "configDhcp"), Function::New(env, ConfigDhcp));
    exports.Set(String::New(env, "dhcpSetOptions"), Function::New(env, DhcpSetOptions));
    exports.Set(String::New(env, "configTun"), Function::New(env, ConfigTun));
    exports.Set(String::New(env, "setMediaStatus"), Function::New(env, SetMediaStatus));
    exports.Set(String::New(env, "read"), Function::New(env, Read));
    exports.Set(String::New(env, "readSync"), Function::New(env, ReadSync));
    exports.Set(String::New(env, "write"), Function::New(env, Write));
    exports.Set(String::New(env, "writeSync"), Function::New(env, WriteSync));
    exports.Set(String::New(env, "close"), Function::New(env, Close));

    return exports;
}

NODE_API_MODULE(nodetap, Init)