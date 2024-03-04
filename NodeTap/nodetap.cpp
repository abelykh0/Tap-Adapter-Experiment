#include "pch.h"
#include "tap.h"

Object Init(Env env, Object exports) 
{
    exports.Set(String::New(env, "openTap"), Function::New(env, OpenTap));
    return exports;
}

NODE_API_MODULE(nodetap, Init)