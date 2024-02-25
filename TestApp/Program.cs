using System;
using TapWindows;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Globalization;
using System.Collections;
using System.Net.Sockets;
using System.Collections.Generic;
using System.Text;
using System.Buffers.Binary;
using System.Net;
using System.IO;
using System.Runtime.CompilerServices;
using System.Threading;
using PacketDotNet;
using PacketDotNet.Utils;

namespace TestApp
{
    internal sealed class Program
    {
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
            var relayConnection = new TcpClient("127.0.0.1", port);
            relayConnection.NoDelay = true;
            using var stream = relayConnection.GetStream();

            //var tcpCatcherPort = 50001;
            //var tcpCatcher = new Socket(SocketType.Stream, ProtocolType.Tcp);
            //tcpCatcher.NoDelay = true;
            //Thread.Sleep(500);
            //tcpCatcher.Bind(new IPEndPoint(IPAddress.Parse("172.16.1.1"), tcpCatcherPort));
            //tcpCatcher.Listen(tcpCatcherPort);

            var firstPacket = new byte[9];
            BinaryPrimitives.WriteInt32BigEndian(firstPacket.AsSpan(0, 4), 6); // identifier=6 (version)
            firstPacket[4] = 3; // command 3
            BinaryPrimitives.WriteInt32BigEndian(firstPacket.AsSpan(5, 4), 0); // size=0
            await stream.WriteAsync(firstPacket);

            var packet = new TcpPacket(30002, 30002);
            packet.PayloadData = firstPacket;

            await tap.WriteAsync(packet.Bytes);
            //tap.DeviceStream.Write(firstPacket);

            while (true)
            {
                //var socket = await tcpCatcher.AcceptAsync();
                //_ = RunConnectionAsync(socket);

                //var buffer = new Memory<byte>();
                //int readBytes = await stream.ReadAsync(buffer);
                //if (readBytes > 0)
                //{
                //    Console.WriteLine("from {0}:{1}, to {2}:{3}, data {4}",
                //        ipPacket.SourceAddress, packet.SourcePort,
                //        ipPacket.DestinationAddress, packet.DestinationPort,
                //        Encoding.UTF8.GetString(ipPacket.PayloadData));
                //}

                var buffer2 = new byte[4096];
                var readBytes2 = await tap.ReadAsync(buffer2, buffer2.Length);
                if (readBytes2 > 0)
                {
                    var ipPacket = new IPPacket(new ByteArraySegment(buffer2, 0, readBytes2));

                    Console.WriteLine("from {0}:{1}, to {2}:{3}, data {4}",
                        ipPacket.SourceAddress, packet.SourcePort,
                        ipPacket.DestinationAddress, packet.DestinationPort,
                        Encoding.UTF8.GetString(ipPacket.PayloadData));
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

        private static async Task RunConnectionAsync(Socket socket)
        {
            var buffer = new Memory<byte>();
            int readBytes = await socket.ReceiveAsync(buffer);
            if (readBytes > 0)
            {
                Console.WriteLine(Encoding.UTF8.GetString(buffer.Span));
            }
            ;
        }
    }
}
