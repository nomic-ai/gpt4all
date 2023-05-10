function Component() {
}

var targetDirectory;
Component.prototype.beginInstallation = function() {
    targetDirectory = installer.value("TargetDir");
};

Component.prototype.createOperations = function()
{
    try {
        // call the base create operations function
        component.createOperations();
        if (systemInfo.productType === "windows") {
            try {
                var userProfile = installer.environmentVariable("USERPROFILE");
                installer.setValue("UserProfile", userProfile);
                component.addOperation("CreateShortcut",
                    targetDirectory + "/bin/chat.exe",
                    "@UserProfile@/Desktop/GPT4All.lnk",
                    "workingDirectory=" + targetDirectory + "/bin",
                    "iconPath=" + targetDirectory + "/favicon.ico",
                    "iconId=0", "description=Open GPT4All");
            } catch (e) {
                print("ERROR: creating desktop shortcut" + e);
            }
            component.addOperation("CreateShortcut",
                targetDirectory + "/bin/chat.exe",
                "@StartMenuDir@/GPT4All.lnk",
                "workingDirectory=" + targetDirectory + "/bin",
                "iconPath=" + targetDirectory + "/favicon.ico",
                "iconId=0", "description=Open GPT4All");
        } else if (systemInfo.productType === "osx") {
            var gpt4allAppPath = targetDirectory + "/bin/gpt4all.app";
            var symlinkPath = targetDirectory + "/../GPT4All.app";
            // Remove the symlink if it already exists
            component.addOperation("Execute", "rm", "-f", symlinkPath);
            // Create the symlink
            component.addOperation("Execute", "ln", "-s", gpt4allAppPath, symlinkPath);
        } else { // linux
            var homeDir = installer.environmentVariable("HOME");
            if (!installer.fileExists(homeDir + "/Desktop/GPT4All.desktop")) {
                component.addOperation("CreateDesktopEntry",
                    homeDir + "/Desktop/GPT4All.desktop",
                    "Type=Application\nTerminal=false\nExec=\"" + targetDirectory +
                    "/bin/chat\"\nName=GPT4All\nIcon=" + targetDirectory +
                    "/logo-48.png\nName[en_US]=GPT4All");
            }
        }
    } catch (e) {
        print("ERROR: running post installscript.qs" + e);
    }
}

Component.prototype.createOperationsForArchive = function(archive)
{
    component.createOperationsForArchive(archive);

    if (systemInfo.productType === "osx") {
        var uninstallTargetDirectory = installer.value("TargetDir");
        var symlinkPath = uninstallTargetDirectory + "/../GPT4All.app";

        // Remove the symlink during uninstallation
        if (installer.isUninstaller()) {
            component.addOperation("Execute", "rm", "-f", symlinkPath, "UNDOEXECUTE");
        }
    }
}
