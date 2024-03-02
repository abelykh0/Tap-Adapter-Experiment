using System;
using System.Net.Sockets;
using Enclave.FastPacket;

namespace TestApp
{
    internal sealed class Connection : IDisposable
    {
        private bool disposedValue;

        public int Identifier
        {
            get
            {
                var result = SourcePort;
                if (Protocol == ProtocolType.Udp)
                {
                    result |= 0x010000;
                }

                return result;
            }
        }

        public ProtocolType Protocol { get; }
        public ValueIpAddress Source { get; }
        public int SourcePort { get; }
        public ValueIpAddress Destination { get; }
        public int DestinationPort { get; }

        private Connection(
            ProtocolType protocolType,
            ValueIpAddress source,
            int sourcePort,
            ValueIpAddress destination,
            int destinatinPort)
        {
            this.Source = source;
            this.SourcePort = sourcePort;
            this.Protocol = protocolType;
            this.Destination = destination;
            this.DestinationPort = destinatinPort;
        }

        public static Connection? Create(ref Ipv4PacketSpan ipPacket)
        {
            switch (ipPacket.Protocol)
            {
                case IpProtocol.Tcp:
                    TcpPacketSpan tcpPacket = new(ipPacket.Payload);
                    return new Connection(ProtocolType.Tcp, ipPacket.Source,
                        tcpPacket.SourcePort, ipPacket.Destination, tcpPacket.DestinationPort);

                case IpProtocol.Udp:
                    UdpPacketSpan udpPacket = new(ipPacket.Payload);
                    return new Connection(ProtocolType.Udp, ipPacket.Source,
                        udpPacket.SourcePort, ipPacket.Destination, udpPacket.DestinationPort);

                default:
                    break;
            }

            return null;
        }

        public void Write(byte[] data)
        {

        }

        public void Accept()
        {

        }

        // command 
        public void Ready()
        {

        }

        public void Close()
        {

        }

        private void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    // TODO: dispose managed state (managed objects)
                }

                // TODO: free unmanaged resources (unmanaged objects) and override finalizer
                // TODO: set large fields to null
                disposedValue = true;
            }
        }

        public void Dispose()
        {
            // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
            Dispose(disposing: true);
            GC.SuppressFinalize(this);
        }

        //public Socket Socket { get; set; }
    }
}
