{
  "targets": [
    {
      "target_name": "ts4all",
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "sources": [
         "../../gpt4all-backend/llama.cpp/ggml.c",
        "../../gpt4all-backend/llama.cpp/llama.cpp",
        "index.cc",
       ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../../gpt4all-backend"
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
