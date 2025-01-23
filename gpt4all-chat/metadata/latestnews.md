## Latest News

GPT4All v3.7.0 was released on January 23rd. Changes include:

* **Windows ARM Support:** GPT4All now supports the Windows ARM platform, ensuring compatibility with devices powered by Qualcomm Snapdragon and Microsoft SQ-series processors.
  * **NOTE:** Support for GPU and/or NPU acceleration is not available at this time. Only the CPU will be used to run LLMs.
  * **NOTE:** You must install the new *Windows ARM* version of GPT4All from the website. The standard *Windows* version will not work due to emulation limitations.
* **Fixed Updating on macOS:** The maintenance tool no longer crashes when attempting to update or uninstall GPT4All on Sequoia.
  * **NOTE:** If you have installed the version from the GitHub releases as a workaround for this issue, you can safely uninstall it and switch back to the version from the website.
* **Fixed Chat Saving on macOS:** Chats now save as expected when the application is quit with Command-Q.
* **Code Interpreter Improvements:**
  * The behavior when the code takes too long to execute and times out has been improved.
  * console.log now accepts multiple arguments for better compatibility with native JavaScript.
* **Chat Templating Improvements:**
  * Two crashes and one compatibility issue have been fixed in the chat template parser.
  * The default chat template for EM German Mistral has been fixed.
  * Automatic replacements have been added for five new models as we continue to improve compatibility with common chat templates.
