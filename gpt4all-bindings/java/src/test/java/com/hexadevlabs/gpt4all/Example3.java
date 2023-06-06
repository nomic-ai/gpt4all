package com.hexadevlabs.gpt4all;

import jnr.ffi.LibraryLoader;

import java.nio.file.Path;
import java.util.List;
import java.util.Map;

/**
 * GPTJ chat completion with system message
 */
public class Example3 {
    public static void main(String[] args) {

        // Optionally in case override to location of shared libraries is necessary
        //LLModel.LIBRARY_SEARCH_PATH = "C:\\Users\\felix\\gpt4all\\lib\\";

        try ( LLModel gptjModel = new LLModel(Path.of("C:\\Users\\felix\\AppData\\Local\\nomic.ai\\GPT4All\\ggml-gpt4all-j-v1.3-groovy.bin")) ){

            LLModel.GenerationConfig config = LLModel.config()
                    .withNPredict(4096).build();

            // String result = gptjModel.generate(prompt, config, true);
            gptjModel.chatCompletion(
                    List.of(Map.of("role", "system", "content", "You are a helpful assistant"),
                            Map.of("role", "user", "content", "Add 2+2")), config, true, true);


        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
