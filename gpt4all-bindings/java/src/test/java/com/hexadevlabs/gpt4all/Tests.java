package com.hexadevlabs.gpt4all;

import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;

import java.nio.file.Path;
import java.util.List;
import java.util.Map;

import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * For now disabled until can run with latest library on windows
 */
@Disabled
public class Tests {

    static LLModel model;
    static String PATH_TO_MODEL="C:\\Users\\felix\\AppData\\Local\\nomic.ai\\GPT4All\\ggml-gpt4all-j-v1.3-groovy.bin";

    @BeforeAll
    public static void before(){
        model = new LLModel(Path.of(PATH_TO_MODEL));
    }

    @Test
    public void simplePrompt(){
        LLModel.GenerationConfig config =
                LLModel.config()
                        .withNPredict(20)
                        .build();
        String prompt =
                "### Instruction: \n" +
                "The prompt below is a question to answer, a task to complete, or a conversation to respond to; decide which and write an appropriate response.\n" +
                "### Prompt: \n" +
                "Add 2+2\n" +
                "### Response: 4\n" +
                "Multiply 4 * 5\n" +
                "### Response:";
        String generatedText = model.generate(prompt, config, true);
        assertTrue(generatedText.contains("20"));
    }

    @Test
    public void chatCompletion(){
        LLModel.GenerationConfig config =
                LLModel.config()
                        .withNPredict(20)
                        .build();

        LLModel.ChatCompletionResponse response= model.chatCompletion(
                List.of(Map.of("role", "system", "content", "You are a helpful assistant"),
                        Map.of("role", "user", "content", "Add 2+2")), config, true, true);

        assertTrue( response.choices.get(0).get("content").contains("4") );

    }

    @AfterAll
    public static void after() throws Exception {
        if(model != null)
            model.close();
    }


}
