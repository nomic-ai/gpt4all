const os = require("node:os");
const path = require("node:path");

const DEFAULT_DIRECTORY = path.resolve(os.homedir(), ".cache/gpt4all");

const librarySearchPaths = [
    path.join(DEFAULT_DIRECTORY, "libraries"),
    path.resolve("./libraries"),
    path.resolve(
        __dirname,
        "..",
        `runtimes/${process.platform}-${process.arch}/native`
    ),
    process.cwd(),
];

const DEFAULT_LIBRARIES_DIRECTORY = librarySearchPaths.join(";");

module.exports = {
    DEFAULT_DIRECTORY,
    DEFAULT_LIBRARIES_DIRECTORY,
};
