const { loadModel } = require("../src/gpt4all.js");

// these tests require an internet connection / a real model
const testModel = "Phi-3-mini-4k-instruct.Q4_0.gguf";

describe("llmodel", () => {
    let model;

    test("load on cpu", async () => {
        model = await loadModel(testModel, {
            device: "cpu",
        });
    });

    test("getter working", async () => {
        const stateSize = model.llm.getStateSize();
        expect(stateSize).toBeGreaterThan(0);
        const name = model.llm.getName();
        expect(name).toBe(testModel);
        const type = model.llm.getType();
        expect(type).toBeUndefined();
        const devices = model.llm.getGpuDevices();
        expect(Array.isArray(devices)).toBe(true);
        const requiredMem = model.llm.getRequiredMemory();
        expect(typeof requiredMem).toBe('number');
        const threadCount = model.llm.getThreadCount();
        expect(threadCount).toBe(4);
    });

    test("setting thread count", () => {
        model.llm.setThreadCount(5);
        expect(model.llm.getThreadCount()).toBe(5);
    });

    test("cpu inference", async () => {
        const res = await model.llm.infer("what is the capital of france?", {
            temp: 0,
            promptTemplate: model.config.promptTemplate,
            nPredict: 10,
            onResponseToken: () => {
                return true;
            },
        });
        expect(res.text).toMatch(/paris/i);
    }, 10000);

    test("dispose and load model on gpu", async () => {
        model.dispose();
        model = await loadModel(testModel, {
            device: "gpu",
        });
    });

    test("gpu inference", async () => {
        const res = await model.llm.infer("what is the capital of france?", {
            temp: 0,
            promptTemplate: model.config.promptTemplate,
            nPredict: 10,
            onResponseToken: () => {
                return true;
            },
        });
        expect(res.text).toMatch(/paris/i);
    }, 10000);

    afterAll(() => {
        model.dispose();
    });
});
