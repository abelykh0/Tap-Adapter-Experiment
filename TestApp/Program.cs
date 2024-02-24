using System;
using AdvancedSharpAdbClient.Models;
using AdvancedSharpAdbClient;
using TapWindows;
using System.Linq;
using AdvancedSharpAdbClient.DeviceCommands;
using AdvancedSharpAdbClient.Receivers;
using System.Globalization;

namespace TestApp
{
    internal class Program
    {
        private static TapAdapter _tap;

        static void Main(string[] args)
        {
            SetupAndroid();
            SetupTapDevice();

            Console.WriteLine("Hello, World!");
        }

        private static void SetupTapDevice()
        {
            _tap = new TapAdapter();

            bool success = _tap.ConfigDHCP("172.16.1.1", "255.255.255.0", "172.16.1.254",
                "172.16.1.0", ["8.8.8.8", "8.8.4.4"]);
            if (!success)
            {
                throw new NotSupportedException();
            }

            success = _tap.SetMediaStatus(connected: true);
            if (!success)
            {
                throw new NotSupportedException();
            }

            success = _tap.ConfigTun("172.16.1.1", "172.16.1.0", "255.255.255.0");
            if (!success)
            {
                throw new NotSupportedException();
            }
        }

        private static void SetupAndroid()
        {
            if (!AdbServer.Instance.GetStatus().IsRunning)
            {
                var server = new AdbServer();
                var result = server.StartServer(@"C:\Temp\platform-tools\adb.exe", false);
                if (result != StartServerResult.Started)
                {
                    throw new NotSupportedException("Can't start adb server");
                }
            }

            var adbServer = (AdbServer)AdbServer.Instance;
            adbServer.StartServerAsync

            var adbClient = new AdbClient(adbServer.EndPoint);
            var device = adbClient.GetDevices().FirstOrDefault();
            if (device == null)
            {
                throw new NotSupportedException("Phone is not connected");
            }

            int port = 30002;

            var receiver1 = new ConsoleOutputReceiver() { TrimLines = true, ParsesErrors = false };
            var command = string.Format(CultureInfo.InvariantCulture, "forward tcp:{0} tcp:30002", port);
            adbClient.ExecuteServerCommand(string.Empty, command, receiver1);

            var receiver2 = new ConsoleOutputReceiver() { TrimLines = true, ParsesErrors = false };
            adbClient.ExecuteShellCommand(device, "am start -n com.koushikdutta.tether/com.koushikdutta.tether.TetherActivity", receiver2);

            var receiver3 = new ConsoleOutputReceiver() { TrimLines = true, ParsesErrors = false };
            adbClient.ExecuteShellCommand(device, "am start -n com.koushikdutta.tether/com.koushikdutta.tether.TetherActivity", receiver3);


        }
    }
}
