namespace SampleContainer
{
    using System;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Runtime.Loader;
    using System.Security.Cryptography.X509Certificates;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Devices.Client;
    using Microsoft.Azure.Devices.Client.Transport.Mqtt;
    using Microsoft.Azure.Devices.Shared;

    class Program
    {
        // static int counter;

        private static ModuleClient moduleClient;

        private static Timer timer;

        static void Main(string[] args)
        {
            Init().Wait();

            // Wait until the app unloads or is cancelled
            var cts = new CancellationTokenSource();
            AssemblyLoadContext.Default.Unloading += (ctx) => cts.Cancel();
            Console.CancelKeyPress += (sender, cpe) => cts.Cancel();
            WhenCancelled(cts.Token).Wait();
        }

        /// <summary>
        /// Handles cleanup operations when app is cancelled or unloads
        /// </summary>
        public static Task WhenCancelled(CancellationToken cancellationToken)
        {
            var tcs = new TaskCompletionSource<bool>();
            cancellationToken.Register(s => ((TaskCompletionSource<bool>)s).SetResult(true), tcs);
            return tcs.Task;
        }

        /// <summary>
        /// Initializes the ModuleClient and sets up the callback to receive
        /// messages containing temperature information
        /// </summary>
        static async Task Init()
        {
            Console.WriteLine("Initialized");
            timer = new Timer(new TimerCallback(Program.SleepEvent), null, 5000 /*dueTime*/, 5000 /*period*/);
        }

        private static void SleepEvent(object stateInfo)
        {
            try
            {   
                    Console.WriteLine("Sleep event.");
                    // moduleClient.SendEventAsync("telemetry", new Message(Encoding.UTF8.GetBytes("abc")));

                    // Twin t = moduleClient.GetTwinAsync().Result;
            }
            catch (Exception exception)
            {
                Console.WriteLine(exception);
                return;
            }
        }

        static void OnConnectionStatusChanged(ConnectionStatus status, ConnectionStatusChangeReason reason)
        {
            try
            {
                Console.WriteLine($"ConnectionStatus:{status}");
                Console.WriteLine($"ConnectionStatusChangeReason:{reason}");
            }
            catch (Exception ex)
            {
                 Console.WriteLine(ex);
            }
        }
    }
}
