using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Microsoft.Win32;
using Microsoft.Win32.SafeHandles;

namespace TapWindows
{
    /// <summary>
    /// Controls "TAP-Windows Adapter V9" adapter.
    /// </summary>
    /// <remarks>
    /// See also https://www.updatestar.com/en/directdownload/tap-windows/2281292.
    /// </remarks>
    public class TapAdapter : IDisposable
    {
        private const string UsermodeDeviceSpace = @"\\.\Global\";
        private const string AdapterKey = @"SYSTEM\CurrentControlSet\Control\Class\{4D36E972-E325-11CE-BFC1-08002BE10318}";
        private const string TapName = "TAP0901";

        private readonly SafeFileHandle _tapHandle;
        private bool _disposedValue;

        internal static string? DeviceGUID
        {
            get
            {
                var regAdapters = Registry.LocalMachine.OpenSubKey(AdapterKey, false);
                if (regAdapters == null) 
                {
                    return null;
                }

                string[] keyNames = regAdapters.GetSubKeyNames();
                string devGuid = "";
                foreach (string x in keyNames)
                {
                    var regAdapter = regAdapters!.OpenSubKey(x, false);
                    if (regAdapter == null)
                    {
                        continue;
                    }

                    var id = regAdapter.GetValue("ComponentId");
                    if (id != null && string.Equals(id!.ToString(), TapName, StringComparison.OrdinalIgnoreCase))
                    {
                        return regAdapter.GetValue("NetCfgInstanceId")?.ToString();
                    }
                }
                return devGuid;
            }
        }

        /// <summary>
        /// Creates a device stream.
        /// </summary>
        /// <returns>The device stream.</returns>
        public FileStream GetStream()
        {
            return new FileStream(this._tapHandle, FileAccess.ReadWrite);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TapAdapter" /> class.
        /// </summary>
        /// <exception cref="System.NotSupportedException">TAP-Windows Adapter V9 driver is not installed or cannot open the TAP-Windows Adapter.</exception>
        public TapAdapter()
        {
            if (DeviceGUID == null)
            {
                throw new NotSupportedException("TAP-Windows Adapter V9 driver is not installed");
            }

            this._tapHandle = NativeMethods.CreateFile(
                UsermodeDeviceSpace + DeviceGUID + ".tap", 
                FileAccess.ReadWrite, FileShare.ReadWrite,
                IntPtr.Zero, FileMode.Open,
                NativeMethods.FILE_ATTRIBUTE_SYSTEM | NativeMethods.FILE_FLAG_OVERLAPPED,
                IntPtr.Zero);

            if (this._tapHandle.IsInvalid)
            {
                throw new NotSupportedException("Cannot open the TAP-Windows Adapter, probably something else is using it");
            }
        }

        /// <summary>
        /// Configures the built-in DHCP server of the TAP driver.
        /// </summary>
        /// <param name="ipAddress">The IP address.</param>
        /// <param name="mask">The network mask.</param>
        /// <param name="dhcpServerIP">The IP address of the DHCP server.</param>
        /// <param name="defaultGateway">The default gateway.</param>
        /// <param name="dns">Up to 4 DNS servers.</param>
        /// <returns><see langword="true"/> if successful, otherwise <see langword="false"/>.</returns>
        public bool ConfigDHCP(
            string ipAddress,
            string mask,
            string dhcpServerIP,
            string? defaultGateway,
            IList<string>? dns)
        {
            var setupDhcp = new byte[16];
            BinaryPrimitives.WriteUInt32LittleEndian(setupDhcp.AsSpan(0, 4), ParseIP(ipAddress));
            BinaryPrimitives.WriteUInt32LittleEndian(setupDhcp.AsSpan(4, 4), ParseIP(mask));
            BinaryPrimitives.WriteUInt32LittleEndian(setupDhcp.AsSpan(8, 4), ParseIP(dhcpServerIP));
            BinaryPrimitives.WriteInt32LittleEndian(setupDhcp.AsSpan(12, 4), 3600); // TTL
            bool result = NativeMethods.DeviceIoControl(
                this._tapHandle, TapControlCode.TAP_IOCTL_CONFIG_DHCP_MASQ,
                setupDhcp, setupDhcp.Length,
                IntPtr.Zero, 0, out _, IntPtr.Zero);

            if (!result)
            {
                return false;
            }

            bool setDefaultGateway = !string.IsNullOrEmpty(defaultGateway);
            int dnsCount = 0;
            if (dns != null && dns.Count > 0)
            {
                dnsCount = int.Min(dns.Count, 4);
            }

            int size = (setDefaultGateway ? 6 : 0) + (dnsCount > 0 ? 2 + (4 * dnsCount) : 0);
            if (size == 0)
            {
                return true;
            }

            var setDhcpOptions = new byte[size];

            int index = 0;
            if (setDefaultGateway)
            {
                setDhcpOptions[0] = 3;
                setDhcpOptions[1] = 4;
                BinaryPrimitives.WriteUInt32LittleEndian(setDhcpOptions.AsSpan(2, 4), ParseIP(defaultGateway!));
                index += 6;
            }

            if (dnsCount > 0)
            {
                setDhcpOptions[index] = 3;
                index++;
                setDhcpOptions[index] = (byte)(4 * dnsCount);
                index++;
                foreach (var dnsAddress in dns!)
                {
                    BinaryPrimitives.WriteUInt32LittleEndian(setDhcpOptions.AsSpan(index, 4), ParseIP(dnsAddress));
                    index += 4;
                }
            }

            result = NativeMethods.DeviceIoControl(
                this._tapHandle, TapControlCode.TAP_IOCTL_CONFIG_DHCP_SET_OPT,
                setDhcpOptions, setDhcpOptions.Length,
                IntPtr.Zero, 0, out _, IntPtr.Zero);

            return result;
        }


        /// <summary>
        /// Sets the media status.
        /// </summary>
        /// <param name="connected">If set to <see langword="true"/>, set device status to "connected",
        /// otherwise to "disconnected".</param>
        /// <returns><see langword="true"/> if successful, otherwise <see langword="false"/>.</returns>
        public bool SetMediaStatus(bool connected)
        {
            var setMediaStatus = new byte[4];
            BinaryPrimitives.WriteInt32LittleEndian(setMediaStatus.AsSpan(0, 4), connected ? 1 : 0);
            bool result = NativeMethods.DeviceIoControl(
                this._tapHandle, TapControlCode.TAP_IOCTL_SET_MEDIA_STATUS,
                setMediaStatus, setMediaStatus.Length,
                IntPtr.Zero, 0, out _, IntPtr.Zero);

            return result;
        }

        /// <summary>
        /// Configures the tunnel.
        /// </summary>
        /// <param name="localIP">The local IP.</param>
        /// <param name="remoteIP">The remote IP.</param>
        /// <param name="remoteMask">The remote network mask.</param>
        /// <returns><see langword="true"/> if successful, otherwise <see langword="false"/>.</returns>
        public bool ConfigTun(string localIP, string remoteIP, string remoteMask)
        {
            var configTun = new byte[12];
            BinaryPrimitives.WriteUInt32LittleEndian(configTun.AsSpan(0, 4), ParseIP(localIP));
            BinaryPrimitives.WriteUInt32LittleEndian(configTun.AsSpan(4, 4), ParseIP(remoteIP));
            BinaryPrimitives.WriteUInt32LittleEndian(configTun.AsSpan(8, 4), ParseIP(remoteMask));

            bool result = NativeMethods.DeviceIoControl(
                this._tapHandle, TapControlCode.TAP_IOCTL_CONFIG_TUN,
                configTun, configTun.Length,
                IntPtr.Zero, 0, out _, IntPtr.Zero);

            return result;
        }


        private static uint ParseIP(string ipAddress)
        {
            var ipBytes = IPAddress.Parse(ipAddress).GetAddressBytes();
            return BinaryPrimitives.ReadUInt32LittleEndian(ipBytes);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!_disposedValue)
            {
                if (disposing)
                {
                    this._tapHandle.Dispose();
                }

                this._disposedValue = true;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
            this.Dispose(disposing: true);
            GC.SuppressFinalize(this);
        }
    }
}
