using Enclave.FastPacket;
using System;
using System.Net.Sockets;

namespace TestApp
{
    internal class PacketInfo
    {
        public PacketInfo(
            ProtocolType protocolType,
            ValueIpAddress destination,
            int destinationPort,
            ValueIpAddress source,
            int sourcePort,
            byte iHL)
        {
            Protocol = protocolType;
            Destination = destination;
            DestinationPort = destinationPort;
            Source = source;
            SourcePort = sourcePort;
            this.IHL = iHL;
        }

        public PacketInfo(byte[] buffer, int length)
        {
            Ipv4PacketSpan ipPacket = new(buffer.AsSpan(0, length));
            Source = ipPacket.Source;
            Destination = ipPacket.Destination;
            IHL = ipPacket.IHL;

            switch (ipPacket.Protocol)
            {
                case IpProtocol.Tcp:
                    Protocol = ProtocolType.Tcp;
                    TcpPacketSpan tcpPacket = new(ipPacket.Payload);
                    SourcePort = tcpPacket.SourcePort;
                    DestinationPort = tcpPacket.DestinationPort;
                    break;

                case IpProtocol.Udp:
                    Protocol = ProtocolType.Udp;
                    UdpPacketSpan udpPacket = new(ipPacket.Payload);
                    SourcePort = udpPacket.SourcePort;
                    DestinationPort = udpPacket.DestinationPort;
                    break;

                default:
                    Protocol = ProtocolType.Unknown;
                    break;
            }
        }

        public ProtocolType Protocol { get; }

        public ValueIpAddress Destination { get; }
        public int DestinationPort { get; }

        public ValueIpAddress Source { get; }
        public int SourcePort { get; }

        public byte IHL { get; }
    }
}
