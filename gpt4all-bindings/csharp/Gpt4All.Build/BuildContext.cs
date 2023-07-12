using System.Linq;
using Cake.Common;
using Cake.Common.IO;
using Cake.Common.Tools.DotNet;
using Cake.Core;
using Cake.Core.IO;
using Cake.Frosting;

namespace Gpt4All.Build
{
    public class BuildContext : FrostingContext
    {
        public BuildContext(ICakeContext context)
            : base(context)
        {
            var currentDirectory = context.Environment.WorkingDirectory;

            SolutionDirectory = GetSolutionRoot(context);
            RepositoryDirectory = GetRepositoryRoot(context);
            LibDirectory = SolutionDirectory.Combine("lib");
            ArtifactsDirectory = SolutionDirectory.Combine("artifacts");
            ProjectPath = SolutionDirectory.Combine("Gpt4All");
            BackendSourceDirectory = RepositoryDirectory.Combine("gpt4all-backend");
            BuildConfiguration = context.Argument("configuration", "Release");

            CmakeToolPath = context.Tools.Resolve("cmake") ??
                            context.Tools.Resolve("cmake.exe") ??
                            throw new Exception("cmake not found");

            MsvcGenerator = context.Argument("msvc-generator", "Visual Studio 17 2022");
            MingwGenerator = context.Argument("mingw-generator", "MinGW Makefiles");
            DotnetVerbosity = context.Argument<DotNetVerbosity?>("dotnet-verbosity", DotNetVerbosity.Detailed);
        }

        public string MingwGenerator { get; set; }

        public DirectoryPath LibDirectory { get; set; }

        public string MsvcGenerator { get; set; }

        public FilePath CmakeToolPath { get; set; }

        public DirectoryPath BackendSourceDirectory { get; set; }

        public string BuildConfiguration { get; set; }

        public DirectoryPath RepositoryDirectory { get; set; }

        public DirectoryPath SolutionDirectory { get; set; }
        public DotNetVerbosity? DotnetVerbosity { get; set; }
        public DirectoryPath ArtifactsDirectory { get; set; }
        public DirectoryPath ProjectPath { get; set; }

        private static DirectoryPath GetSolutionRoot(ICakeContext context)
        {
            var directoryPath = context.Environment.WorkingDirectory;
            while (true)
            {
                var directory = context.FileSystem.GetDirectory(directoryPath);
                if (directory.GetFiles("*.sln", SearchScope.Current).Any())
                {
                    return directoryPath;
                }

                directoryPath = directoryPath.GetParent();
            }
        }

        private static DirectoryPath GetRepositoryRoot(ICakeContext context)
        {
            var directoryPath = context.Environment.WorkingDirectory;

            while (!context.DirectoryExists(directoryPath.Combine(".git")))
            {
                directoryPath = directoryPath.GetParent();
            }

            return directoryPath;
        }

        public DirectoryPath ResolveRuntimePath()
        {
            var runtimePath = SolutionDirectory.Combine("runtime");

            return runtimePath;
        }
    }
}
