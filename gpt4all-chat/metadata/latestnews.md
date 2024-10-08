## Latest News

GPT4All v3.4.0 was released on October 8th. Changes include:

* **Attached Files:** You can now attach a small Microsoft Excel spreadsheet (.xlsx) to a chat message and ask the model about it.
* **LocalDocs Accuracy:** The LocalDocs algorithm has been enhanced to find more accurate references for some queries.
* **Word Document Support:** LocalDocs now supports Microsoft Word (.docx) documents natively.
  * **IMPORTANT NOTE:** If .docx files are not found, make sure Settings > LocalDocs > Allowed File Extensions includes "docx".
* **Forgetful Model Fixes:** Issues with the "Redo last chat response" button, and with continuing chats from previous sessions, have been fixed.
* **Chat Saving Improvements:** On exit, GPT4All will no longer save chats that are not new or modified. As a bonus, downgrading without losing access to all chats will be possible in the future, should the need arise.
* **UI Fixes:** The model list no longer scrolls to the top when you start downloading a model.
