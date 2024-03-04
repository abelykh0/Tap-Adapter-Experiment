#pragma once

using namespace Napi;

class WriteAsyncWorker : public AsyncWorker
{
public:
    WriteAsyncWorker(
        const Function& callback,
        const HANDLE handle,
        const Uint8Array buffer,
        const int bytesToRead
    );

protected:
    void Execute() override;
    void OnOK() override;
    void OnError(const Error& e) override;

private:
    HANDLE _handle;
    Uint8Array _buffer;
    DWORD _bytesToWrite;
    DWORD _bytesWritten;
};