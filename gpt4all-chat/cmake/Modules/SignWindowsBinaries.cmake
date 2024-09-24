function(sign_target_windows tgt)
    if(WIN32 AND GPT4ALL_SIGN_INSTALL)
        add_custom_command(TARGET ${tgt}
            POST_BUILD
            COMMAND AzureSignTool.exe sign
                -du "https://www.nomic.ai/gpt4all"
                -kvu https://gpt4all.vault.azure.net
                -kvi "$Env{AZSignGUID}"
                -kvs "$Env{AZSignPWD}"
                -kvc "$Env{AZSignCertName}"
                -kvt "$Env{AZSignTID}"
                -tr http://timestamp.digicert.com
                -v
                $<TARGET_FILE:${tgt}>
        )
    endif()
endfunction()
