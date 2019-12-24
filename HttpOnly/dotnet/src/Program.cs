using System;
using System.Diagnostics;
using System.Net.Http;
using System.Threading.Tasks;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Configuration;
using System.IO;

namespace iotCentral.Http
{
    class Program
    {
        static async Task Main(string[] args)
        {
            try {
                var builder = new ConfigurationBuilder()
                    .SetBasePath(Directory.GetCurrentDirectory())
                    .AddJsonFile("appsettings.json", optional: true, reloadOnChange: true);

                var serviceProvider = new ServiceCollection()
                                    .AddHttpClient()
                                    .AddTransient<IRequestHelper>((serv)=>new RequestHelper(serv.GetService<IHttpClientFactory>()))
                                    .AddSingleton<IConfiguration>(builder.Build())
                                    .AddSingleton<IoTCentralDemo>()
                                    .BuildServiceProvider();
                                    
                
                IoTCentralDemo demo = serviceProvider.GetService<IoTCentralDemo>();
                await demo.Run();
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
                Debugger.Break();
                
            }
        }
    }
}
