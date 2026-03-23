param([string]$action = "build", [string]$port = "COM14")

switch ($action) {
    "build"         { idf.py build }
    "flash"         { idf.py -p $port flash }
    "monitor"       { idf.py -p $port monitor }
    "flash-monitor" { idf.py -p $port flash monitor }
    "menuconfig"    { idf.py menuconfig }
    "clean"         { Remove-Item -Recurse -Force -ErrorAction SilentlyContinue build, sdkconfig }
    "update-deps"   { idf.py reconfigure }
    "size"          { idf.py size }
    "size-components" { idf.py size-components }
    "size-files"    { idf.py size-files }
    "build-size"    { idf.py build; idf.py size; idf.py size-components }
    default          { Write-Error "Unknown action '$action'"; exit 1 }
}
