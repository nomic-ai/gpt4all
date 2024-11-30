execute_process(
    COMMAND ln -s bin/gpt4all.app gpt4all
    WORKING_DIRECTORY ${CPACK_TEMPORARY_INSTALL_DIRECTORY}/ALL_IN_ONE
)