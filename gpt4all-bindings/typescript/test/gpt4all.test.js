const path = require("node:path");
const os = require("node:os");
const { LLModel } = require("node-gyp-build")(path.resolve(__dirname, ".."));
const {
    listModels,
    downloadModel,
    appendBinSuffixIfMissing,
    normalizePromptContext,
} = require("../src/util.js");
const {
    DEFAULT_DIRECTORY,
    DEFAULT_LIBRARIES_DIRECTORY,
} = require("../src/config.js");
const {
    loadModel,
    createPrompt,
    createCompletion,
} = require("../src/gpt4all.js");

global.fetch = jest.fn(() =>
    Promise.resolve({
        json: () => Promise.resolve([{}, {}, {}]),
    })
);

jest.mock("../src/util.js", () => {
    const actualModule = jest.requireActual("../src/util.js");
    return {
        ...actualModule,
        downloadModel: jest.fn(() => ({
            cancel: jest.fn(),
            promise: jest.fn(),
        })),
    };
});

beforeEach(() => {
    downloadModel.mockClear();
});

afterEach(() => {
    fetch.mockClear();
    jest.clearAllMocks();
});

describe("utils", () => {
    test("appendBinSuffixIfMissing", () => {
        expect(appendBinSuffixIfMissing("filename")).toBe("filename.bin");
        expect(appendBinSuffixIfMissing("filename.bin")).toBe("filename.bin");
    });
    test("default paths", () => {
        expect(DEFAULT_DIRECTORY).toBe(
            path.resolve(os.homedir(), ".cache/gpt4all")
        );
        const paths = [
            path.join(DEFAULT_DIRECTORY, "libraries"),
            path.resolve("./libraries"),
            path.resolve(
                __dirname,
                "..",
                `runtimes/${process.platform}-${process.arch}/native`
            ),
            process.cwd(),
        ];
        expect(typeof DEFAULT_LIBRARIES_DIRECTORY).toBe("string");
        expect(DEFAULT_LIBRARIES_DIRECTORY).toBe(paths.join(";"));
    });

    test("listModels", async () => {
        try {
            await listModels();
        } catch (e) {}

        expect(fetch).toHaveBeenCalledTimes(1);
        expect(fetch).toHaveBeenCalledWith(
            "https://gpt4all.io/models/models.json"
        );
    });

    describe("normalizePromptContext", () => {
        test("should convert a dict with camelCased keys to snake_case", () => {
            const camelCased = {
                topK: 20,
                repeatLastN: 10,
            };

            const expectedSnakeCased = {
                top_k: 20,
                repeat_last_n: 10,
            };

            const result = normalizePromptContext(camelCased);
            expect(result).toEqual(expectedSnakeCased);
        });

        test("should convert a mixed case dict to snake_case, last value taking precedence", () => {
            const mixedCased = {
                topK: 20,
                top_k: 10,
                repeatLastN: 10,
            };

            const expectedSnakeCased = {
                top_k: 10,
                repeat_last_n: 10,
            };

            const result = normalizePromptContext(mixedCased);
            expect(result).toEqual(expectedSnakeCased);
        });

        test("should not modify already snake cased dict", () => {
            const snakeCased = {
                top_k: 10,
                repeast_last_n: 10,
            };

            const result = normalizePromptContext(snakeCased);
            expect(result).toEqual(snakeCased);
        });
    });
});
