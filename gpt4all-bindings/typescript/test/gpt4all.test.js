const path = require("node:path");
const os = require("node:os");
const fsp = require("node:fs/promises");
const { existsSync } = require('node:fs');
const { LLModel } = require("node-gyp-build")(path.resolve(__dirname, ".."));
const {
    listModels,
    downloadModel,
    appendBinSuffixIfMissing,
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
            path.resolve(
                __dirname,
                "..",
                `runtimes/${process.platform}/native`,
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

    it("should throw an error if neither url nor file is specified", async () => {
        await expect(listModels(null)).rejects.toThrow(
            "No model list source specified. Please specify either a url or a file."
        );
    });
});

describe("appendBinSuffixIfMissing", () => {
    it("should make sure the suffix is there", () => {
        expect(appendBinSuffixIfMissing("filename")).toBe("filename.gguf");
        expect(appendBinSuffixIfMissing("filename.bin")).toBe("filename.bin");
    });
});

describe("downloadModel", () => {
    let mockAbortController, mockFetch;
    const fakeModelName = "fake-model";

    const createMockFetch = () => {
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
        return mockFetchImplementation;
    };

    beforeEach(async () => {
        // Mocking the AbortController constructor
        mockAbortController = jest.fn();
        global.AbortController = mockAbortController;
        mockAbortController.mockReturnValue({
            signal: "signal",
            abort: jest.fn(),
        });
        mockFetch = createMockFetch();
        jest.spyOn(global, "fetch").mockImplementation(mockFetch);

    });

    afterEach(async () => {
        // Clean up mocks
        mockAbortController.mockReset();
        mockFetch.mockClear();
        global.fetch.mockRestore();

        const rootDefaultPath = path.resolve(DEFAULT_DIRECTORY),
              partialPath = path.resolve(rootDefaultPath, fakeModelName+'.part'),
              fullPath = path.resolve(rootDefaultPath, fakeModelName+'.bin')

        //if tests fail, remove the created files
        // acts as cleanup if tests fail
        //
        if(existsSync(fullPath)) {
            await fsp.rm(fullPath)
        }
        if(existsSync(partialPath)) {
            await fsp.rm(partialPath)
        }

    });

    test("should successfully download a model file", async () => {
        const downloadController = downloadModel(fakeModelName);
        const modelFilePath = await downloadController.promise;
        expect(modelFilePath).toBe(path.resolve(DEFAULT_DIRECTORY, `${fakeModelName}.gguf`));

        expect(global.fetch).toHaveBeenCalledTimes(1);
        expect(global.fetch).toHaveBeenCalledWith(
            "https://gpt4all.io/models/gguf/fake-model.gguf",
            {
                signal: "signal",
                headers: {
                    "Accept-Ranges": "arraybuffer",
                    "Response-Type": "arraybuffer",
                },
            }
        );

        // final model file should be present
        await expect(fsp.access(modelFilePath)).resolves.not.toThrow();

        // remove the testing model file
        await fsp.unlink(modelFilePath);
    });

    test("should error and cleanup if md5sum is not matching", async () => {
        const downloadController = downloadModel(fakeModelName, {
            md5sum: "wrong-md5sum",
        });
        // the promise should reject with a mismatch
        await expect(downloadController.promise).rejects.toThrow(
            `Model "fake-model" failed verification: Hashes mismatch. Expected wrong-md5sum, got 08d6c05a21512a79a1dfeb9d2a8f262f`
        );
        // fetch should have been called
        expect(global.fetch).toHaveBeenCalledTimes(1);
        // the file should be missing
        await expect(
            fsp.access(path.resolve(DEFAULT_DIRECTORY, `${fakeModelName}.gguf`))
        ).rejects.toThrow();
        // partial file should also be missing
        await expect(
            fsp.access(path.resolve(DEFAULT_DIRECTORY, `${fakeModelName}.part`))
        ).rejects.toThrow();
    });

    // TODO
    // test("should be able to cancel and resume a download", async () => {
    // });
});
