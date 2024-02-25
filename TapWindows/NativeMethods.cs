using System;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using Microsoft.Win32.SafeHandles;

namespace TapWindows
{
    internal static partial class NativeMethods
    {
        internal const uint FILE_ATTRIBUTE_SYSTEM = 0x04;
        internal const uint FILE_FLAG_OVERLAPPED = 0x40000000;
        internal const uint FILE_FLAG_NO_BUFFERING = 0x20000000;
        internal const uint FILE_FLAG_WRITE_THROUGH = 0x80000000;

        [LibraryImport("kernel32.dll", EntryPoint = "CreateFileW", SetLastError = true, StringMarshalling = StringMarshalling.Utf16)]
        public static partial SafeFileHandle CreateFile(
            string lpFileName,
            FileAccess dwDesiredAccess,
            FileShare dwShareMode,
            IntPtr lpSecurityAttributes,
            FileMode dwCreationDisposition,
            uint dwFlagsAndAttributes,
            IntPtr hTemplateFile);

        internal const int FILE_ANY_ACCESS = 0;
        internal const int METHOD_BUFFERED = 0;
        internal const int FILE_DEVICE_UNKNOWN = 0x00000022;

        [LibraryImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static partial bool DeviceIoControl(
            SafeFileHandle hDevice, int dwIoControlCode,
            [In, MarshalAs(UnmanagedType.LPArray)] byte[] lpInBuffer, int nInBufferSize,
            IntPtr lpOutBuffer, int nOutBufferSize,
            out int lpBytesReturned, IntPtr lpOverlapped);

        internal const int ERROR_SUCCESS = 0x0;
        internal const int ERROR_INVALID_HANDLE = 0x6;
        internal const int ERROR_OPERATION_ABORTED = 0x3E3;
        internal const int ERROR_IO_PENDING = 997;

        [LibraryImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal unsafe static partial bool ReadFile(
            SafeFileHandle hFile,
            byte* lpBuffer, 
            int nNumberOfBytesToRead, 
            out int lpNumberOfBytesRead,
            NativeOverlapped* lpOverlapped);

        [LibraryImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal unsafe static partial bool WriteFile(
            SafeFileHandle hFile,
            byte* lpBuffer, 
            int nNumberOfBytesToWrite, 
            out int lpNumberOfBytesWritten,
            NativeOverlapped* lpOverlapped);
    }
}
