function Component() {
}

var targetDirectory;
Component.prototype.beginInstallation = function() {
    targetDirectory = installer.value("TargetDir");
};

Component.prototype.createOperations = function() {
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
                    "iconPath=" + targetDirectory + "/gpt4all.ico",
                    "iconId=0", "description=Open GPT4All");
            } catch (e) {
                print("ERROR: creating desktop shortcut" + e);
            }
            component.addOperation("CreateShortcut",
                targetDirectory + "/bin/chat.exe",
                "@StartMenuDir@/GPT4All.lnk",
                "workingDirectory=" + targetDirectory + "/bin",
                "iconPath=" + targetDirectory + "/gpt4all.ico",
                "iconId=0", "description=Open GPT4All");
        } else if (systemInfo.productType === "macos") {
            var gpt4allAppPath = targetDirectory + "/bin/gpt4all.app";
            var symlinkPath = targetDirectory + "/../GPT4All.app";
            // Remove the symlink if it already exists
            component.addOperation("Execute", "rm", "-f", symlinkPath);
            // Create the symlink
            component.addOperation("Execute", "ln", "-s", gpt4allAppPath, symlinkPath);
        } else { // linux
            var homeDir = installer.environmentVariable("HOME");
            var desktopEntryContent = "Type=Application\n" +
                "Terminal=false\n" +
                "Exec=\"" + targetDirectory + "/bin/chat\"\n" +
                "Name=GPT4All\n" +
                "Comment=Run open-source large language models locally\n" +
                "GenericName=ChatBot\n" +
                "Keywords=gpt;gpt4all;llm;ai;chatbot\n" +
                "Icon=" + targetDirectory + "/gpt4all-48.png\n" +
                "Categories=Office;Chat;\n" +
                "Name[en_US]=GPT4All\n" +
                "StartupNotify=true";

            // Desktop shortcut
            if (!installer.fileExists(homeDir + "/Desktop/GPT4All.desktop")) {
                component.addOperation("CreateDesktopEntry",
                    homeDir + "/Desktop/GPT4All.desktop",
                    desktopEntryContent);
            }

            // XDG application menu entry (~/.local/share/applications/)
            // so GPT4All appears in the system start menu / application launcher
            var appsDir = homeDir + "/.local/share/applications";
            if (!installer.fileExists(appsDir + "/GPT4All.desktop")) {
                component.addOperation("CreateDesktopEntry",
                    appsDir + "/GPT4All.desktop",
                    desktopEntryContent);
            }
        }
    } catch (e) {
        print("ERROR: running post installscript.qs" + e);
    }
}

Component.prototype.createOperationsForArchive = function(archive)
{
    component.createOperationsForArchive(archive);

    if (systemInfo.productType === "macos") {
        var uninstallTargetDirectory = installer.value("TargetDir");
        var symlinkPath = uninstallTargetDirectory + "/../GPT4All.app";

        // Remove the symlink during uninstallation
        if (installer.isUninstaller()) {
            component.addOperation("Execute", "rm", "-f", symlinkPath, "UNDOEXECUTE");
        }
    }
}
