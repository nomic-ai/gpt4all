using Cake.Common;
using Cake.Common.IO;
using Cake.Core;
using Cake.Core.IO;
using Cake.Frosting;

namespace Gpt4All.Build.Cmake.Mingw
{
    [TaskName("Cmake.Mingw.Build")]
    [IsDependentOn(typeof(ConfigureTask))]
    public sealed class BuildTask : FrostingTask<BuildContext>
    {
        public override void Run(BuildContext context)
        {
            var runtimePath = context.ResolveRuntimePath();

            var buildPath = runtimePath.Combine("build").Combine(context.MingwGenerator);

            var processParameterBuilder = new ProcessArgumentBuilder();

            processParameterBuilder.Append("--build");
            processParameterBuilder.AppendQuoted(buildPath.FullPath);
            processParameterBuilder.Append("--parallel");
            processParameterBuilder.Append("--config ");
            processParameterBuilder.AppendQuoted(context.BuildConfiguration);

            var process = context.StartProcess(context.CmakeToolPath,
                new ProcessSettings { WorkingDirectory = runtimePath, Arguments = processParameterBuilder.Render() });
            if (process != 0)
            {
                throw new CakeException("Cmake build failed");
            }
        }
    }
    [TaskName("Cmake.Mingw.Configure")]
    public sealed class ConfigureTask : FrostingTask<BuildContext>
    {
        public override void Run(BuildContext context)
        {
            var runtimePath = context.ResolveRuntimePath();

            context.EnsureDirectoryDoesNotExist(runtimePath);
            context.EnsureDirectoryExists(runtimePath);
            var buildPath = runtimePath.Combine("build").Combine(context.MingwGenerator);
            context.EnsureDirectoryExists(buildPath);

            var processParameterBuilder = new ProcessArgumentBuilder();

            processParameterBuilder.Append("-G ");
            processParameterBuilder.AppendQuoted(context.MingwGenerator);
            processParameterBuilder.Append("-S ");
            processParameterBuilder.AppendQuoted(context.BackendSourceDirectory.FullPath);
            processParameterBuilder.Append("-B ");
            processParameterBuilder.AppendQuoted(buildPath.FullPath);

            var process = context.StartProcess(context.CmakeToolPath,
                new ProcessSettings { WorkingDirectory = runtimePath, Arguments = processParameterBuilder.Render() });

            if (process != 0)
            {
                throw new CakeException("Cmake configure failed");
            }
        }
    }
}
