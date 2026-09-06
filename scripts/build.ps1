param(
    [Parameter(Mandatory = $true)]
    [string]$Zig
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$source = Join-Path $repoRoot 'src\canoe_analog_state.c'
$output = Join-Path $repoRoot 'dist\libcanoe_analog_state.so'

# SNES Classic Mini: ARMv7, hard-float Linux userspace compatible with glibc 2.17.
& $Zig cc `
    -target arm-linux-gnueabihf.2.17 `
    -shared -fPIC -O2 `
    -o $output $source -ldl

if ($LASTEXITCODE -ne 0) {
    throw 'Zig failed to build the library.'
}

Get-FileHash -LiteralPath $output -Algorithm SHA256
