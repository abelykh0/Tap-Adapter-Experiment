using System;

namespace TapWindows
{
    internal static class TapControlCode
    {
        public const int TAP_IOCTL_SET_MEDIA_STATUS = (6 << 2) | (NativeMethods.FILE_DEVICE_UNKNOWN << 16)
            | (NativeMethods.FILE_ANY_ACCESS << 14) | NativeMethods.METHOD_BUFFERED;

        public const int TAP_IOCTL_CONFIG_DHCP_MASQ = (7 << 2) | (NativeMethods.FILE_DEVICE_UNKNOWN << 16) 
            | (NativeMethods.FILE_ANY_ACCESS << 14) | NativeMethods.METHOD_BUFFERED;

        public const int TAP_IOCTL_CONFIG_DHCP_SET_OPT = (9 << 2) | (NativeMethods.FILE_DEVICE_UNKNOWN << 16)
            | (NativeMethods.FILE_ANY_ACCESS << 14) | NativeMethods.METHOD_BUFFERED;

        public const int TAP_IOCTL_CONFIG_TUN = (10 << 2) | (NativeMethods.FILE_DEVICE_UNKNOWN << 16)
            | (NativeMethods.FILE_ANY_ACCESS << 14) | NativeMethods.METHOD_BUFFERED;
    }
}
