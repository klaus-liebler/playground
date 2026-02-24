using System.Text;

namespace UsbHidCommunicator
{
    class Program
    {
        private static HidDeviceManager? _hidManager;
        private static bool _running = true;
        
        static async Task Main(string[] args)
        {
            Console.Title = "USB HID Communicator";
            Console.OutputEncoding = Encoding.UTF8;
            
            // ASCII-Art Logo
            PrintHeader();
            
            // Verfügbare Geräte auflisten
            HidDeviceManager.ListAllHidDevices();
            
            // Beispiel: Arduino Leonardo (VID=0x2341, PID=0x8036)
            // Oder Custom Gerät: z.B. VID=0x1234, PID=0x5678
            int vid = 0xCAFE;  // Anpassen!
            int pid = 0x4005;  // Anpassen!
            
            // Manager instanziieren
            _hidManager = new HidDeviceManager();
            _hidManager.DataReceived += OnDataReceived;
            _hidManager.ConnectionChanged += OnConnectionChanged;
            
            // Tastenkürzel anzeigen
            PrintHelp();
            
            // Hauptschleife
            while (_running)
            {
                if (Console.KeyAvailable)
                {
                    var key = Console.ReadKey(true);
                    await HandleKeyPress(key, vid, pid);
                }
                
                await Task.Delay(50);
            }
            
            // Aufräumen
            _hidManager.Dispose();
            Console.WriteLine("\nProgramm beendet.");
        }
        
        private static async Task HandleKeyPress(ConsoleKeyInfo key, int vid, int pid)
        {
            switch (key.Key)
            {
                case ConsoleKey.C:
                    if (_hidManager != null && !_hidManager.IsConnected)
                    {
                        Console.WriteLine($"\nVerbinde mit VID={vid:X4}, PID={pid:X4}...");
                        await _hidManager.ConnectWithRetryAsync(vid, pid, maxRetries: 3);
                    }
                    else
                    {
                        Console.WriteLine("\nBereits verbunden oder Manager nicht initialisiert.");
                    }
                    break;
                    
                case ConsoleKey.D:
                    Console.WriteLine("\nTrenne Verbindung...");
                    _hidManager?.Disconnect();
                    break;
                    
                case ConsoleKey.L:
                    Console.WriteLine();
                    HidDeviceManager.ListAllHidDevices();
                    break;
                    
                case ConsoleKey.S:
                    if (_hidManager?.IsConnected == true)
                    {
                        Console.Write("\nNachricht eingeben: ");
                        var message = Console.ReadLine();
                        if (!string.IsNullOrEmpty(message))
                        {
                            bool success = _hidManager.SendString(message);
                            Console.WriteLine(success ? "✓ Gesendet" : "✗ Fehler beim Senden");
                        }
                    }
                    else
                    {
                        Console.WriteLine("\nNicht verbunden!");
                    }
                    break;
                    
                case ConsoleKey.T:
                    if (_hidManager?.IsConnected == true)
                    {
                        // Testdaten senden
                        byte[] testData = { 0x01, 0x02, 0x03, 0x04, 0x05 };
                        bool success = _hidManager.SendData(testData);
                        Console.WriteLine(success ? "\n✓ Testdaten gesendet" : "\n✗ Testdaten fehlgeschlagen");
                    }
                    break;
                    
                case ConsoleKey.H:
                    PrintHelp();
                    break;
                    
                case ConsoleKey.Q:
                    Console.WriteLine("\nBeende Programm...");
                    _running = false;
                    break;
                    
                case ConsoleKey.R:
                    Console.WriteLine("\nStatistik:");
                    if (_hidManager != null)
                    {
                        Console.WriteLine($"Empfangen: {_hidManager.BytesReceived} Bytes");
                        Console.WriteLine($"Gesendet: {_hidManager.BytesSent} Bytes");
                        Console.WriteLine($"Letzte Kommunikation: {_hidManager.LastCommunication:HH:mm:ss}");
                    }
                    break;
            }
        }
        
        private static void OnDataReceived(object? sender, byte[] data)
        {
            if (data.Length == 0) return;
            
            Console.ForegroundColor = ConsoleColor.Green;
            Console.Write($"\n[RX] ");
            Console.ResetColor();
            
            // Hex-Ausgabe
            Console.Write($"{BitConverter.ToString(data).Replace("-", " ")}");
            
            // ASCII-Ausgabe falls druckbare Zeichen
            if (data.Any(b => b >= 32 && b <= 126))
            {
                Console.Write("  \"");
                foreach (byte b in data)
                {
                    if (b >= 32 && b <= 126)
                        Console.Write((char)b);
                    else
                        Console.Write('.');
                }
                Console.Write("\"");
            }
            
            Console.WriteLine();
        }
        
        private static void OnConnectionChanged(object? sender, bool connected)
        {
            Console.ForegroundColor = connected ? ConsoleColor.Green : ConsoleColor.Red;
            Console.WriteLine($"\n[Status] {(connected ? "Verbunden" : "Getrennt")}");
            Console.ResetColor();
        }
        
        private static void PrintHeader()
        {
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine(@"
╔══════════════════════════════════════════════════╗
║         USB HID Communicator v1.0               ║
║         Plattformübergreifend (Win/Linux)       ║
╚══════════════════════════════════════════════════╝");
            Console.ResetColor();
            Console.WriteLine($"Laufzeit: {Environment.OSVersion}");
            Console.WriteLine($"Plattform: {Environment.OSVersion.Platform}");
            Console.WriteLine();
        }
        
        private static void PrintHelp()
        {
            Console.WriteLine("\n=== Tastenkürzel ===");
            Console.WriteLine("C  - Verbinden");
            Console.WriteLine("D  - Trennen");
            Console.WriteLine("L  - Geräte auflisten");
            Console.WriteLine("S  - String senden");
            Console.WriteLine("T  - Testdaten senden");
            Console.WriteLine("R  - Statistik anzeigen");
            Console.WriteLine("H  - Diese Hilfe");
            Console.WriteLine("Q  - Beenden");
            Console.WriteLine("====================\n");
        }
    }
}