#include "pch.h"
#include "writeasyncworker.h"

WriteAsyncWorker::WriteAsyncWorker(
    const Function& callback,
    const HANDLE handle,
    const Uint8Array buffer,
    const int bytesToWrite
) : AsyncWorker(callback)
{
    _handle = handle;
    _buffer = buffer;
    _bytesToWrite = bytesToWrite;
    _bytesWritten = 0;
}

void WriteAsyncWorker::OnOK()
{
    Napi::Env env = Env();

    Value bytesWritten = BigInt::New(env, (uint64_t)_bytesWritten);

    Callback().MakeCallback(
        Receiver().Value(),
        {
            env.Null(), // error
            _buffer, 
            bytesWritten
        }
    );
}

void WriteAsyncWorker::OnError(const Error& e)
{
    Napi::Env env = Env();

    Value bytesWritten = BigInt::New(env, (uint64_t)0);

    Callback().MakeCallback(
        Receiver().Value(),
        {
            e.Value(),
            _buffer, 
            bytesWritten
        }
    );
}

void WriteAsyncWorker::Execute()
{
    BOOL result = WriteFile(_handle, _buffer.Data(), _bytesToWrite, &_bytesWritten, nullptr);
    if (result != TRUE)
    {
        Napi::Env env = Env();
        Error::New(env, "File read error").ThrowAsJavaScriptException();
    }
}
