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
            serviceProvider = services.BuildServiceProvider();
        }

        public static LLModel CreateLLModel(string modelPath, ILogger? logger = null)
        {
            logger ??= serviceProvider.GetRequiredService<ILogger<LLModel>>();
            var wrapper = serviceProvider.GetRequiredService<INativeInteropWrapper>();
            return new LLModel(modelPath, wrapper, logger);
        }
    }
}
