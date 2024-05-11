using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;

namespace Gpt4All
{
    public static class ModelFactory
    {
        static readonly IServiceProvider serviceProvider;

        static ModelFactory()
        {
            var services = new ServiceCollection();
            services.AddLogging(builder => builder.AddConsole());
            services.AddSingleton<INativeInteropWrapper, NativeInteropWrapper>();
            services.AddSingleton<IIOWrapper, IOWrapper>();
            serviceProvider = services.BuildServiceProvider();
        }

        public static LLModel CreateLLModel(string modelPath, ILogger? logger = null)
        {
            logger ??= serviceProvider.GetRequiredService<ILogger<LLModel>>();
            var nativeWrapper = serviceProvider.GetRequiredService<INativeInteropWrapper>();
            var ioWrapper = serviceProvider.GetRequiredService<IIOWrapper>();
            return new LLModel(modelPath, nativeWrapper, logger, ioWrapper);
        }
    }
}
