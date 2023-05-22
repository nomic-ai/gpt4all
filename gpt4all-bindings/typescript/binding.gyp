{
  "targets": [
    {
      "target_name": "gpt4allts", # gpt4all-ts will cause compile error
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../../gpt4all-backend/llama.cpp/", # need to include llama.cpp because the include paths for examples/common.h include llama.h relatively
        "../../gpt4all-backend",
      ],
      "sources": [ # is there a better way to do this 
        "../../gpt4all-backend/llama.cpp/examples/common.cpp",
        "../../gpt4all-backend/llama.cpp/ggml.c",
        "../../gpt4all-backend/llama.cpp/llama.cpp",
        "../../gpt4all-backend/utils.cpp", 
        "../../gpt4all-backend/llmodel_c.cpp",
        "../../gpt4all-backend/gptj.cpp",
        "../../gpt4all-backend/llamamodel.cpp",
        "../../gpt4all-backend/mpt.cpp",
        "stdcapture.cc",
        "index.cc",
       ],
      "conditions": [
        ['OS=="mac"', {
            'defines': [
                'NAPI_CPP_EXCEPTIONS'
            ],
        }],
        ['OS=="win"', {
            'defines': [
                'NAPI_CPP_EXCEPTIONS',
                "__AVX2__" # allows SIMD: https://discord.com/channels/1076964370942267462/1092290790388150272/1107564673957630023
            ],
            "msvs_settings": {
                "VCCLCompilerTool": {
                    "AdditionalOptions": [
                        "/std:c++20",
                        "/EHsc"
                    ], 
                },  
            },
        }]
      ]
    }]
}
