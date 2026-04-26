<#
.SYNOPSIS
    Dataset Tools smoke test script.
    Runs L1 library tests and L3 build verification.

.DESCRIPTION
    Executes all available test executables and verifies build outputs.
    Exit code equals the number of failures (0 = all passed).

.PARAMETER BuildDir
    Path to the build output directory containing executables.

.PARAMETER TestDataDir
    Path to the test data directory.

.EXAMPLE
    .\smoke-test.ps1 -BuildDir build/bin -TestDataDir test-data
#>

param(
    [string]$BuildDir = "build/bin",
    [string]$TestDataDir = "test-data"
)

$ErrorCount = 0
$PassCount = 0

function Write-TestResult {
    param([string]$Status, [string]$Name, [string]$Detail = "")
    if ($Status -eq "PASS") {
        Write-Host "  PASS: $Name" -ForegroundColor Green
        $script:PassCount++
    } elseif ($Status -eq "FAIL") {
        Write-Host "  FAIL: $Name $Detail" -ForegroundColor Red
        $script:ErrorCount++
    } elseif ($Status -eq "SKIP") {
        Write-Host "  SKIP: $Name $Detail" -ForegroundColor Yellow
    }
}

function Test-Exe {
    param(
        [string]$Name,
        [string[]]$Arguments,
        [int]$ExpectedExit = 0,
        [int]$TimeoutSec = 300
    )

    $exe = Join-Path $BuildDir "$Name.exe"
    if (!(Test-Path $exe)) {
        Write-TestResult "SKIP" "$Name" "(not found: $exe)"
        return
    }

    try {
        $process = Start-Process -FilePath $exe -ArgumentList $Arguments `
            -Wait -PassThru -NoNewWindow -RedirectStandardError "$env:TEMP\${Name}_stderr.txt" `
            -RedirectStandardOutput "$env:TEMP\${Name}_stdout.txt"

        # Timeout check
        if (!$process.WaitForExit($TimeoutSec * 1000)) {
            $process.Kill()
            Write-TestResult "FAIL" "$Name" "(timeout after ${TimeoutSec}s)"
            return
        }

        if ($process.ExitCode -ne $ExpectedExit) {
            $stderr = ""
            if (Test-Path "$env:TEMP\${Name}_stderr.txt") {
                $stderr = Get-Content "$env:TEMP\${Name}_stderr.txt" -Raw -ErrorAction SilentlyContinue
            }
            Write-TestResult "FAIL" "$Name" "(exit=$($process.ExitCode), expected=$ExpectedExit) $stderr"
        } else {
            Write-TestResult "PASS" "$Name"
        }
    }
    catch {
        Write-TestResult "FAIL" "$Name" "(exception: $($_.Exception.Message))"
    }
}

# =========================================================================
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Dataset Tools Smoke Test" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# -------------------------------------------------------------------------
# L3: Build output verification
# -------------------------------------------------------------------------
Write-Host "[L3] Build Output Verification" -ForegroundColor Cyan

$requiredApps = @("MinLabel", "SlurCutter", "AudioSlicer", "LyricFA", "HubertFA", "GameInfer")
foreach ($app in $requiredApps) {
    $path = Join-Path $BuildDir "$app.exe"
    if (Test-Path $path) {
        Write-TestResult "PASS" "BD-004 $app.exe exists"
    } else {
        Write-TestResult "FAIL" "BD-004 $app.exe missing"
    }
}

$requiredTests = @("TestAudioUtil", "TestGame", "TestSome", "TestRmvpe")
foreach ($test in $requiredTests) {
    $path = Join-Path $BuildDir "$test.exe"
    if (Test-Path $path) {
        Write-TestResult "PASS" "BD-004 $test.exe exists"
    } else {
        Write-TestResult "FAIL" "BD-004 $test.exe missing"
    }
}

$requiredDlls = @("audio-util", "game-infer", "some-infer", "rmvpe-infer")
foreach ($dll in $requiredDlls) {
    $path = Join-Path $BuildDir "$dll.dll"
    if (Test-Path $path) {
        Write-TestResult "PASS" "BD-005 $dll.dll exists"
    } else {
        Write-TestResult "FAIL" "BD-005 $dll.dll missing"
    }
}

Write-Host ""

# -------------------------------------------------------------------------
# L1: Library tests (require test-data)
# -------------------------------------------------------------------------
Write-Host "[L1] Library Unit Tests" -ForegroundColor Cyan

$audioDir = Join-Path $TestDataDir "audio"
$modelDir = Join-Path $TestDataDir "models"

# --- TestAudioUtil ---
$testAudio = Join-Path $audioDir "test_mono_44100.wav"
$outWav = Join-Path $env:TEMP "test_au_out.wav"

if (Test-Path $testAudio) {
    # Basic test (backward compatible)
    Test-Exe "TestAudioUtil" @($testAudio, $outWav)

    # Verify output file
    if (Test-Path $outWav) {
        if ((Get-Item $outWav).Length -gt 0) {
            Write-TestResult "PASS" "TestAudioUtil output WAV valid"
        } else {
            Write-TestResult "FAIL" "TestAudioUtil output WAV is empty"
        }
        Remove-Item $outWav -ErrorAction SilentlyContinue
    } else {
        Write-TestResult "FAIL" "TestAudioUtil output WAV not created"
    }

    # Full test suite
    $outWavAll = Join-Path $env:TEMP "test_au_all_out.wav"
    Test-Exe "TestAudioUtil" @($testAudio, $outWavAll, "--all")
    Remove-Item $outWavAll -ErrorAction SilentlyContinue
} else {
    Write-TestResult "SKIP" "TestAudioUtil" "(test audio not found: $testAudio)"
}

# --- TestRmvpe ---
$rmvpeModel = Join-Path $modelDir "rmvpe-test.onnx"
$rmvpeAudio = Join-Path $audioDir "test_16k_mono.wav"
$rmvpeCsv = Join-Path $env:TEMP "test_rmvpe_f0.csv"

if ((Test-Path $rmvpeModel) -and (Test-Path $rmvpeAudio)) {
    Test-Exe "TestRmvpe" @($rmvpeModel, $rmvpeAudio, "cpu", "0", $rmvpeCsv)

    if (Test-Path $rmvpeCsv) {
        if ((Get-Item $rmvpeCsv).Length -gt 0) {
            Write-TestResult "PASS" "TestRmvpe CSV output valid"
        } else {
            Write-TestResult "FAIL" "TestRmvpe CSV output is empty"
        }
        Remove-Item $rmvpeCsv -ErrorAction SilentlyContinue
    }
} else {
    Write-TestResult "SKIP" "TestRmvpe" "(model or audio not found)"
}

# --- TestSome ---
$someModel = Join-Path $modelDir "some-test.onnx"
$someAudio = Join-Path $audioDir "test_mono_44100.wav"
$someMidi = Join-Path $env:TEMP "test_some_out.mid"

if ((Test-Path $someModel) -and (Test-Path $someAudio)) {
    Test-Exe "TestSome" @($someModel, $someAudio, "cpu", "0", $someMidi, "120.0")

    if (Test-Path $someMidi) {
        if ((Get-Item $someMidi).Length -gt 0) {
            Write-TestResult "PASS" "TestSome MIDI output valid"
        } else {
            Write-TestResult "FAIL" "TestSome MIDI output is empty"
        }
        Remove-Item $someMidi -ErrorAction SilentlyContinue
    }
} else {
    Write-TestResult "SKIP" "TestSome" "(model or audio not found)"
}

# --- TestGame ---
$gameModelDir = Join-Path $modelDir "GAME-test"
$gameAudio = Join-Path $audioDir "test_mono_44100.wav"
$gameOutDir = Join-Path $env:TEMP "test_game_out"

if ((Test-Path $gameModelDir) -and (Test-Path $gameAudio)) {
    if (!(Test-Path $gameOutDir)) {
        New-Item -ItemType Directory -Path $gameOutDir -Force | Out-Null
    }

    Test-Exe "TestGame" @($gameAudio, $gameModelDir, $gameOutDir, "--provider", "cpu")

    $gameMidi = Join-Path $gameOutDir "test_mono_44100.mid"
    if (Test-Path $gameMidi) {
        if ((Get-Item $gameMidi).Length -gt 0) {
            Write-TestResult "PASS" "TestGame MIDI output valid"
        } else {
            Write-TestResult "FAIL" "TestGame MIDI output is empty"
        }
    }

    Remove-Item $gameOutDir -Recurse -ErrorAction SilentlyContinue
} else {
    Write-TestResult "SKIP" "TestGame" "(model or audio not found)"
}

# =========================================================================
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Total: $PassCount passed, $ErrorCount failed" -ForegroundColor $(if ($ErrorCount -gt 0) { "Red" } else { "Green" })
Write-Host "========================================" -ForegroundColor Cyan

exit $ErrorCount
