using Xunit;

namespace Gpt4All.Tests;

public static class Platforms
{
    public const string Windows = "windows";
    public const string Linux = "linux";
    public const string MacOS = "macOS";
}

/// <summary>
/// This attribute ensures the Fact is only run on the specified platform.
/// </summary>
/// <remarks>
/// <see cref="OperatingSystem.IsOSPlatform(string)"/> for info about the platform string.
/// </remarks>
public class PlatformSpecificFactAttribute : FactAttribute
{
    public PlatformSpecificFactAttribute(string platform)
    {
        if (!OperatingSystem.IsOSPlatform(platform))
        {
            Skip = $"Test only runs on {platform}.";
        }
    }
}
