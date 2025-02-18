add_library(ollama-hpp INTERFACE)
target_include_directories(ollama-hpp SYSTEM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/ollama-hpp/include"
)
target_link_libraries(ollama-hpp INTERFACE
    nlohmann_json::nlohmann_json
)
