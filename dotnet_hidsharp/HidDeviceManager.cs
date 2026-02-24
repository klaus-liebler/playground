using HidSharp;
using System.Text;

namespace UsbHidCommunicator
{
    public class HidDeviceManager : IDisposable
    {
        private HidDevice? _device;
        private HidStream? _stream;
        private Thread? _readThread;
        private CancellationTokenSource? _cts;
        private readonly object _lockObject = new();
        
        public event EventHandler<byte[]>? DataReceived;
        public event EventHandler<bool>? ConnectionChanged;
        
        // Geräteeigenschaften
        public int VendorId { get; private set; }
        public int ProductId { get; private set; }
        public string? SerialNumber { get; private set; }
        public bool IsConnected => _stream != null && _stream.CanRead;
        
        // Statistik
        public long BytesReceived { get; private set; }
        public long BytesSent { get; private set; }
        public DateTime LastCommunication { get; private set; }
        
        /// <summary>
        /// Nach Gerät suchen und verbinden
        /// </summary>
        public bool Connect(int vendorId, int productId, string? serialNumber = null)
        {
            try
            {
                Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] Suche Gerät VID={vendorId:X4}, PID={productId:X4}...");
                
                var deviceList = DeviceList.Local;
                var devices = deviceList.GetHidDevices()
                    .Where(d => d.VendorID == vendorId && d.ProductID == productId)
                    .ToList();
                
                if (!devices.Any())
                {
                    Console.WriteLine("Kein passendes Gerät gefunden.");
                    LogAvailableDevices();
                    return false;
                }
                
                Console.WriteLine($"Gefundene Geräte: {devices.Count}");
                
                // Gerät auswählen (mit Seriennummer falls angegeben)
                if (!string.IsNullOrEmpty(serialNumber))
                {
                    _device = devices.FirstOrDefault(d => 
                        d.GetSerialNumber()?.Equals(serialNumber, StringComparison.OrdinalIgnoreCase) == true);
                }
                else
                {
                    _device = devices.FirstOrDefault();
                }
                
                if (_device == null)
                {
                    Console.WriteLine($"Gerät nicht gefunden (Serial: {serialNumber ?? "any"})");
                    return false;
                }
                
                // Geräteinformationen ausgeben
                PrintDeviceInfo(_device);
                
                // Verbindung öffnen
                if (!_device.TryOpen(out _stream))
                {
                    Console.WriteLine("Konnte Gerät nicht öffnen.");
                    ShowPlatformSpecificHelp();
                    return false;
                }
                
                // Stream konfigurieren
                _stream.ReadTimeout = Timeout.Infinite;
                
                // Eigenschaften speichern
                VendorId = vendorId;
                ProductId = productId;
                SerialNumber = _device.GetSerialNumber();
                
                // Lese-Thread starten
                StartReading();
                
                Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] Verbunden: {_device.DevicePath}");
                ConnectionChanged?.Invoke(this, true);
                
                return true;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Verbindungsfehler: {ex.Message}");
                if (ex.InnerException != null)
                {
                    Console.WriteLine($"Inner Exception: {ex.InnerException.Message}");
                }
                return false;
            }
        }
        
        /// <summary>
        //Asynchroner Verbindungsversuch mit automatischen Reconnects
        /// </summary>
        public async Task<bool> ConnectWithRetryAsync(
            int vendorId, 
            int productId, 
            int maxRetries = 5, 
            int retryDelayMs = 2000,
            CancellationToken cancellationToken = default)
        {
            for (int attempt = 1; attempt <= maxRetries; attempt++)
            {
                if (cancellationToken.IsCancellationRequested)
                    return false;
                
                Console.WriteLine($"Verbindungsversuch {attempt}/{maxRetries}...");
                
                if (Connect(vendorId, productId))
                    return true;
                
                if (attempt < maxRetries)
                {
                    Console.WriteLine($"Warte {retryDelayMs}ms vor nächstem Versuch...");
                    await Task.Delay(retryDelayMs, cancellationToken);
                }
            }
            
            return false;
        }
        
        private void StartReading()
        {
            _cts = new CancellationTokenSource();
            _readThread = new Thread(ReadLoop)
            {
                IsBackground = true,
                Name = "HID-ReadThread"
            };
            _readThread.Start();
        }
        
        private void ReadLoop()
        {
            if (_stream == null || _device == null) return;
            
            var buffer = new byte[_device.GetMaxInputReportLength()];
            Console.WriteLine($"Lesepuffer-Größe: {buffer.Length} Bytes");
            
            while (!_cts?.Token.IsCancellationRequested ?? false)
            {
                try
                {
                    // Blockierendes Lesen (mit Timeout)
                    _stream.ReadTimeout = 1000;
                    int bytesRead = _stream.Read(buffer, 0, buffer.Length);
                    
                    if (bytesRead > 0)
                    {
                        var receivedData = new byte[bytesRead];
                        Array.Copy(buffer, receivedData, bytesRead);
                        
                        BytesReceived += bytesRead;
                        LastCommunication = DateTime.Now;
                        
                        // Ereignis auslösen
                        DataReceived?.Invoke(this, receivedData);
                        
                        // Optional: In Hex ausgeben
                        if (bytesRead > 0)
                        {
                            LogReceivedData(receivedData);
                        }
                    }
                }
                catch (TimeoutException)
                {
                    // Timeout ist normal, Loop fortsetzen
                    continue;
                }
                catch (IOException ex)
                {
                    Console.WriteLine($"E/A-Fehler: {ex.Message}");
                    if (ex.InnerException is System.ComponentModel.Win32Exception win32Ex)
                    {
                        // Windows-spezifische Fehlercodes
                        Console.WriteLine($"Win32 Error: {win32Ex.NativeErrorCode}");
                    }
                    break;
                }
                catch (ObjectDisposedException)
                {
                    // Stream wurde geschlossen
                    break;
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Lesefehler: {ex.Message}");
                    Thread.Sleep(100);
                }
            }
            
            Console.WriteLine("Lese-Thread beendet.");
        }
        
        /// <summary>
        /// Daten an das Gerät senden
        /// </summary>
        public bool SendData(byte[] data, byte reportId = 0x00)
        {
            lock (_lockObject)
            {
                if (_stream == null || !_stream.CanWrite)
                {
                    Console.WriteLine("Keine aktive Verbindung zum Senden.");
                    return false;
                }
                
                try
                {
                    // Report-Byte hinzufügen (für HID-Geräte typisch)
                    int reportLength = _device?.GetMaxOutputReportLength() ?? 65;
                    byte[] reportData = new byte[reportLength];
                    
                    // Report ID setzen (oft 0x00)
                    reportData[0] = reportId;
                    
                    // Daten kopieren (beginnend bei Index 1)
                    int bytesToCopy = Math.Min(data.Length, reportLength - 1);
                    Array.Copy(data, 0, reportData, 1, bytesToCopy);
                    
                    // Senden
                    _stream.Write(reportData);
                    
                    BytesSent += bytesToCopy;
                    LastCommunication = DateTime.Now;
                    
                    Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] Gesendet: {FormatHexData(reportData, bytesToCopy + 1)}");
                    
                    return true;
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Sendefehler: {ex.Message}");
                    return false;
                }
            }
        }
        
        /// <summary>
        /// String als Daten senden (UTF-8 kodiert)
        /// </summary>
        public bool SendString(string text, byte reportId = 0x00)
        {
            byte[] data = Encoding.UTF8.GetBytes(text);
            return SendData(data, reportId);
        }
        

        
        /// <summary>
        /// Alle verfügbaren HID-Geräte auflisten
        /// </summary>
        public static void ListAllHidDevices()
        {
            Console.WriteLine("\n=== Verfügbare HID-Geräte ===");
            
            var deviceList = DeviceList.Local;
            var devices = deviceList.GetHidDevices().ToList();
            
            if (!devices.Any())
            {
                Console.WriteLine("Keine HID-Geräte gefunden.");
                return;
            }
            
            foreach (var device in devices)
            {
                Console.WriteLine($"- VID: {device.VendorID:X4}, PID: {device.ProductID:X4}");
                Console.WriteLine($"  Pfad: {device.DevicePath}");
                try
                {
                    Console.WriteLine($"  Seriennummer: {device.GetSerialNumber() ?? "N/A"}");
                    Console.WriteLine($"  Hersteller: {device.GetManufacturer() ?? "N/A"}");
                    Console.WriteLine($"  Produkt: {device.GetProductName() ?? "N/A"}");
                    Console.WriteLine($"  Input: {device.GetMaxInputReportLength()}B, " +
                                     $"Output: {device.GetMaxOutputReportLength()}B, " +
                                     $"Feature: {device.GetMaxFeatureReportLength()}B");
                }
                catch
                {
                    Console.WriteLine("  Error retrieving device info.");
                }

                Console.WriteLine();
            }
        }
        
        public void Disconnect()
        {
            lock (_lockObject)
            {
                Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] Trenne Verbindung...");
                
                _cts?.Cancel();
                
                // Thread beenden
                if (_readThread != null && _readThread.IsAlive)
                {
                    if (!_readThread.Join(1000))
                    {
                        Console.WriteLine("Warnung: ReadThread konnte nicht sauber beendet werden.");
                    }
                }
                
                // Stream schließen
                _stream?.Close();
                _stream?.Dispose();
                _stream = null;
                
                _device = null;
                _cts?.Dispose();
                _cts = null;
                
                ConnectionChanged?.Invoke(this, false);
                Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] Verbindung getrennt.");
            }
        }
        
        private void PrintDeviceInfo(HidDevice device)
        {
            Console.WriteLine("\n=== Geräteinformationen ===");
            Console.WriteLine($"Gerätepfad: {device.DevicePath}");
            Console.WriteLine($"VID/PID: {device.VendorID:X4}:{device.ProductID:X4}");
            Console.WriteLine($"Hersteller: {device.GetManufacturer() ?? "Unbekannt"}");
            Console.WriteLine($"Produkt: {device.GetProductName() ?? "Unbekannt"}");
            Console.WriteLine($"Seriennummer: {device.GetSerialNumber() ?? "Keine"}");
            Console.WriteLine($"Input Report: {device.GetMaxInputReportLength()} Bytes");
            Console.WriteLine($"Output Report: {device.GetMaxOutputReportLength()} Bytes");
            Console.WriteLine($"Feature Report: {device.GetMaxFeatureReportLength()} Bytes");
            Console.WriteLine();
        }
        
        private void LogReceivedData(byte[] data)
        {
            Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] Empfangen ({data.Length} Bytes):");
            Console.WriteLine($"  HEX: {FormatHexData(data)}");
            Console.WriteLine($"  ASCII: {FormatAsciiData(data)}");
            Console.WriteLine($"  DEC: {string.Join(" ", data.Select(b => b.ToString().PadLeft(3)))}");
        }
        
        private string FormatHexData(byte[] data, int? length = null)
        {
            int len = length ?? data.Length;
            return BitConverter.ToString(data, 0, len).Replace("-", " ");
        }
        
        private string FormatAsciiData(byte[] data)
        {
            var sb = new StringBuilder();
            foreach (byte b in data)
            {
                if (b >= 32 && b <= 126)
                    sb.Append((char)b);
                else
                    sb.Append('.');
            }
            return sb.ToString();
        }
        
        private void LogAvailableDevices()
        {
            Console.WriteLine("\nVerfügbare HID-Geräte:");
            var devices = DeviceList.Local.GetHidDevices();
            foreach (var dev in devices.Take(10)) // Nur erste 10 anzeigen
            {
                Console.WriteLine($"  {dev.VendorID:X4}:{dev.ProductID:X4} - {dev.GetProductName() ?? "Unbekannt"}");
            }
            if (devices.Count() > 10)
                Console.WriteLine($"  ... und {devices.Count() - 10} weitere");
        }
        
        private void ShowPlatformSpecificHelp()
        {
            if (Environment.OSVersion.Platform == PlatformID.Win32NT)
            {
                Console.WriteLine("\nWindows-Hilfe:");
                Console.WriteLine("- Administratorrechte prüfen");
                Console.WriteLine("- Gerätemanager: Prüfen ob Gerät erkannt wird");
                Console.WriteLine("- Treiber aktualisieren");
            }
            else
            {
                Console.WriteLine("\nLinux-Hilfe:");
                Console.WriteLine("1. Geräte anzeigen: lsusb");
                Console.WriteLine("2. HID-Geräte anzeigen: ls -la /dev/hidraw*");
                Console.WriteLine("3. Temporäre Berechtigung: sudo chmod 666 /dev/hidraw0");
                Console.WriteLine("4. Permanente udev-Regel:");
                Console.WriteLine("   sudo nano /etc/udev/rules.d/99-myhid.rules");
                Console.WriteLine("   Inhalt:");
                Console.WriteLine("   SUBSYSTEM==\"usb\", ATTRS{idVendor}==\"XXXX\", ATTRS{idProduct}==\"YYYY\", MODE=\"0666\"");
                Console.WriteLine("   SUBSYSTEM==\"hidraw\", ATTRS{idVendor}==\"XXXX\", ATTRS{idProduct}==\"YYYY\", MODE=\"0666\"");
                Console.WriteLine("5. Regeln neu laden:");
                Console.WriteLine("   sudo udevadm control --reload-rules");
                Console.WriteLine("   sudo udevadm trigger");
            }
        }
        
        public void Dispose()
        {
            Disconnect();
            GC.SuppressFinalize(this);
        }
        
        ~HidDeviceManager()
        {
            Disconnect();
        }
    }
}