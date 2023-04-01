import subprocess
import platform
import threading

model_file = "../gpt4all-lora-quantized.bin"

def gpt4all_prompt(p, t, m):
    def load_correct_runtime():
        os_name = platform.system()

        if os_name == "Windows":
            return "win64"
        elif os_name == "Linux":
            return "linux-x86"
        elif os_name == "Darwin":
            if platform.machine().startswith('arm'):
                return "OSX-m1"
            else:
                return "OSX-intel"
        else:
            return "Unknown"

    # Define the command and its arguments as a list of strings
    # would be good for me to locate the file a better way
    command = ["../gpt4all/chat/gpt4all-lora-quantized-" + load_correct_runtime()
        , "-p", p
        , "-t", str(t)
        , "-m", m
    ]

    # Run the command and capture the output
    output = subprocess.check_output(command)

    # Print the output
    print(output.decode())

def ask_question():
    if __name__ == "__main__":
        question = input("What is your question? ")
        # Convert special characters to Unicode escape sequences, hopefully
        question = question.encode('unicode_escape').decode('utf-8')
        return question
    else:
        return "hello I am testing"

# Call the function with the desired parameters
gpt4all_prompt(ask_question(), 4, model_file)
