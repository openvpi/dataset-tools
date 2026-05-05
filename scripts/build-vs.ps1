$env:QT_DIR = "D:\Programs\qt5\6.10.0\msvc2022_64"
$env:Qt6_DIR = $env:QT_DIR
$env:VCPKG_KEEP_ENV_VARS = "QT_DIR;Qt6_DIR"

# Import VS environment
$vsPath = "D:\Programs\vs2026"
$vcvarsall = "$vsPath\VC\Auxiliary\Build\vcvarsall.bat"

# Run vcvarsall.bat and capture the environment
$output = cmd /c "`"$vcvarsall`" x64 >nul 2>&1 && set" 2>&1
foreach ($line in $output) {
    if ($line -match "^([^=]+)=(.*)$") {
        [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
    }
}

# Now run cmake
$cmakeExe = "C:\Program Files\CMake\bin\cmake.exe"
& $cmakeExe --build build-vs --config Release 2>&1 | Tee-Object -FilePath "d:\projects\dataset-tools\build-log.txt"
