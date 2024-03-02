﻿using System;
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
using System.Net;

namespace TestApp
{
    internal sealed class Program
    {
        private static readonly ValueIpAddress TunIp = ValueIpAddress.Create(IPAddress.Parse("172.16.1.1"));

        private const int RelayPort = 30002;

        private const int CatcherPort = 50001;

        private static Dictionary<int, Connection> _relays = [];

        static async Task Main(string[] args)
        {
            await SetupAndroidAsync(@"C:\Temp\platform-tools\adb.exe", RelayPort);

            var tap = SetupTapDevice();

            await DoWork(tap, RelayPort);

            Console.WriteLine("Hello, World!");
        }

        private static TapAdapter SetupTapDevice()
        {
            var result = new TapAdapter();

            string ipAddress = TunIp.ToString();

            bool success = result.ConfigDHCP(ipAddress, "255.255.255.0", "172.16.1.254",
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

            success = result.ConfigTun(ipAddress, "172.16.1.0", "255.255.255.0");
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
                        _relays[connection.Identifier] = connection;
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

        // From TAP (infinite loop)
        private static async Task HandleTapData(TapAdapter tap)
        {
            while (true)
            {
                var buffer = new byte[4096];
                var readBytes = await tap.ReadAsync(buffer, buffer.Length);
                if (readBytes > 0)
                {
                    var packetInfo = new PacketInfo(buffer, readBytes);

                    // see if this packet is originating from a traffic catcher
                    if (packetInfo.SourcePort == CatcherPort)
                    {
                        if (!_relays.TryGetValue(packetInfo.DestinationPort, out Connection? connection))
                        {
                            // unknown connection
                            continue;
                        }

                        var rewritePacketInfo = new PacketInfo(connection.Protocol, 
                            connection.Destination, connection.DestinationPort, 
                            connection.Source, connection.SourcePort);
                        RewritePacket(rewritePacketInfo);
                    }
                    else
                    {
                        // see if this packet is already part of an accepted connection
                        // and needs to be directed to a traffic catcher

                        if (!_relays.TryGetValue(packetInfo.SourcePort, out Connection? connection))
                        {
                            // unknown connection
                            continue;
                        }

                        var rewritePacketInfo = new PacketInfo(packetInfo.Protocol, 
                            packetInfo.Destination, packetInfo.SourcePort, 
                            packetInfo.Source, CatcherPort);
                        RewritePacket(rewritePacketInfo);
                    }

                    //CreateConnection(buffer, readBytes);
                }
            }
        }

        private static void RewritePacket(PacketInfo packetInfo)
        {
        }

        // From phone (infinite loop)
        private static void HandleRelayData(NetworkStream relayStream)
        {
            while (true)
            {
                var packet = GetNextPacket(relayStream);

                if (packet.Command == CommandKind.Version)
                {
                    // received phone Tether version
                    continue;
                }

                if (!_relays.TryGetValue(packet.Identifier, out Connection? connection))
                {
                    // unknown connection
                    continue;
                }

                switch (packet.Command)
                {
                    case CommandKind.Close:
                        _relays.Remove(packet.Identifier);
                        break;

                    case CommandKind.Ready:
                        connection.Ready();
                        break;

                    case CommandKind.Data:
                        connection.Write(packet.Data);
                        break;
                }
            }
        }

        private static RelayPacket GetNextPacket(NetworkStream relayStream)
        {
            while (true)
            {
                var data = new MemoryStream();

                while (data.Length < 9)
                {
                    int dataByte = relayStream.ReadByte();
                    if (dataByte == -1)
                    {
                        continue;
                    }

                    data.WriteByte((byte)dataByte);
                }

                // Header received
                var header = data.ToArray();
                var identifier = BinaryPrimitives.ReadInt32BigEndian(header.AsSpan(0, 4));
                var command = (CommandKind)header[4];
                var size = BinaryPrimitives.ReadInt32BigEndian(header.AsSpan(5, 4));

                // Read the rest
                while (data.Length < 9 + size)
                {
                    int dataByte = relayStream.ReadByte();
                    if (dataByte == -1)
                    {
                        continue;
                    }

                    data.WriteByte((byte)dataByte);
                }

                return new RelayPacket()
                {
                    Identifier = identifier,
                    Command = command,
                    Data = data.ToArray()
                };
            }
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
    }
}

