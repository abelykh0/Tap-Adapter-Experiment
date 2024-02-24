using System;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace TapWindows
{
    internal static partial class NativeMethods
    {
        internal const int FILE_ATTRIBUTE_SYSTEM = 0x04;
        internal const int FILE_FLAG_OVERLAPPED = 0x40000000;
        //internal const int FILE_FLAG_WRITE_THROUGH = 0x80000000;

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
            [In][MarshalAs(UnmanagedType.LPArray)] byte[] lpInBuffer, int nInBufferSize,
            IntPtr lpOutBuffer, int nOutBufferSize,
            out int lpBytesReturned, IntPtr lpOverlapped);
    }
}
