using Cake.Frosting;

namespace Gpt4All.Build
{
    [TaskName("Default")]
    [IsDependentOn(typeof(Gpt4All.Build.Cmake.Mingw.BuildTask))]
    public class DefaultTask : FrostingTask<BuildContext>
    {
        public override void Run(BuildContext context)
        {
        }
    }
}
