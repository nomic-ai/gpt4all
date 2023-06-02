package com.hexadevlabs.gpt4all;

import java.nio.file.Path;

public class Example4 {

    public static void main(String[] args) {

        String prompt = "### Human:\nWhat is the meaning of life\n### Assistant:";

        // Optionally in case extra search path are necessary.
        //LLModel.LIBRARY_SEARCH_PATH = "C:\\Users\\felix\\gpt4all\\lib\\";

        String model = "ggml-vicuna-7b-1.1-q4_2.bin";
        try (LlamaModel mptModel = new LlamaModel(Path.of("C:\\Users\\felix\\AppData\\Local\\nomic.ai\\GPT4All\\" + model))) {

            LLModel.GenerationConfig config =
                    LLModel.config()
                            .withNPredict(4096)
                            .withRepeatLastN(64)
                            .build();

            mptModel.generate(prompt, config, true);

        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
