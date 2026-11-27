{
  "targets": [
    {
      "target_name": "Jurischain",
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
        ".",
        "../../include"
      ],
      "sources": [
        "main.cpp"
      ],
      "cflags_cc": [
        "-Wall"
      ],
      "xcode_settings": {
        "WARNING_CFLAGS": [
          "-Wall"
        ]
      }
    }
  ]
}
