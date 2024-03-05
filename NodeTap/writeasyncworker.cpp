#include "pch.h"
#include "writeasyncworker.h"

WriteAsyncWorker::WriteAsyncWorker(
    const Function& callback,
    const HANDLE handle,
    const Buffer<uint8_t> buffer,
    const int bytesToWrite
) : AsyncWorker(callback)
{
    _handle = handle;
    _buffer = buffer;
    _bufferRef = ObjectReference::New(buffer, 1);
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
            bytesWritten,
            _bufferRef.Value()
        }
    );

    _bufferRef.Unref();
}

void WriteAsyncWorker::OnError(const Error& e)
{
    Napi::Env env = Env();

    Value bytesWritten = BigInt::New(env, (uint64_t)0);

    Callback().MakeCallback(
        Receiver().Value(),
        {
            e.Value(),
            bytesWritten,
            _bufferRef.Value()
        }
    );

    _bufferRef.Unref();
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
