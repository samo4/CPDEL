param([string]$action = "build", [string]$port = "COM14",  [string]$baud = "921600" )

#  -b 921600
switch ($action) {
    "build"         { idf.py build  }
    "flash"         { idf.py -p $port -b $baud flash  }
    "monitor"       { idf.py -p $port -b $baud monitor }
    "flash-monitor" { idf.py -p $port -b $baud flash monitor }
    "menuconfig"    { idf.py menuconfig }
    "clean"         { Remove-Item -Recurse -Force -ErrorAction SilentlyContinue build, sdkconfig }
    "update-deps"   { idf.py reconfigure }
    "size"          { idf.py size }
    "size-components" { idf.py size-components }
    "size-files"    { idf.py size-files }
    "build-size"    { idf.py build; idf.py size; idf.py size-components }
    default          { Write-Error "Unknown action '$action'"; exit 1 }
}
