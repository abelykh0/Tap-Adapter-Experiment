#include "pch.h"
#include "readasyncworker.h"

ReadAsyncWorker::ReadAsyncWorker(
    const Function& callback,
    const HANDLE handle,
    const Uint8Array buffer,
    const int bytesToRead
) : AsyncWorker(callback)
{
    _handle = handle;
    _buffer = buffer;
    _bytesToRead = bytesToRead;
    _bytesRead = 0;
}

void ReadAsyncWorker::OnOK()
{
    Napi::Env env = Env();

    Value bytesRead = BigInt::New(env, (uint64_t)_bytesRead);

    Callback().MakeCallback(
        Receiver().Value(),
        {
            env.Null(), // error
            _buffer, 
            bytesRead 
        }
    );
}

void ReadAsyncWorker::OnError(const Error& e)
{
    Napi::Env env = Env();

    Value bytesRead = BigInt::New(env, (uint64_t)0);

    Callback().MakeCallback(
        Receiver().Value(),
        {
            e.Value(),
            _buffer, 
            bytesRead
        }
    );
}

void ReadAsyncWorker::Execute()
{
    BOOL result = ReadFile(_handle, _buffer.Data(), _bytesToRead, &_bytesRead, nullptr);
    if (result != TRUE)
    {
        Napi::Env env = Env();
        Error::New(env, "File read error").ThrowAsJavaScriptException();
    }
}
