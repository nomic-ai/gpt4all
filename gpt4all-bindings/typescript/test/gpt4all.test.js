const path = require("node:path");
const os = require("node:os");
const fsp = require("node:fs/promises");
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
    DEFAULT_MODEL_LIST_URL,
} = require("../src/config.js");
const {
    loadModel,
    createPrompt,
    createCompletion,
} = require("../src/gpt4all.js");

describe("config", () => {
    test("default paths constants are available and correct", () => {
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
});

describe("listModels", () => {
    const fakeModels = require("./models.json");
    const fakeModel = fakeModels[0];
    const mockResponse = JSON.stringify([fakeModel]);

    let mockFetch, originalFetch;

    beforeAll(() => {
        // Mock the fetch function for all tests
        mockFetch = jest.fn().mockResolvedValue({
            ok: true,
            json: () => JSON.parse(mockResponse),
        });
        originalFetch = global.fetch;
        global.fetch = mockFetch;
    });

    afterEach(() => {
        // Reset the fetch counter after each test
        mockFetch.mockClear();
    });
    afterAll(() => {
        // Restore fetch
        global.fetch = originalFetch;
    });

    it("should load the model list from remote when called without args", async () => {
        const models = await listModels();
        expect(fetch).toHaveBeenCalledTimes(1);
        expect(fetch).toHaveBeenCalledWith(DEFAULT_MODEL_LIST_URL);
        expect(models[0]).toEqual(fakeModel);
    });

    it("should load the model list from a local file, if specified", async () => {
        const models = await listModels({
            file: path.resolve(__dirname, "models.json"),
        });
        expect(fetch).toHaveBeenCalledTimes(0);
        expect(models[0]).toEqual(fakeModel);
    });
});

describe("appendBinSuffixIfMissing", () => {
    it("should make sure the suffix is there", () => {
        expect(appendBinSuffixIfMissing("filename")).toBe("filename.bin");
        expect(appendBinSuffixIfMissing("filename.bin")).toBe("filename.bin");
    });
});

describe("downloadModel", () => {
    let mockAbortController;
    beforeEach(() => {
        // Mocking the AbortController constructor
        mockAbortController = jest.fn();
        global.AbortController = mockAbortController;
    });

    afterEach(() => {
        // Clean up mocks
        mockAbortController.mockReset();
    });

    test("should successfully download a model file", async () => {
        // Mock successful fetch with fake data
        const fakeModelName = "fake-model";
        const mockData = new Uint8Array([1, 2, 3, 4]);
        const mockResponse = new ReadableStream({
            start(controller) {
                controller.enqueue(mockData);
                controller.close();
            },
        });
        const mockFetchImplementation = jest.fn(() =>
            Promise.resolve({
                ok: true,
                body: mockResponse,
            })
        );

        jest.spyOn(global, "fetch").mockImplementation(mockFetchImplementation);

        mockAbortController.mockReturnValueOnce({
            signal: "signal",
            abort: jest.fn(),
        });
        const result = downloadModel(fakeModelName, {
            allowDownload: true,
        });

        // await the download
        const modelFilePath = await result.promise;

        expect(modelFilePath).toBe(`${DEFAULT_DIRECTORY}/${fakeModelName}.bin`);
        expect(global.fetch).toHaveBeenCalledTimes(1);
        expect(global.fetch).toHaveBeenCalledWith(
            "https://gpt4all.io/models/fake-model.bin",
            {
                signal: "signal",
                headers: {
                    "Accept-Ranges": "arraybuffer",
                    "Response-Type": "arraybuffer",
                },
            }
        );

        // Restore the original fetch implementation
        global.fetch.mockRestore();
        // Remove the created file
        await fsp.unlink(modelFilePath);
    });

    // TODO
    // test("should be able to cancel and resume a download", async () => {
    // });

    // test("should error and cleanup if md5sum is not matching", async () => {
    // });
});

describe("normalizePromptContext", () => {
    it("should convert a dict with camelCased keys to snake_case", () => {
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

    it("should convert a mixed case dict to snake_case, last value taking precedence", () => {
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

    it("should not modify already snake cased dict", () => {
        const snakeCased = {
            top_k: 10,
            repeast_last_n: 10,
        };

        const result = normalizePromptContext(snakeCased);
        expect(result).toEqual(snakeCased);
    });
});
