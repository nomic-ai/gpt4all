using Cake.Common.Tools.DotNet;
using Cake.Common.Tools.DotNet.Test;
using Cake.Frosting;

namespace Gpt4All.Build.Dotnet
{
    [TaskName("Dotnet.Test")]
    [IsDependentOn(typeof(BuildTask))]
    public class TestTask : FrostingTask<BuildContext>
    {
        public override void Run(BuildContext context)
        {
            context.DotNetTest(context.SolutionDirectory.FullPath,
                new DotNetTestSettings() { Configuration = context.BuildConfiguration, });
        }
    }
}