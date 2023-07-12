using Cake.Frosting;

namespace Gpt4All.Build.Cmake
{
    [TaskName("Cmake.Build")]
    [IsDependentOn(typeof(Msvc.BuildTask))]
    public class BuildTask : FrostingTask<BuildContext>
    {
    }
}
