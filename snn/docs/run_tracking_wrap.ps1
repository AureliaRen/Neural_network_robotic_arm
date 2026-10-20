$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $root 'code\moving\build_ninja\fig4_closed_loop.exe'
$seeds = @(910001, 910002, 910003, 910004, 910005, 910006)
$protocol = 'wrap'
$log = Join-Path $root 'runs\tracking_wrap_jobs.csv'
"kind,seed,out,exit,seconds" | Set-Content -LiteralPath $log

$queue = New-Object System.Collections.Queue
foreach ($s in $seeds) {
    $out = Join-Path $root ("runs\tracking_wrap_train_reward_{0}" -f $s)
    $queue.Enqueue([pscustomobject]@{ kind = 'train'; seed = $s; out = $out })
}
$running = @()
while ($queue.Count -gt 0 -or $running.Count -gt 0) {
    while ($running.Count -lt 4 -and $queue.Count -gt 0) {
        $item = $queue.Dequeue()
        $job = Start-Job -ScriptBlock {
            param($exe, $protocol, $seed, $out)
            $argList = @('--mode', 'reward', '--seed', [string]$seed, '--tracking-cue', $protocol,
                '--psp-history', 'sum-arrival', '--feedback-topology', 'bank', '--feedback-scale', '1',
                '--pre-clock', 'emission', '--i-rs', '8.8779296875', '--decision-ms', '200',
                '--feedback-noise', 'off', '--noise-sampling', 'per-phase',
                '--center-cue', 'previous', '--center-action', 'compete', '--out', $out)
            $t = Measure-Command { & $exe @argList | Out-Null }
            [pscustomobject]@{ kind = 'train'; seed = $seed; out = $out; exit = $LASTEXITCODE; seconds = [math]::Round($t.TotalSeconds, 1) }
        } -ArgumentList $exe, $protocol, $item.seed, $item.out
        $running += $job
    }
    $done = Wait-Job -Job $running -Any
    $res = Receive-Job -Job $done
    Remove-Job -Job $done
    $running = @($running | Where-Object { $_.State -eq 'Running' })
    foreach ($r in @($res)) { if ($r) { $r | Export-Csv -LiteralPath $log -Append -NoTypeInformation } }
}

$queue = New-Object System.Collections.Queue
foreach ($s in $seeds) {
    $queue.Enqueue([pscustomobject]@{ kind = 'init'; seed = $s
        weights = (Join-Path $root ("runs\tracking_wrap_train_reward_{0}\weights_init.csv" -f $s))
        out = (Join-Path $root ("runs\tracking_wrap_eval_init_{0}" -f $s)) })
    $queue.Enqueue([pscustomobject]@{ kind = 'final'; seed = $s
        weights = (Join-Path $root ("runs\tracking_wrap_train_reward_{0}\weights_final.csv" -f $s))
        out = (Join-Path $root ("runs\tracking_wrap_eval_final_reward_{0}" -f $s)) })
}
$running = @()
while ($queue.Count -gt 0 -or $running.Count -gt 0) {
    while ($running.Count -lt 4 -and $queue.Count -gt 0) {
        $item = $queue.Dequeue()
        $job = Start-Job -ScriptBlock {
            param($exe, $protocol, $kind, $seed, $weights, $out)
            $argList = @('--mode', 'control', '--tracking-cue', $protocol, '--eval', $weights,
                '--psp-history', 'sum-arrival', '--feedback-topology', 'bank', '--feedback-scale', '1',
                '--pre-clock', 'emission', '--i-rs', '8.8779296875', '--decision-ms', '200',
                '--feedback-noise', 'off', '--noise-sampling', 'per-phase',
                '--center-cue', 'previous', '--center-action', 'compete', '--eval-seed', '4242', '--out', $out)
            $t = Measure-Command { & $exe @argList | Out-Null }
            [pscustomobject]@{ kind = $kind; seed = $seed; out = $out; exit = $LASTEXITCODE; seconds = [math]::Round($t.TotalSeconds, 1) }
        } -ArgumentList $exe, $protocol, $item.kind, $item.seed, $item.weights, $item.out
        $running += $job
    }
    $done = Wait-Job -Job $running -Any
    $res = Receive-Job -Job $done
    Remove-Job -Job $done
    $running = @($running | Where-Object { $_.State -eq 'Running' })
    foreach ($r in @($res)) { if ($r) { $r | Export-Csv -LiteralPath $log -Append -NoTypeInformation } }
}
Write-Output 'tracking wrap done'
