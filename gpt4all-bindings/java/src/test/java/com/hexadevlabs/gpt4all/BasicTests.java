package com.hexadevlabs.gpt4all;


import jnr.ffi.Memory;
import jnr.ffi.Pointer;
import jnr.ffi.Runtime;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mockito;

import org.mockito.junit.jupiter.MockitoExtension;


import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Map;

import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.*;

/**
 * These tests only test the Java implementation as the underlying backend can't be mocked.
 * These tests do serve the purpose of validating the java bits that do
 * not directly have to do with the function of the underlying gp4all library.
 */
@ExtendWith(MockitoExtension.class)
public class BasicTests {

    @Test
    public void simplePromptWithObject(){

        LLModel model = Mockito.spy(new LLModel());

        LLModel.GenerationConfig config =
                LLModel.config()
                        .withNPredict(20)
                        .build();

        // The generate method will return "4"
        doReturn("4").when( model ).generate(anyString(), eq(config), eq(true));

        LLModel.PromptMessage promptMessage1 = new LLModel.PromptMessage(LLModel.Role.SYSTEM, "You are a helpful assistant");
        LLModel.PromptMessage promptMessage2 = new LLModel.PromptMessage(LLModel.Role.USER, "Add 2+2");

        LLModel.Messages messages = new LLModel.Messages(promptMessage1, promptMessage2);

        LLModel.CompletionReturn response = model.chatCompletion(
                messages, config, true, true);

        assertTrue( response.choices().first().content().contains("4") );

        // Verifies the prompt and response are certain length.
        assertEquals( 224 , response.usage().totalTokens );
    }

    @Test
    public void simplePrompt(){

        LLModel model = Mockito.spy(new LLModel());

        LLModel.GenerationConfig config =
                LLModel.config()
                        .withNPredict(20)
                        .build();

        // The generate method will return "4"
        doReturn("4").when( model ).generate(anyString(), eq(config), eq(true));

        LLModel.ChatCompletionResponse response= model.chatCompletion(
                    List.of(Map.of("role", "system", "content", "You are a helpful assistant"),
                            Map.of("role", "user", "content", "Add 2+2")), config, true, true);

        assertTrue( response.choices.get(0).get("content").contains("4") );

        // Verifies the prompt and response are certain length.
        assertEquals( 224 , response.usage.totalTokens );
    }

    @Test
    public void testResponseCallback(){

        ByteArrayOutputStream bufferingForStdOutStream = new ByteArrayOutputStream();
        ByteArrayOutputStream bufferingForWholeGeneration = new ByteArrayOutputStream();

        LLModelLibrary.ResponseCallback responseCallback = LLModel.getResponseCallback(false, bufferingForStdOutStream, bufferingForWholeGeneration);

        // Get the runtime instance
        Runtime runtime = Runtime.getSystemRuntime();

        // Allocate memory for the byte array. Has to be null terminated

        // UTF-8 Encoding of the character: 0xF0 0x9F 0x92 0xA9
        byte[] utf8ByteArray = {(byte) 0xF0, (byte) 0x9F, (byte) 0x92, (byte) 0xA9, 0x00}; // Adding null termination

        // Optional: Converting the byte array back to a String to print the character
        String decodedString = new String(utf8ByteArray, 0, utf8ByteArray.length - 1, java.nio.charset.StandardCharsets.UTF_8);

        Pointer pointer = Memory.allocateDirect(runtime, utf8ByteArray.length);

        // Copy the byte array to the allocated memory
        pointer.put(0, utf8ByteArray, 0, utf8ByteArray.length);

        responseCallback.invoke(1, pointer);

        String result = bufferingForWholeGeneration.toString(StandardCharsets.UTF_8);

        assertEquals(decodedString, result);

    }

    @Test
    public void testResponseCallbackTwoTokens(){

        ByteArrayOutputStream bufferingForStdOutStream = new ByteArrayOutputStream();
        ByteArrayOutputStream bufferingForWholeGeneration = new ByteArrayOutputStream();

        LLModelLibrary.ResponseCallback responseCallback = LLModel.getResponseCallback(false, bufferingForStdOutStream, bufferingForWholeGeneration);

        // Get the runtime instance
        Runtime runtime = Runtime.getSystemRuntime();

        // Allocate memory for the byte array. Has to be null terminated

        // UTF-8 Encoding of the character: 0xF0 0x9F 0x92 0xA9
        byte[] utf8ByteArray = {  (byte) 0xF0, (byte) 0x9F, 0x00}; // Adding null termination
        byte[] utf8ByteArray2 = { (byte) 0x92, (byte) 0xA9, 0x00}; // Adding null termination

        // Optional: Converting the byte array back to a String to print the character
        Pointer pointer = Memory.allocateDirect(runtime, utf8ByteArray.length);

        // Copy the byte array to the allocated memory
        pointer.put(0, utf8ByteArray, 0, utf8ByteArray.length);

        responseCallback.invoke(1, pointer);
        // Copy the byte array to the allocated memory
        pointer.put(0, utf8ByteArray2, 0, utf8ByteArray2.length);

        responseCallback.invoke(2, pointer);

        String result = bufferingForWholeGeneration.toString(StandardCharsets.UTF_8);

        assertEquals("\uD83D\uDCA9", result);

    }


    @Test
    public void testResponseCallbackExpectError(){

        ByteArrayOutputStream bufferingForStdOutStream = new ByteArrayOutputStream();
        ByteArrayOutputStream bufferingForWholeGeneration = new ByteArrayOutputStream();

        LLModelLibrary.ResponseCallback responseCallback = LLModel.getResponseCallback(false, bufferingForStdOutStream, bufferingForWholeGeneration);

        // Get the runtime instance
        Runtime runtime = Runtime.getSystemRuntime();

        // UTF-8 Encoding of the character: 0xF0 0x9F 0x92 0xA9
        byte[] utf8ByteArray = {(byte) 0xF0, (byte) 0x9F, (byte) 0x92, (byte) 0xA9}; // No null termination

        Pointer pointer = Memory.allocateDirect(runtime, utf8ByteArray.length);

        // Copy the byte array to the allocated memory
        pointer.put(0, utf8ByteArray, 0, utf8ByteArray.length);

        Exception exception = assertThrows(RuntimeException.class, () -> responseCallback.invoke(1, pointer));

        assertEquals("Empty array or not null terminated", exception.getMessage());

        // With empty array
        utf8ByteArray = new byte[0];
        pointer.put(0, utf8ByteArray, 0, utf8ByteArray.length);

        Exception exceptionN = assertThrows(RuntimeException.class, () -> responseCallback.invoke(1, pointer));

        assertEquals("Empty array or not null terminated", exceptionN.getMessage());

    }

}
