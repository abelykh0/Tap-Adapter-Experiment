#include "pch.h"
#include "readasyncworker.h"

ReadAsyncWorker::ReadAsyncWorker(
    const Function& callback,
    const HANDLE handle,
    const Buffer<uint8_t>& buffer,
    const int bytesToRead
) : AsyncWorker(callback)
{
    _handle = handle;
    _bufferRef = ObjectReference::New(buffer, 1);
    _buffer = buffer;
    _bytesToRead = bytesToRead;
    _bytesRead = 0;
}

void ReadAsyncWorker::OnOK()
{
    Napi::Env env = Env();

    Value bytesRead = Number::New(env, (double)_bytesRead);

    Callback().MakeCallback(
        Receiver().Value(),
        {
            env.Null(), // error
            bytesRead,
            _bufferRef.Value()
        }
    );

    _bufferRef.Unref();
}

void ReadAsyncWorker::OnError(const Error& e)
{
    Napi::Env env = Env();

    Value bytesRead = Number::New(env, (double)0);

    Callback().MakeCallback(
        Receiver().Value(),
        {
            e.Value(),
            bytesRead,
            _bufferRef.Value()
        }
    );

    _bufferRef.Unref();
}

void ReadAsyncWorker::Execute()
{
#ifdef _WIN32
    BOOL result = ReadFile(_handle, _buffer.Data(), _bytesToRead, &_bytesRead, nullptr);
#else
    BOOL result = read(_handle, _buffer.Data(), _bytesToRead) >= 0;
#endif

    if (result != TRUE)
    {
        Napi::Env env = Env();
        Error::New(env, "File read error").ThrowAsJavaScriptException();
    }
}
