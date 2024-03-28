const os = require("node:os");
const path = require("node:path");

const DEFAULT_DIRECTORY = path.resolve(os.homedir(), ".cache/gpt4all");

const librarySearchPaths = [
    path.join(DEFAULT_DIRECTORY, "libraries"),
    path.resolve("./libraries"),
    path.resolve(
        __dirname,
        "..",
        `runtimes/${process.platform}-${process.arch}/native`,
    ),
    //for darwin. This is hardcoded for now but it should work
    path.resolve(
        __dirname,
        "..",
        `runtimes/${process.platform}/native`,
    ),
    process.cwd(),
];

const DEFAULT_LIBRARIES_DIRECTORY = librarySearchPaths.join(";");

const DEFAULT_MODEL_CONFIG = {
    systemPrompt: "",
    promptTemplate: "### Human:\n%1\n\n### Assistant:\n",
}

const DEFAULT_MODEL_LIST_URL = "https://gpt4all.io/models/models3.json";

const DEFAULT_PROMPT_CONTEXT = {
    temp: 0.1,
    topK: 40,
    topP: 0.9,
    minP: 0.0,
    repeatPenalty: 1.18,
    repeatLastN: 10,
    nBatch: 100,
}

module.exports = {
    DEFAULT_DIRECTORY,
    DEFAULT_LIBRARIES_DIRECTORY,
    DEFAULT_MODEL_CONFIG,
    DEFAULT_MODEL_LIST_URL,
    DEFAULT_PROMPT_CONTEXT,
};
