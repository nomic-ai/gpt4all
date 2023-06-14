using Cake.Common;
using Cake.Core;
using Cake.Core.IO;
using Cake.Frosting;

namespace Gpt4All.Build.Cmake.Msvc
{
    [TaskName("Cmake.Msvc.Build")]
    [IsDependentOn(typeof(ConfigureTask))]
    public sealed class BuildTask : FrostingTask<BuildContext>
    {
        public override void Run(BuildContext context)
        {
            var runtimePath = context.ResolveRuntimePath();

            var buildPath = runtimePath.Combine("build").Combine(context.MsvcGenerator);

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
}
