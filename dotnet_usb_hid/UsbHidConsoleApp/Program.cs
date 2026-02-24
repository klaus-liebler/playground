using HidLibrary;
using System.Text;
using System.Threading;

Console.WriteLine("USB HID Console Application");

// List all HID devices
var devices = HidDevices.Enumerate().ToList();
if (devices.Count == 0)
{
    Console.WriteLine("No HID devices found.");
    return;
}

Console.WriteLine($"Found {devices.Count} HID device(s):");
for (int i = 0; i < devices.Count; i++)
{
    var dev = devices[i];
    Console.WriteLine($"{i}: VID={dev.Attributes.VendorId:X4}, PID={dev.Attributes.ProductId:X4}, {dev.DevicePath}");
}

// Select the device with Vendor ID 0xCAFE
var selectedDevice = devices.FirstOrDefault(d => d.Attributes.VendorId == 0xCAFE);
if (selectedDevice == null)
{
    Console.WriteLine("No HID device with Vendor ID 0xCAFE found.");
    return;
}
Console.WriteLine($"Selecting device: VID={selectedDevice.Attributes.VendorId:X4}, PID={selectedDevice.Attributes.ProductId:X4}");

selectedDevice.OpenDevice();
if (!selectedDevice.IsOpen)
{
    Console.WriteLine("Could not open the HID device.");
    return;
}

Console.WriteLine("Device opened successfully.");

Console.WriteLine("\n--- HID Device Capabilities ---");
var caps = selectedDevice.Capabilities;
Console.WriteLine($"Usage: {caps.Usage}");
Console.WriteLine($"Usage Page: {caps.UsagePage}");
Console.WriteLine($"Input Report Byte Length: {caps.InputReportByteLength}");
Console.WriteLine($"Output Report Byte Length: {caps.OutputReportByteLength}");
Console.WriteLine($"Feature Report Byte Length: {caps.FeatureReportByteLength}");
Console.WriteLine($"Number of Link Collection Nodes: {caps.NumberLinkCollectionNodes}");
Console.WriteLine($"Number of Input Button Caps: {caps.NumberInputButtonCaps}");
Console.WriteLine($"Number of Input Value Caps: {caps.NumberInputValueCaps}");
Console.WriteLine($"Number of Input Data Indices: {caps.NumberInputDataIndices}");
Console.WriteLine($"Number of Output Button Caps: {caps.NumberOutputButtonCaps}");
Console.WriteLine($"Number of Output Value Caps: {caps.NumberOutputValueCaps}");
Console.WriteLine($"Number of Output Data Indices: {caps.NumberOutputDataIndices}");
Console.WriteLine($"Number of Feature Button Caps: {caps.NumberFeatureButtonCaps}");
Console.WriteLine($"Number of Feature Value Caps: {caps.NumberFeatureValueCaps}");
Console.WriteLine($"Number of Feature Data Indices: {caps.NumberFeatureDataIndices}");
Console.WriteLine("-------------------------------\n");

Console.WriteLine("Starting continuous HID input report reading. Press Ctrl+C to stop.");

// Continuous loop for reading input reports
while (true) // Infinite loop, stop with Ctrl+C
{
    try
    {
        // Check if device supports input reports
        if (selectedDevice.Capabilities.InputReportByteLength > 0)
        {
            // Read an input report (blocking until received)
            var inputReport = selectedDevice.ReadReport();
            if (inputReport != null)
            {
                Console.WriteLine("Input report received:");
                Console.WriteLine(BitConverter.ToString(inputReport.Data));
            }
            else
            {
                Console.WriteLine("No input report received.");
            }
        }
        else
        {
            Console.WriteLine("Device does not support input reports.");
            break;
        }
    }
    catch (Exception ex)
    {
        Console.WriteLine($"Error in communication loop: {ex.Message}");
        break;
    }
}

Console.WriteLine("Test loop completed.");

// Close the device
selectedDevice.CloseDevice();
Console.WriteLine("Device closed.");
