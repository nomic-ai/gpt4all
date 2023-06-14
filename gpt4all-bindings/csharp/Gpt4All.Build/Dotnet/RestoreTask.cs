using Cake.Common.Tools.DotNet;
using Cake.Common.Tools.DotNet.Restore;
using Cake.Frosting;

namespace Gpt4All.Build.Dotnet
{
    [TaskName("Dotnet.Restore")]
    public class RestoreTask : FrostingTask<BuildContext>
    {
        public override void Run(BuildContext context)
        {
            context.DotNetRestore(context.SolutionDirectory.FullPath,
                new DotNetRestoreSettings() { Verbosity = context.DotnetVerbosity, });
        }
    }
}