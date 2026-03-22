# ESP-IDF helper (PowerShell / EIM).
#
# Commands:  build | flash [PORT] | monitor [PORT] | flash-monitor [PORT]
#            menuconfig | clean | update-deps | set-target
param(
    [string]$Command = "build",
    [string]$Port = "COM3"
)

$EspDir = "esp"
$EimProfile = "C:\Espressif\tools\Microsoft.v6.0.PowerShell_profile.ps1"

# Strip MSYS/MinGW environment variables inherited from Git Bash so idf.py
# doesn't reject the environment as unsupported.
foreach ($var in @('MSYSTEM', 'MSYS', 'MSYS2_PATH_TYPE', 'MINGW_PREFIX',
                   'MINGW_CHOST', 'MINGW_PACKAGE_PREFIX', 'MSYS2_ENV_CONV_EXCL',
                   'CONFIG_SITE', 'ORIGINAL_PATH', 'ORIGINAL_TEMP', 'ORIGINAL_TMP')) {
    [System.Environment]::SetEnvironmentVariable($var, $null, 'Process')
}

# Activate ESP-IDF environment via EIM profile
if (-not (Get-Command idf.py -ErrorAction SilentlyContinue)) {
    if (-not (Test-Path $EimProfile)) {
        Write-Error "EIM profile not found at $EimProfile. Run the EIM installer first."
        exit 1
    }
    Write-Host "Activating ESP-IDF via $EimProfile ..."
    . $EimProfile
}

Set-Location $EspDir

# First-run bootstrap (only for commands that need a configured project)
$NeedsBootstrap = $Command -in @('build', 'flash', 'monitor', 'flash-monitor', 'menuconfig')
if ($NeedsBootstrap -and -not (Test-Path "sdkconfig")) {
    Write-Host "Setting target to esp32s3..."
    idf.py set-target esp32s3
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host "Fetching dependencies..."
    idf.py update-dependencies
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

switch ($Command) {
    'build' {
        Write-Host "Building..."
        idf.py build
    }
    'flash' {
        Write-Host "Building and flashing to $Port..."
        idf.py -p $Port flash
    }
    'monitor' {
        Write-Host "Opening monitor on $Port..."
        idf.py -p $Port monitor
    }
    'flash-monitor' {
        Write-Host "Flashing to $Port and opening monitor..."
        idf.py -p $Port flash monitor
    }
    'menuconfig' {
        Write-Host "Opening menuconfig..."
        idf.py menuconfig
    }
    'clean' {
        Write-Host "Cleaning build directory..."
        idf.py fullclean
    }
    'update-deps' {
        Write-Host "Updating managed components..."
        idf.py update-dependencies
    }
    'set-target' {
        Write-Host "Setting target to esp32s3..."
        idf.py set-target esp32s3
    }
    default {
        Write-Error "Unknown command '$Command'. Valid: build, flash, monitor, flash-monitor, menuconfig, clean, update-deps, set-target"
        exit 1
    }
}
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
