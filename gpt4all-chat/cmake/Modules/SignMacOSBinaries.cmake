function(install_sign_osx tgt)
    install(CODE "execute_process(COMMAND codesign --options runtime --timestamp -s \"${MAC_SIGNING_IDENTITY}\" $<TARGET_FILE:${tgt}>)")
endfunction()