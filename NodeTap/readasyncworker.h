#pragma once

using namespace Napi;

class ReadAsyncWorker : public AsyncWorker
{
public:
    ReadAsyncWorker(
        const Function& callback,
        const HANDLE handle,
        const Buffer<uint8_t>& buffer,
        const int bytesToRead
    );

protected:
    void Execute() override;
    void OnOK() override;
    void OnError(const Error& e) override;

private:
    HANDLE _handle;
    ObjectReference _bufferRef;
    Buffer<uint8_t> _buffer;
    DWORD _bytesToRead;
    DWORD _bytesRead;
};