## Latest News

GPT4All v3.4.1 was released on October 11th, and fixes several issues with LocalDocs from the previous release.

GPT4All v3.4.2 was released on October 16th, and fixes more issues with LocalDocs.

**IMPORTANT NOTE:** If you are coming from v3.4.0, be sure to "Rebuild" your collections at least once after updating!

---

GPT4All v3.4.0 was released on October 8th. Changes include:

* **Attached Files:** You can now attach a small Microsoft Excel spreadsheet (.xlsx) to a chat message and ask the model about it.
* **LocalDocs Accuracy:** The LocalDocs algorithm has been enhanced to find more accurate references for some queries.
* **Word Document Support:** LocalDocs now supports Microsoft Word (.docx) documents natively.
  * **IMPORTANT NOTE:** If .docx files are not found, make sure Settings > LocalDocs > Allowed File Extensions includes "docx".
* **Forgetful Model Fixes:** Issues with the "Redo last chat response" button, and with continuing chats from previous sessions, have been fixed.
* **Chat Saving Improvements:** On exit, GPT4All will no longer save chats that are not new or modified. As a bonus, downgrading without losing access to all chats will be possible in the future, should the need arise.
* **UI Fixes:** The model list no longer scrolls to the top when you start downloading a model.
* **New Models:** LLama 3.2 Instruct 3B and 1B models now available in model list.
