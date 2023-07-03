package com.hexadevlabs.gpt4all;

import java.io.OutputStream;
import java.io.PrintStream;
import java.nio.file.Path;

public class Example5 {

    public static void main(String[] args) {

        String prompt = "### Human:\nWhat is the meaning of life\n### Assistant:";
        String model = "ggml-gpt4all-j-v1.3-groovy.bin";
        String basePath = "C:\\Users\\felix\\AppData\\Local\\nomic.ai\\GPT4All\\";

        try (LLModel mptModel = new LLModel(Path.of(basePath + model))) {

            LLModel.GenerationConfig config =
                    LLModel.config()
                            .withNPredict(4096)
                            .withRepeatLastN(64)
                            .build();


            try (var out = new OutputStream() {
                @Override
                public void write(int b) {
                    System.out.print((char) b);
                }
            }) {
                mptModel.generate(prompt, config, new PrintStream(out));
            }


        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
