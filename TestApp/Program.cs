using System;
using TapWindows;

namespace TestApp
{
    internal class Program
    {
        static void Main(string[] args)
        {
            var tap = new TapAdapter();

            bool success = tap.ConfigDHCP("172.16.1.1", "255.255.255.0", "172.16.1.254",
                "172.16.1.0", ["8.8.8.8", "8.8.4.4"]);
            if (!success)
            {
                throw new NotSupportedException();
            }

            success = tap.SetMediaStatus(connected: true);
            if (!success)
            {
                throw new NotSupportedException();
            }

            success = tap.ConfigTun("172.16.1.1", "172.16.1.0", "255.255.255.0");
            if (!success)
            {
                throw new NotSupportedException();
            }

            Console.WriteLine("Hello, World!");
        }
    }
}
