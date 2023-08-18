{
  "targets": [
    {
      "target_name": "gpt4all", # gpt4all-ts will cause compile error
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../../gpt4all-backend",
      ],
      "sources": [ 
        # PREVIOUS VERSION: had to required the sources, but with newest changes do not need to
        #"../../gpt4all-backend/llama.cpp/examples/common.cpp",
        #"../../gpt4all-backend/llama.cpp/ggml.c",
        #"../../gpt4all-backend/llama.cpp/llama.cpp",
        # "../../gpt4all-backend/utils.cpp", 
        "../../gpt4all-backend/llmodel_c.cpp",
        "../../gpt4all-backend/llmodel.cpp",
        "prompt.cc",
        "index.cc",
       ],
      "conditions": [
        ['OS=="mac"', {
            'xcode_settings': {
                'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
            },
            'defines': [
                'LIB_FILE_EXT=".dylib"',
                'NAPI_CPP_EXCEPTIONS',
            ],
            'cflags_cc': [
                "-fexceptions"
            ]
        }],
        ['OS=="win"', {
            'defines': [
                'LIB_FILE_EXT=".dll"',
                'NAPI_CPP_EXCEPTIONS',
            ],
            "msvs_settings": {
                "VCCLCompilerTool": {
                    "AdditionalOptions": [
                        "/std:c++20",
                        "/EHsc",
                  ], 
                },
            },
        }],
        ['OS=="linux"', {
            'defines': [
                'LIB_FILE_EXT=".so"',
                'NAPI_CPP_EXCEPTIONS',
            ],
            'cflags_cc!': [
                '-fno-rtti',
            ],
            'cflags_cc': [
                '-std=c++2a',
                '-fexceptions'
            ]
        }]
      ]
    }]
}
