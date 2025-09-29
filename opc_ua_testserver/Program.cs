using Opc.Ua;
using Opc.Ua.Configuration;
using Opc.Ua.Server;
using System;

namespace BatchPlant
{
    public static class ServerProgram
    {
        public static void Main(string[] args)
        {
            ApplicationInstance application = new ApplicationInstance
            {
                ApplicationName = "BatchPlant OPC UA Server",
                ApplicationType = ApplicationType.Server,
                ConfigSectionName = "BatchPlantServer"
            };
            application.LoadApplicationConfiguration("./BatchPlantServer.Config.xml", false).Wait();
            application.CheckApplicationInstanceCertificate(false, 0).Wait();
            application.Start(new BatchPlantServer()).Wait();

            Console.WriteLine("The BatchPlant OPC UA Server is now running.");
            Console.WriteLine("Server URLs: {0}", string.Join(", ", application.Server.GetEndpoints().Select(e => e.EndpointUrl)) );

            Console.WriteLine("BatchPlant OPC UA Server is running. Press Ctrl-C to exit...");
            System.Threading.AutoResetEvent quitEvent = new(false);
            Console.CancelKeyPress += (sender, eArgs) =>
            {
                quitEvent.Set();
                eArgs.Cancel = true;
            };
            quitEvent.WaitOne();
            application.Stop();
            Console.WriteLine("BatchPlant OPC UA Server has stopped.");

        }
    }


    class BatchPlantServer : StandardServer
    {
        protected override MasterNodeManager CreateMasterNodeManager(IServerInternal server, ApplicationConfiguration configuration)
        {
            Console.WriteLine("Creating the Node Managers.");
            IList<INodeManager> nodeManagers = new List<INodeManager>
            {
                new BatchPlantNodeManager(server, configuration)
            };
            return new MasterNodeManager(server, configuration, null, nodeManagers.ToArray());
        }

        protected override ServerProperties LoadServerProperties()
        {
            return new ServerProperties
            {
                ManufacturerName = "Klaus Liebler",
                ProductName = "BatchPlant OPC UA Server",
                ProductUri = "http://opcfoundation.org/BatchPlant/Server",
                SoftwareVersion = Utils.GetAssemblySoftwareVersion(),
                BuildNumber = Utils.GetAssemblyBuildNumber(),
                BuildDate = Utils.GetAssemblyTimestamp()
            };
        }
    }
}
