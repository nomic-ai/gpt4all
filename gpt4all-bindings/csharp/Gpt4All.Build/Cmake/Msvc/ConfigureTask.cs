using Cake.Common;
using Cake.Common.IO;
using Cake.Core;
using Cake.Core.IO;
using Cake.Frosting;

namespace Gpt4All.Build.Cmake.Msvc
{
    [TaskName("Cmake.Msvc.Configure")]
    public sealed class ConfigureTask : FrostingTask<BuildContext>
    {
        public override void Run(BuildContext context)
        {
            var runtimePath = context.ResolveRuntimePath();

            context.EnsureDirectoryDoesNotExist(runtimePath);
            context.EnsureDirectoryExists(runtimePath);
            var buildPath = runtimePath.Combine("build").Combine(context.MsvcGenerator);
            context.EnsureDirectoryExists(buildPath);

            var processParameterBuilder = new ProcessArgumentBuilder();

            processParameterBuilder.Append("-G ");
            processParameterBuilder.AppendQuoted(context.MsvcGenerator);
            processParameterBuilder.Append("-A ");
            processParameterBuilder.Append("X64");
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
