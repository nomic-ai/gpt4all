import wrapper as ll
import ctypes


class GPT4ALL_GPTJ:
    def __init__(self, model_path):
        self.gptj = ll.llmodel_gptj_create()
        self.load_model(model_path)
        self.ctx = self.set_context_defaults()

    def load_model(self, model_path):
        loaded = ll.llmodel_loadModel(self.gptj, model_path)
        if loaded:
            print("Model loaded successfully.")
        else:
            print("Failed to load the model.")
            raise Exception("Failed to load the model.")

    def set_context_defaults(self):
        ctx = ll.LLModelPromptContext()
        ctx.logits = ctypes.POINTER(ctypes.c_float)()
        ctx.logits_size = 0
        ctx.tokens = ctypes.POINTER(ctypes.c_int32)()
        ctx.tokens_size = 0

        ctx.n_past = 0
        ctx.n_ctx = 0
        ctx.n_predict = 200
        ctx.top_k = 40
        ctx.top_p = 0.9
        ctx.temp = 0.9
        ctx.n_batch = 9

        ctx.repeat_penalty = 1.1
        ctx.repeat_last_n = 64
        ctx.context_erase = 0.0

        return ctx

    @staticmethod
    def prompt_callback(prompt_id):
        pass
        return True

    @staticmethod
    def response_callback(prompt_id, response):
        resp = response.decode("utf-8")
        print(resp, end="", flush=True)
        return True

    @staticmethod
    def recalculate_callback(recalculate):
        print("Recalculate callback called with recalculate", recalculate)
        return True

    def run_prompt(self, prompt_text):
        ll.llmodel_prompt(
            self.gptj,
            prompt_text,
            self.prompt_callback,
            self.response_callback,
            self.recalculate_callback,
            self.ctx
        )

    def destroy(self):
        ll.llmodel_gptj_destroy(self.gptj)


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("-m", "--model", help="Path to the model file.")

    args = parser.parse_args()

    if args.model:
        model_path = args.model
    else:
        print("Please specify the model path.")
        exit(1)

    gptj_instance = GPT4ALL_GPTJ(model_path)

    prompt = input("\n ⇢ ")
    gptj_instance.run_prompt(prompt)

    print("\n")
    gptj_instance.destroy()
