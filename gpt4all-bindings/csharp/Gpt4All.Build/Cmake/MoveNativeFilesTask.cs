using Cake.Common.IO;
using Cake.Core.IO;
using Cake.Frosting;

namespace Gpt4All.Build.Cmake
{
    [TaskName("Cmake.MoveNativeTasks")]
    [IsDependentOn(typeof(BuildTask))]
    public class MoveNativeFilesTask : FrostingTask<BuildContext>
    {
        public override void Run(BuildContext context)
        {
            var msvcOutputPath = context.ResolveRuntimePath().Combine("build").Combine("Visual Studio 17 2022")
                .Combine("bin").Combine(context.BuildConfiguration);
            var files = context.GetFiles(GlobPattern.FromString(msvcOutputPath.CombineWithFilePath("*.dll").FullPath));

            context.EnsureDirectoryDoesNotExist(context.LibDirectory);
            context.EnsureDirectoryExists(context.LibDirectory);

            foreach (var file in files)
            {
                context.CopyFileToDirectory(file, context.LibDirectory);
            }

            foreach (var filePath in context.GetFiles(
                         GlobPattern.FromString(context.LibDirectory.CombineWithFilePath("llmodel.*").FullPath)))
            {
                context.MoveFile(filePath, filePath.FullPath.Replace("llmodel", "libllmodel"));
            }
        }
    }
}
