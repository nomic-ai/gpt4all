function Component()
{
    component.ifwVersion = installer.value("FrameworkVersion");
    installer.installationStarted.connect(this, Component.prototype.onInstallationStarted);
}

Component.prototype.onInstallationStarted = function()
{
    if (component.updateRequested() || component.installationRequested()) {
        if (installer.value("os") == "win") {
            component.installerbaseBinaryPath = "@TargetDir@/installerbase.exe";
        } else if (installer.value("os") == "x11") {
            component.installerbaseBinaryPath = "@TargetDir@/installerbase";
        } else if (installer.value("os") == "mac") {
            component.installerbaseBinaryPath = "@TargetDir@/MaintenanceTool.app";
        }
        installer.setInstallerBaseBinary(component.installerbaseBinaryPath);
    }
}
