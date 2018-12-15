{
  "targets": [
    {
      "target_name": "deltachat",
      "conditions": [
        [ "OS == 'win'", {}],
        [ "OS == 'linux'", {
          "libraries": [
            "../deltachat-core/builddir/src/libdeltachat.a",
            "../deltachat-core/builddir/libs/netpgp/libnetpgp.a",
            "/usr/local/lib/libetpan.a",
            "/usr/local/lib/libsasl2.a",
            "/usr/local/lib/libssl.a",
            "/usr/local/lib/libcrypto.a",
            "/usr/local/lib/libz.a",
            "/usr/local/lib/libsqlite3.a",
            "-lpthread"
          ]
        }],
        [ "OS == 'mac'", {
          "libraries": [
            "../deltachat-core/builddir/src/libdeltachat.a",
            "/usr/local/Cellar/libetpan/1.9.2_1/lib/libetpan.a",
            "../deltachat-core/builddir/libs/netpgp/libnetpgp.a",
            "-framework CoreFoundation",
            "-framework CoreServices",
            "-framework Security",
            "-lsasl2",
            "-lssl",
            "-lsqlite3",
            "-lpthread"
          ]
        }]
      ],
      "sources": [
        "./src/module.c",
        "./src/eventqueue.c",
        "./src/strtable.c"
      ],
      "include_dirs": [
        "deltachat-core/src",
        "<!(node -e \"require('napi-macros')\")"
      ]
    }
  ]
}
