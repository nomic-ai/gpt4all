var finishedText = null;

function cancelInstaller(message) {
    installer.setDefaultPageVisible(QInstaller.Introduction,         false);
    installer.setDefaultPageVisible(QInstaller.TargetDirectory,      false);
    installer.setDefaultPageVisible(QInstaller.ComponentSelection,   false);
    installer.setDefaultPageVisible(QInstaller.ReadyForInstallation, false);
    installer.setDefaultPageVisible(QInstaller.StartMenuSelection,   false);
    installer.setDefaultPageVisible(QInstaller.PerformInstallation,  false);
    installer.setDefaultPageVisible(QInstaller.LicenseCheck,         false);
    finishedText = message;
    installer.setCanceled();
}

function vercmp(a, b) {
    return a.localeCompare(b, undefined, { numeric: true, sensitivity: "base" });
}

function Controller() {
}

Controller.prototype.TargetDirectoryPageCallback = function() {
    var failedReq = null;
    if (systemInfo.productType === "ubuntu" && vercmp(systemInfo.productVersion, "22.04") < 0) {
        failedReq = "Ubuntu 22.04 LTS";
    } else if (systemInfo.productType === "macos" && vercmp(systemInfo.productVersion, "12.6") < 0) {
        failedReq = "macOS Monterey 12.6";
    }

    if (failedReq !== null) {
        cancelInstaller(
            "Installation cannot continue because GPT4All does not support your operating system: " +
            `${systemInfo.prettyProductName}<br/><br/>` +
            `GPT4All requires ${failedReq} or newer.`
        );
    }
}

Controller.prototype.FinishedPageCallback = function() {
    const widget = gui.currentPageWidget();
    if (widget != null && finishedText != null) {
        widget.MessageLabel.setText(finishedText);
    }
}
