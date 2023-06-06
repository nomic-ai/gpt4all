package com.hexadevlabs.gpt4all;

import java.nio.file.Path;
import java.util.List;
import java.util.Map;

/**
 * GPTJ chat completion, multiple messages
 */
public class Example1 {
    public static void main(String[] args) {

        // Optionally in case override to location of shared libraries is necessary
        //LLModel.LIBRARY_SEARCH_PATH = "C:\\Users\\felix\\gpt4all\\lib\\";

        try ( LLModel gptjModel = new LLModel(Path.of("C:\\Users\\felix\\AppData\\Local\\nomic.ai\\GPT4All\\ggml-gpt4all-j-v1.3-groovy.bin")) ){

            LLModel.GenerationConfig config = LLModel.config()
                    .withNPredict(4096).build();

            gptjModel.chatCompletion(
                    List.of(Map.of("role", "user", "content", "Add 2+2"),
                            Map.of("role", "assistant", "content", "4"),
                            Map.of("role", "user", "content", "Multiply 4 * 5")), config, true, true);

        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}