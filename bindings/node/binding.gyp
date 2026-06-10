{
  "targets": [
    {
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
        ".",
        "../../include",
      ],
      "cflags": ["-Wstack-protector", "-Wall", "-g"],
      "target_name": "Jurischain",
      "sources": [ 'main.cpp' ]
    }
  ]
}