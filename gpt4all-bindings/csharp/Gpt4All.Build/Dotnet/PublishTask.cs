using Cake.Common.IO;
using Cake.Common.Tools.DotNet;
using Cake.Common.Tools.DotNet.Pack;
using Cake.Frosting;

namespace Gpt4All.Build.Dotnet
{
    [TaskName("Dotnet.Publish")]
    [IsDependentOn(typeof(BuildTask))]
    public class PublishTask : FrostingTask<BuildContext>
    {
        public override void Run(BuildContext context)
        {
            context.EnsureDirectoryExists(context.ArtifactsDirectory);
            context.DotNetPack(context.ProjectPath.FullPath,
                new DotNetPackSettings()
                {
                    Configuration = context.BuildConfiguration,
                    OutputDirectory = context.ArtifactsDirectory.FullPath,
                    NoBuild = true,
                    NoRestore = true,
                    Verbosity = context.DotnetVerbosity,
                });
        }
    }
}