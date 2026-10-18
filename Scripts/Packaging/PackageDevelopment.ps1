$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$engineRoot = 'C:/Program Files/Epic Games/UE_5.7'
$archiveRoot = Join-Path $projectRoot 'Builds/Development'
$projectFile = Join-Path $projectRoot 'ArenaGame.uproject'
$uat = Join-Path $engineRoot 'Engine/Build/BatchFiles/RunUAT.bat'
& $uat BuildCookRun "-project=$projectFile" -noP4 -platform=Win64 `
    -clientconfig=Development -build -cook -stage -pak -iostore -archive `
    "-archivedirectory=$archiveRoot" -prereqs -utf8output -unattended
if ($LASTEXITCODE -ne 0) { throw "Packaging failed (exit code $LASTEXITCODE)." }
$gameFolder = Join-Path $archiveRoot 'Windows'
if (-not (Test-Path -LiteralPath (Join-Path $gameFolder 'ArenaGame.exe'))) {
    throw 'The packaged executable was not found.'
}
foreach ($name in @('HOST.bat', 'JOIN.bat', 'README.txt')) {
    $sourceText = [IO.File]::ReadAllText((Join-Path $PSScriptRoot $name))
    [IO.File]::WriteAllText((Join-Path $gameFolder $name),
        ($sourceText -replace '\r?\n', "`r`n"), [Text.Encoding]::ASCII)
}
Write-Host "Development game ready: $gameFolder"
