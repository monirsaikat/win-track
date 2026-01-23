{
  "targets": [
    {
      "target_name": "win_trace",
      "sources": [
        "src/addon.cc"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "defines": [
        "NAPI_CPP_EXCEPTIONS"
      ],
      "libraries": [
        "Psapi.lib",
        "Uiautomationcore.lib",
        "Ole32.lib",
        "Version.lib"
      ],
      "cflags!": [
        "-fno-exceptions"
      ],
      "cflags_cc!": [
        "-fno-exceptions"
      ],
      "msvs_settings": {
        "VCCLCompilerTool": {
          "ExceptionHandling": 1
        }
      },
      "conditions": [
        [
          "OS!=\"win\"",
          {
            "sources!": [
              "src/addon.cc"
            ],
            "sources": [
              "src/addon_stub.cc"
            ],
            "libraries!": [
              "Psapi.lib",
              "Uiautomationcore.lib",
              "Ole32.lib",
              "Version.lib"
            ]
          }
        ]
      ]
    }
  ]
}
