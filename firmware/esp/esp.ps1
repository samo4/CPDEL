param([string]$action = "build", [string]$port = "COM14", [string]$baud = "921600")

switch ($action) {
    "build"           { idf.py build }
    "flash"           { idf.py -p $port -b $baud flash }
    "flash-spiffs"    { $env:ESPPORT = $port; $env:ESPBAUD = $baud; idf.py spiffs-flash }
    "monitor"         { idf.py -p $port -b $baud monitor }
    "flash-monitor"   { idf.py -p $port -b $baud flash monitor }
    "menuconfig"      { idf.py menuconfig }
    "clean"           { Remove-Item -Recurse -Force -ErrorAction SilentlyContinue build, sdkconfig }
    "update-deps"     { idf.py reconfigure }
    "size"            { idf.py size }
    "size-components" { idf.py size-components }
    "size-files"      { idf.py size-files }
    "build-size"      { idf.py build; idf.py size; idf.py size-components }

    "ota" {
        # Prerequisites: npm install -g surge  +  surge login
        if (-not (Test-Path "build\sousim2.bin")) {
            Write-Error "build\sousim2.bin not found - run: .\esp.ps1 build"; exit 1
        }
        $s = New-Item -ItemType Directory -Path (Join-Path $env:TEMP ([IO.Path]::GetRandomFileName()))
        Copy-Item "build\sousim2.bin" "$s\sousim2.bin"
        Set-Content "$s\index.html" "sousim2 OTA firmware"
        surge $s wrathful-fight.surge.sh
        Remove-Item -Recurse -Force $s
    }

    default { Write-Error "Unknown action '$action'"; exit 1 }
}
