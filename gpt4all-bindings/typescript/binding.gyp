{
  "targets": [
    {
      "target_name": "gpt4allts",
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../../gpt4all-backend/llama.cpp/",
        "../../gpt4all-backend",
      ],
      "sources": [
        "../../gpt4all-backend/llama.cpp/examples/common.cpp",
        "../../gpt4all-backend/llama.cpp/ggml.c",
        "../../gpt4all-backend/llama.cpp/llama.cpp",
        "../../gpt4all-backend/utils.cpp", 
        "../../gpt4all-backend/llmodel_c.cpp",
        "../../gpt4all-backend/gptj.cpp",
        "../../gpt4all-backend/llamamodel.cpp",
        "../../gpt4all-backend/mpt.cpp",
        "index.cc",
       ],

      "conditions": [
        ['OS=="mac"', {
            'defines': [
                'NAPI_DISABLE_CPP_EXCEPTIONS'
            ],
        }],
        ['OS=="win"', {
            'defines': [
                'NAPI_DISABLE_CPP_EXCEPTIONS',
                "__AVX2__"
            ],
            "msvs_settings": {
                "VCCLCompilerTool": { "AdditionalOptions": ["/std:c++20"], },
            },
        }, {
        }]
      ],
    },

  ]
}
