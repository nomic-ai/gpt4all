{
  "targets": [
    {
      "target_name": "gpt4all",
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "backend",
      ],
      "sources": [
        "backend/llmodel_c.cpp",
        "backend/llmodel.cpp",
        "backend/dlhandle.cpp",
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
                "-fexceptions",
                "-std=c++20"
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
