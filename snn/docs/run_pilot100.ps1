$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$trainExe = Join-Path $root "code\train\build_ninja\fig4_closed_loop.exe"
$movingExe = Join-Path $root "code\moving\build_ninja\fig4_closed_loop.exe"
$flags = @(
    "--psp-history", "sum-arrival",
    "--feedback-topology", "bank",
    "--feedback-scale", "1",
    "--pre-clock", "emission",
    "--i-rs", "8.8779296875",
    "--decision-ms", "200",
    "--feedback-noise", "off",
    "--noise-sampling", "per-phase",
    "--center-cue", "previous",
    "--center-action", "compete"
)
function Invoke-Exe {
    param($exe, $argList, $name)
    $log = Join-Path $root "runs\$name.log"
    & $exe @argList *> $log
    if ($LASTEXITCODE -ne 0) {
        throw "$name failed with exit $LASTEXITCODE; see $log"
    }
    [pscustomobject]@{ name = $name; exit = $LASTEXITCODE }
}

$results = @()
foreach ($s in @(730001, 730004, 730005, 730006)) {
    $out = Join-Path $root "runs\pilot100_reward_$s"
    if (Test-Path -LiteralPath $out) { throw "Output already exists: $out" }
    $a = $flags + @("--mode", "reward", "--cycles", "100", "--seed", "$s", "--out", $out)
    $results += Invoke-Exe $trainExe $a "train_$s"
}
$results | Export-Csv -LiteralPath (Join-Path $root "runs\pilot100_train_jobs.csv") -NoTypeInformation

$results = @()
foreach ($s in @(730001, 730002, 730003, 730004, 730005, 730006)) {
    $weights = Join-Path $root "runs\pilot100_reward_$s\weights_final.csv"
    $i = $s - 730000
    $evalSeed = 830000 + $i
    $evalOut = Join-Path $root "runs\pilot100_eval6_static_$s"
    if (Test-Path -LiteralPath $evalOut) { throw "Output already exists: $evalOut" }
    $a = $flags + @("--mode", "control", "--eval", $weights, "--eval-seed", "$evalSeed", "--out", $evalOut)
    $results += Invoke-Exe $trainExe $a "eval_static_$s"
}
$results | Export-Csv -LiteralPath (Join-Path $root "runs\pilot100_static_jobs.csv") -NoTypeInformation

$results = @()
foreach ($s in @(730001, 730002, 730003, 730004, 730005, 730006)) {
    $weights = Join-Path $root "runs\pilot100_reward_$s\weights_final.csv"
    $i = $s - 730000
    $evalSeed = 930000 + $i
    $evalOut = Join-Path $root "runs\pilot100_eval6_triangle_$s"
    if (Test-Path -LiteralPath $evalOut) { throw "Output already exists: $evalOut" }
    $a = $flags + @("--mode", "control", "--tracking-cue", "triangle", "--eval", $weights, "--eval-seed", "$evalSeed", "--out", $evalOut)
    $results += Invoke-Exe $movingExe $a "eval_triangle_$s"
}
$results | Export-Csv -LiteralPath (Join-Path $root "runs\pilot100_triangle_jobs.csv") -NoTypeInformation

Write-Output "done"
