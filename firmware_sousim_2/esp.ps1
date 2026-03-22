# ESP-IDF helper (PowerShell / EIM).
#
# Commands:  build | flash [PORT] | monitor [PORT] | flash-monitor [PORT]
#            menuconfig | clean | update-deps | set-target
#            size | size-components | size-files | build-size
param(
    [string]$Command = "build",
    [string]$Port = "COM3"
)

$EspDir = "esp"
$EimProfile = "C:\Espressif\tools\Microsoft.v6.0.PowerShell_profile.ps1"

function Write-IdfLine {
    param([string]$Line)

    if ($Line -match '(?i)(^|\s)(error:|fatal error:|FAILED:|ninja: build stopped)') {
        Write-Host $Line -ForegroundColor Red
        return
    }
    if ($Line -match '(?i)(^|\s)(warning:)') {
        Write-Host $Line -ForegroundColor Yellow
        return
    }
    if ($Line -match '(?i)(Building|Compiling|Linking|Generating|Invoking|Scanning dependencies)') {
        Write-Host $Line -ForegroundColor Cyan
        return
    }
    if ($Line -match '(?i)(Done|Succeeded|ready|Project build complete)') {
        Write-Host $Line -ForegroundColor Green
        return
    }

    Write-Host $Line
}

function Invoke-Idf {
    param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Args)

    # Keep ANSI color support enabled for tools that emit it.
    $env:CLICOLOR_FORCE = "1"

    & idf.py @Args 2>&1 | ForEach-Object {
        Write-IdfLine $_.ToString()
    }

    return $LASTEXITCODE
}

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
$NeedsBootstrap = $Command -in @(
    'build', 'flash', 'monitor', 'flash-monitor', 'menuconfig',
    'size', 'size-components', 'size-files', 'build-size'
)
if ($NeedsBootstrap -and -not (Test-Path "sdkconfig")) {
    Write-Host "Setting target to esp32s3..."
    Invoke-Idf set-target esp32s3
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host "Fetching dependencies..."
    Invoke-Idf update-dependencies
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

switch ($Command) {
    'build' {
        Write-Host "Building..."
        Invoke-Idf build
    }
    'flash' {
        Write-Host "Building and flashing to $Port..."
        Invoke-Idf -p $Port flash
    }
    'monitor' {
        Write-Host "Opening monitor on $Port..."
        idf.py -p $Port monitor
    }
    'flash-monitor' {
        Write-Host "Flashing to $Port and opening monitor..."
        Invoke-Idf -p $Port flash monitor
    }
    'menuconfig' {
        Write-Host "Opening menuconfig..."
        Invoke-Idf menuconfig
    }
    'clean' {
        Write-Host "Cleaning build directory..."
        Invoke-Idf fullclean
    }
    'update-deps' {
        Write-Host "Updating managed components..."
        Invoke-Idf update-dependencies
    }
    'set-target' {
        Write-Host "Setting target to esp32s3..."
        Invoke-Idf set-target esp32s3
    }
    'size' {
        Write-Host "Showing image size summary..."
        Invoke-Idf size
    }
    'size-components' {
        Write-Host "Showing size by component..."
        Invoke-Idf size-components
    }
    'size-files' {
        Write-Host "Showing size by object/source file..."
        Invoke-Idf size-files
    }
    'build-size' {
        Write-Host "Building..."
        Invoke-Idf build
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

        Write-Host "Image size summary:"
        Invoke-Idf size
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

        Write-Host "Size by component:"
        Invoke-Idf size-components
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

        Write-Host "Size by file:"
        Invoke-Idf size-files
    }
    default {
        Write-Error "Unknown command '$Command'. Valid: build, flash, monitor, flash-monitor, menuconfig, clean, update-deps, set-target, size, size-components, size-files, build-size"
        exit 1
    }
}
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
