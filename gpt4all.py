
# to call the c runtime
import subprocess

# to check the os for correct runtime binary 
import platform

# TODO to determine resources available for c runtime
import threading

# to prevent remote code execution through prompt input
import shlex


model_file = "chat/gpt4all-lora-quantized.bin"

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
    command = ["chat/gpt4all-lora-quantized-" + load_correct_runtime()
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
    	# sanitize input with shlex to prevent code injection
        sanitized_prompt = shlex.quote(input(">"))
        
        # Convert special characters to Unicode escape sequences, hopefully
        return sanitized_prompt.encode('unicode_escape').decode('utf-8')
    else:
        return "hello I am testing"

# Call the function with the desired parameters
gpt4all_prompt(ask_question(), 4, model_file)
