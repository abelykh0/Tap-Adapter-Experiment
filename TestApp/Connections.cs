using System;
using System.Net.Sockets;

namespace TestApp
{
    internal abstract class Connections
    {
        public abstract ProtocolType ProtocolType { get; protected set; }

        public abstract bool IsCatcherPort(int port);

        public abstract Connection this[int port] { get; }
    }
}
