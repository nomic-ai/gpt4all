package com.hexadevlabs.gpt4all;

public class PromptIsTooLongException extends RuntimeException {
    public PromptIsTooLongException(String message) {
        super(message);
    }
}
