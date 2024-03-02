using System;

namespace TestApp
{
    internal class RelayPacket
    {
        public int Identifier { get; set; }
        public CommandKind Command { get; set; }
        public byte[] Data { get; set; } = null!;
    }
}
