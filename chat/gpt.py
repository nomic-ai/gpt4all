from nomic.gpt4all import GPT4All
m = GPT4All()
m.open()


def ask(input_string):
    out = m.prompt(input_string)
    print(out)
    return out


#ask("Reverse a string in python.")
