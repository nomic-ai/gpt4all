package com.hexadevlabs.gpt4all;

import java.nio.file.Path;

public class Example4 {

    public static void main(String[] args) {

        String prompt = "### Human:\nWhat is the meaning of life\n### Assistant:";
        // The emoji is poop emoji. The Unicode character is encoded as surrogate pair for Java string.
        // LLM should correctly identify it as poop emoji in the description
        //String prompt = "### Human:\nDescribe the meaning of this emoji \uD83D\uDCA9\n### Assistant:";
        //String prompt = "### Human:\nOutput the unicode character of smiley face emoji\n### Assistant:";

        // Optionally in case override to location of shared libraries is necessary
        //LLModel.LIBRARY_SEARCH_PATH = "C:\\Users\\felix\\gpt4all\\lib\\";

        String model = "ggml-vicuna-7b-1.1-q4_2.bin";
        //String model = "ggml-gpt4all-j-v1.3-groovy.bin";
        //String model = "ggml-mpt-7b-instruct.bin";
        String basePath = "C:\\Users\\felix\\AppData\\Local\\nomic.ai\\GPT4All\\";
        //String basePath = "/Users/fzaslavs/Library/Application Support/nomic.ai/GPT4All/";

        try (LLModel mptModel = new LLModel(Path.of(basePath + model))) {

            LLModel.GenerationConfig config =
                    LLModel.config()
                            .withNPredict(4096)
                            .withRepeatLastN(64)
                            .build();


            String result = mptModel.generate(prompt, config, true);

            System.out.println("Code points:");
            result.codePoints().forEach(System.out::println);


        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
