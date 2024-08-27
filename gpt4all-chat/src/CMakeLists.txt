qt_add_executable(chat
    main.cpp
    chat.cpp chat.h
    chatapi.cpp chatapi.h
    chatlistmodel.cpp chatlistmodel.h
    chatllm.cpp chatllm.h
    chatmodel.h
    chatviewtextprocessor.cpp chatviewtextprocessor.h
    database.cpp database.h
    download.cpp download.h
    embllm.cpp embllm.h
    llm.cpp llm.h
    localdocs.cpp localdocs.h
    localdocsmodel.cpp localdocsmodel.h
    logger.cpp logger.h
    modellist.cpp modellist.h
    mysettings.cpp mysettings.h
    network.cpp network.h
    server.cpp server.h
)

qt_add_qml_module(chat
    URI gpt4all
    VERSION 1.0
    NO_CACHEGEN
    QML_FILES
      main.qml
      qml/AddCollectionView.qml
      qml/AddModelView.qml
      qml/ApplicationSettings.qml
      qml/ChatDrawer.qml
      qml/ChatView.qml
      qml/CollectionsDrawer.qml
      qml/HomeView.qml
      qml/LocalDocsSettings.qml
      qml/LocalDocsView.qml
      qml/ModelSettings.qml
      qml/ModelsView.qml
      qml/NetworkDialog.qml
      qml/NewVersionDialog.qml
      qml/PopupDialog.qml
      qml/SettingsView.qml
      qml/StartupDialog.qml
      qml/SwitchModelDialog.qml
      qml/Theme.qml
      qml/ThumbsDownDialog.qml
      qml/Toast.qml
      qml/ToastManager.qml
      qml/MyBusyIndicator.qml
      qml/MyButton.qml
      qml/MyCheckBox.qml
      qml/MyComboBox.qml
      qml/MyDialog.qml
      qml/MyDirectoryField.qml
      qml/MyFancyLink.qml
      qml/MyMenu.qml
      qml/MyMenuItem.qml
      qml/MyMiniButton.qml
      qml/MySettingsButton.qml
      qml/MySettingsDestructiveButton.qml
      qml/MySettingsLabel.qml
      qml/MySettingsStack.qml
      qml/MySettingsTab.qml
      qml/MySlug.qml
      qml/MyTextArea.qml
      qml/MyTextButton.qml
      qml/MyTextField.qml
      qml/MyToolButton.qml
      qml/MyWelcomeButton.qml
)
