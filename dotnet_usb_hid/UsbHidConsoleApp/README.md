# USB HID Console Application

This is a .NET console application that demonstrates communication with USB HID (Human Interface Device) devices.

## Features

- Lists all connected HID devices with their Vendor ID (VID) and Product ID (PID).
- Automatically selects the HID device with Vendor ID 0xCAFE (if available).
- Checks device capabilities for input and output reports.
- Sends an output report if supported (with sample data "Hello HID Device!").
- Reads an input report if supported (with a 5-second timeout).

## Prerequisites

- .NET 9.0 or later
- Windows operating system (HidLibrary is Windows-specific)

## Building and Running

1. Navigate to the project directory:
   ```
   cd UsbHidConsoleApp
   ```

2. Build the application:
   ```
   dotnet build
   ```

3. Run the application:
   ```
   dotnet run
   ```

## Dependencies

- HidLibrary: A .NET library for communicating with HID devices.

## HID Report Descriptor

The application is designed to work with HID devices that have the following report descriptor (as provided):

```
static uint8_t const desc_hid_report[] = {
  0x05, 0x01,       // Usage Page (Generic Desktop)
  0x09, 0x00,       // Usage (Undefined)
  0xA1, 0x01,       // Collection (Application)
  0x15, 0x00,       // Logical Minimum (0)
  0x26, 0xFF, 0x00, // Logical Maximum (255)
  0x75, 0x08,       // Report Size (8)
  0x95, 0x40,       // Report Count (64) - 64*8=512 bits = 64 bytes
  0x09, 0x00,       // Usage (Undefined)
  0x81, 0x02,       // Input (Data,Var,Abs)
  0x09, 0x00,       // Usage (Undefined)
  0x91, 0x02,       // Output (Data,Var,Abs)
  0xC0              // End Collection
};
```

This descriptor defines a generic HID device with 64-byte input and output reports.

## Notes

- The application selects the first HID device in the list for demonstration purposes. In a real-world scenario, you would select the device based on VID/PID or other criteria.
- Ensure that the HID device is properly connected and not in use by other applications.
- Some HID devices (like keyboards and mice) may not support output reports or may require special permissions.