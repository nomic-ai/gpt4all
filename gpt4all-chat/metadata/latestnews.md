## Latest News

GPT4All v3.8.0 was released on January 30th. Changes include:

* **Native DeepSeek-R1-Distill Support:** GPT4All now has robust support for the DeepSeek-R1 family of distillations.
  * Several model variants are now available on the downloads page.
  * Reasoning (wrapped in "think" tags) is displayed similarly to the Reasoner model.
  * The DeepSeek-R1 Qwen pretokenizer is now supported, resolving the loading failure in previous versions.
  * The model is now configured with a GPT4All-compatible prompt template by default.
* **Chat Templating Overhaul:** The template parser has been *completely* replaced with one that has much better compatibility with common models.
* **Code Interpreter Fixes:**
  * An issue preventing the code interpreter from logging a single string in v3.7.0 has been fixed.
  * The UI no longer freezes while the code interpreter is running a computation.
* **Local Server Fixes:**
  * An issue preventing the server from using LocalDocs after the first request since v3.5.0 has been fixed.
  * System messages are now correctly hidden from the message history.
