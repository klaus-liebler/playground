using Device.Net;
using Hid.Net;
using Hid.Net.Windows;
using Microsoft.Extensions.Logging;
using System.Linq;
using System.Threading.Tasks;


namespace Usb.Net.WindowsSample
{
    internal class Program
    {
        private static async Task Main()
        {
            //Create logger factory that will pick up all logs and output them in the debug output window
            var loggerFactory = LoggerFactory.Create((builder) =>
            {
                _ = builder.AddDebug().SetMinimumLevel(LogLevel.Trace);
            });

            //----------------------

            // This is Windows specific code. You can replace this with your platform of choice or put this part in the composition root of your app

            //Register the factory for creating Hid devices. 
            var hidFactory =
                new FilterDeviceDefinition(vendorId: 0xCAFE, productId: 0x4005)
                .CreateWindowsHidDeviceFactory(loggerFactory);



            //Get connected device definitions
            var deviceDefinitions = (await hidFactory.GetConnectedDeviceDefinitionsAsync().ConfigureAwait(false)).ToList();

            if (deviceDefinitions.Count == 0)
            {
                Console.WriteLine("No devices were found");
                return;
            }

            //Get the device from its definition
            var myDevice = (IHidDevice)await hidFactory.GetDeviceAsync(deviceDefinitions.First()).ConfigureAwait(false);

            //Initialize the device
            await myDevice.InitializeAsync().ConfigureAwait(false);

            myDevice.ReadReportAsync()
            
            //Create the request buffer
            var buffer = new byte[65];
            buffer[0] = 0x00;
            buffer[1] = 0x3f;
            buffer[2] = 0x23;
            buffer[3] = 0x23;

            //Write and read the data to the device
            var readBuffer = await myDevice.WriteAndReadAsync(buffer).ConfigureAwait(false);
        }
    }
}