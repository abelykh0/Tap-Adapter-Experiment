using System;
using TapWindows;
using System.Threading.Tasks;
using System.Globalization;
using System.Net.Sockets;
using System.Collections.Generic;
using System.Text;
using System.Buffers.Binary;
using System.Threading;
using Enclave.FastPacket;
using System.IO;

namespace TestApp
{
    internal sealed class Program
    {
        private static Dictionary<int, Connection> _relays = new Dictionary<int, Connection>();

        static async Task Main(string[] args)
        {
            int port = 30002;

            await SetupAndroidAsync(@"C:\Temp\platform-tools\adb.exe", port);

            var tap = SetupTapDevice();

            await DoWork(tap, port);

            Console.WriteLine("Hello, World!");
        }

        private static TapAdapter SetupTapDevice()
        {
            var result = new TapAdapter();

            bool success = result.ConfigDHCP("172.16.1.1", "255.255.255.0", "172.16.1.254",
                "172.16.1.0", ["8.8.8.8", "8.8.4.4"]);
            if (!success)
            {
                throw new NotSupportedException();
            }

            success = result.SetMediaStatus(connected: true);
            if (!success)
            {
                throw new NotSupportedException();
            }

            success = result.ConfigTun("172.16.1.1", "172.16.1.0", "255.255.255.0");
            if (!success)
            {
                throw new NotSupportedException();
            }

            return result;
        }

        private static async Task SetupAndroidAsync(string adbPath, int port)
        {
            // Get the connected device
            // Note: if the adb server is not started, it will start it as well
            var results = await RunProcessAsTask.ProcessEx.RunAsync(adbPath, "devices");
            string device = results.StandardOutput[1].Split('\t')[0];
            if (string.IsNullOrEmpty(device))
            {
                throw new NotSupportedException("The phone is not connected.");
            }

            // Set up forwarding
            var command = string.Format(CultureInfo.InvariantCulture, "forward tcp:{0} tcp:30002", port);
            results = await RunProcessAsTask.ProcessEx.RunAsync(adbPath, command);
        }

        private static async Task DoWork(TapAdapter tap, int port)
        {
            var relayConnection = new TcpClient("127.0.0.1", port)
            {
                NoDelay = true
            };

            using var stream = relayConnection.GetStream();

            var firstPacket = new byte[9];
            BinaryPrimitives.WriteInt32BigEndian(firstPacket.AsSpan(0, 4), 6); // identifier=6 (version)
            firstPacket[4] = (byte)CommandKind.Version;
            BinaryPrimitives.WriteInt32BigEndian(firstPacket.AsSpan(5, 4), 0); // size=0
            await stream.WriteAsync(firstPacket);

            Thread.Sleep(500);

            var secondPacket = new byte[18];
            BinaryPrimitives.WriteInt32BigEndian(secondPacket.AsSpan(0, 4), 5000);  // identifier=5000 (source port)
            secondPacket[4] = (byte)CommandKind.Ready;
            BinaryPrimitives.WriteInt32BigEndian(secondPacket.AsSpan(5, 4), 9);     // size=9
            secondPacket[9] = (byte)ProtocolType.Tcp;
            BinaryPrimitives.WriteInt32BigEndian(secondPacket.AsSpan(10, 4), 9);    // destination IP
            BinaryPrimitives.WriteInt32BigEndian(secondPacket.AsSpan(14, 4), 5000); // destination port
            await stream.WriteAsync(secondPacket);

            //var tcpCatcherPort = 50001;
            //var tcpCatcher = new TcpListener(IPAddress.Parse("172.16.1.1"), tcpCatcherPort);
            //tcpCatcher.Start();
            //_ = RunConnectionAsync(tcpCatcher);

            while (true)
            {
                var buffer = new byte[4096];
                var readBytes = await tap.ReadAsync(buffer, buffer.Length);
                if (readBytes > 0)
                {
                    var connection = CreateConnection(buffer, readBytes);
                    if (connection != null)
                    {
                        relays[connection.Identifier] = connection;
                    }
                }
            }

            // Header is
            // .word32be('identifier')
            // .word8('command')
            // .word32be('size');
            // command 0: close
            // command 1: data
            // command 2: ready
            // command 3: version
        }

        // From phone
        private static async Task HandleRelayData(NetworkStream relayStream)
        {
            var data = new MemoryStream();
            while (true)
            {
                data.Position = data.Length;

                var buffer = new Memory<byte>();
                var bytesRead = await relayStream.ReadAsync(buffer);
                data.Write(buffer.Slice(0, bytesRead).Span);

                if (data.Length < 9)
                {
                    continue;
                }

                var header = data.ToArray();
                var identifier = BinaryPrimitives.ReadInt32BigEndian(header.AsSpan(0, 4));
                var command = (CommandKind)header[4];
                var size = BinaryPrimitives.ReadInt32BigEndian(header.AsSpan(5, 4));

                if (_relays.TryGetValue(identifier, out Connection? connection))
                {

                }

                var connnection = _relays
            }
        }

        private static int CreateConnection(byte[] buffer, int length)
        {
            Ipv4PacketSpan ipPacket = new(buffer.AsSpan(0, length));
            return Connection.Create(ref ipPacket);
        }

        private static Connection? CreateConnection(byte[] buffer, int length)
        {
            Ipv4PacketSpan ipPacket = new(buffer.AsSpan(0, length));
            return Connection.Create(ref ipPacket);
        }

        private static void PrintPacket(byte[] buffer, int length)
        {
            Ipv4PacketSpan ipPacket = new(buffer.AsSpan(0, length));

            switch (ipPacket.Protocol)
            {
                case IpProtocol.Tcp:
                    TcpPacketSpan tcpPacket = new(ipPacket.Payload);
                    Console.WriteLine("TCP from {0}:{1}, to {2}:{3}, data {4}",
                        ipPacket.Source.ToString(), tcpPacket.SourcePort,
                        ipPacket.Destination.ToString(), tcpPacket.DestinationPort,
                        Encoding.UTF8.GetString(tcpPacket.Payload));
                    break;

                case IpProtocol.Udp:
                    //UdpPacketSpan udpPacket = new(ipPacket.Payload);
                    //Console.WriteLine("UDP from {0}:{1}, to {2}:{3}, data {4}",
                    //    ipPacket.Source.ToString(), udpPacket.SourcePort,
                    //    ipPacket.Destination.ToString(), udpPacket.DestinationPort,
                    //    Encoding.UTF8.GetString(udpPacket.Payload));
                    break;

                default:
                    break;
            }

        }

        private static async Task RunConnectionAsync(TcpListener catcher)
        {
            while (true)
            {
                TcpClient handler = await catcher.AcceptTcpClientAsync();
                NetworkStream stream = handler.GetStream();

                while (true)
                {
                    var buffer = new Memory<byte>();
                    int bytesRead = await stream.ReadAsync(buffer);
                    if (bytesRead > 0)
                    {
                        Console.WriteLine("From phone {0}", Encoding.ASCII.GetString(buffer.ToArray()));
                    }
                }
            }
        }
    }
}

