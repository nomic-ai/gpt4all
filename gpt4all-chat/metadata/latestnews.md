## Latest News

GPT4All v3.6.1 was released on December 20th and fixes issues with the stop generation and copy conversation buttons which were broken in v3.6.0.

---

GPT4All v3.6.0 was released on December 19th. Changes include:

* **Reasoner v1:**
  * Built-in javascript code interpreter tool.
  * Custom curated model that utilizes the code interpreter to break down, analyze, perform, and verify complex reasoning tasks.
* **Templates:** Automatically substitute chat templates that are not compatible with Jinja2Cpp in GGUFs.
* **Fixes:**
  * Remote model template to allow for XML in messages.
  * Jinja2Cpp bug that broke system message detection in chat templates.
  * LocalDocs sources displaying in unconsolidated form after v3.5.0.
