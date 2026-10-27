$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $root 'code\moving\build_ninja\fig4_closed_loop.exe'
$seeds = @(910001, 910002, 910003, 910004, 910005, 910006)
$modes = @('reward', 'punishment')
$queue = New-Object System.Collections.Queue
foreach ($s in $seeds) {
    $queue.Enqueue([pscustomobject]@{ kind = 'init'; mode = 'reward'; seed = $s
        weights = (Join-Path $root ("runs\tracking_train_reward_{0}\weights_init.csv" -f $s))
        out = (Join-Path $root ("runs\tracking_eval_init_{0}" -f $s)) })
}
foreach ($m in $modes) {
    foreach ($s in $seeds) {
        $queue.Enqueue([pscustomobject]@{ kind = 'final'; mode = $m; seed = $s
            weights = (Join-Path $root ("runs\tracking_train_{0}_{1}\weights_final.csv" -f $m, $s))
            out = (Join-Path $root ("runs\tracking_eval_final_{0}_{1}" -f $m, $s)) })
    }
}
$running = @()
$log = Join-Path $root 'runs\tracking_eval_jobs.csv'
"kind,mode,seed,out,exit,seconds" | Set-Content -LiteralPath $log
while ($queue.Count -gt 0 -or $running.Count -gt 0) {
    while ($running.Count -lt 4 -and $queue.Count -gt 0) {
        $item = $queue.Dequeue()
        $job = Start-Job -ScriptBlock {
            param($exe, $kind, $mode, $seed, $weights, $out)
            $argList = @('--mode', 'control', '--tracking-cue', 'triangle', '--eval', $weights,
                '--psp-history', 'sum-arrival', '--feedback-topology', 'bank', '--feedback-scale', '1',
                '--pre-clock', 'emission', '--i-rs', '8.8779296875', '--decision-ms', '200',
                '--feedback-noise', 'off', '--noise-sampling', 'per-phase',
                '--center-cue', 'previous', '--center-action', 'compete', '--eval-seed', '4242', '--out', $out)
            $t = Measure-Command { & $exe @argList | Out-Null }
            [pscustomobject]@{ kind = $kind; mode = $mode; seed = $seed; out = $out; exit = $LASTEXITCODE; seconds = [math]::Round($t.TotalSeconds, 1) }
        } -ArgumentList $exe, $item.kind, $item.mode, $item.seed, $item.weights, $item.out
        $running += $job
    }
    $done = Wait-Job -Job $running -Any
    $res = Receive-Job -Job $done
    Remove-Job -Job $done
    $running = @($running | Where-Object { $_.State -eq 'Running' })
    foreach ($r in @($res)) { if ($r) { $r | Export-Csv -LiteralPath $log -Append -NoTypeInformation } }
}
Write-Output 'tracking eval done'
