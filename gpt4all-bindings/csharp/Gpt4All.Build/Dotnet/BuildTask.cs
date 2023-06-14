using Cake.Common.Tools.DotNet;
using Cake.Common.Tools.DotNet.Build;
using Cake.Common.Tools.DotNet.Restore;
using Cake.Frosting;
using Gpt4All.Build.Cmake;

namespace Gpt4All.Build.Dotnet
{
    [TaskName("Dotnet.Build")]
    [IsDependentOn(typeof(MoveNativeFilesTask))]
    public class BuildTask : FrostingTask<BuildContext>
    {
        public override void Run(BuildContext context)
        {
            context.DotNetRestore(context.SolutionDirectory.FullPath,
                new DotNetRestoreSettings() { Verbosity = context.DotnetVerbosity, });

            context.DotNetBuild(context.SolutionDirectory.FullPath,
                new DotNetBuildSettings()
                {
                    Configuration = context.BuildConfiguration,
                    NoRestore = true,
                    NoIncremental = true
                });
        }
    }
}
