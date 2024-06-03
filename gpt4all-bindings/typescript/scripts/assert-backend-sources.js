const fs = require("fs");
const path = require("path");

// Copies the shared llmodel sources from gpt4all-backend into the backend folder.
// These are dependencies of the bindings and will be required in case node-gyp-build
// cannot find a prebuild. This script is used in the package install hook and will
// be executed BOTH when `yarn install` is run in the root folder AND when the package
// is installed as a dependency in another project.

const backendDeps = [
    "llmodel.h",
    "llmodel.cpp",
    "llmodel_c.cpp",
    "llmodel_c.h",
    "sysinfo.h",
    "dlhandle.h",
    "dlhandle.cpp",
];

const sourcePath = path.resolve(__dirname, "../../../gpt4all-backend");
const destPath = path.resolve(__dirname, "../backend");

// Silently ignore if the backend sources are not available.
// When the package is installed as a dependency, gpt4all-backend will not be present.
if (fs.existsSync(sourcePath)) {
    if (!fs.existsSync(destPath)) {
        fs.mkdirSync(destPath);
    }
    for (const file of backendDeps) {
        const sourceFile = path.join(sourcePath, file);
        const destFile = path.join(destPath, file);
        if (fs.existsSync(sourceFile)) {
            console.info(`Copying ${sourceFile} to ${destFile}`);
            fs.copyFileSync(sourceFile, destFile); // overwrite
        } else {
            throw new Error(`File ${sourceFile} does not exist`);
        }
    }
}

// assert that the backend sources are present
for (const file of backendDeps) {
    const destFile = path.join(destPath, file);
    if (!fs.existsSync(destFile)) {
        throw new Error(`File ${destFile} does not exist`);
    }
}
