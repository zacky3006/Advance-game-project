# Helpers for verifying the actual packaged windows, not PIE.
if (-not ('DirtyWorkWindow' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class DirtyWorkWindow {
    [StructLayout(LayoutKind.Sequential)] public struct Rect { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr window);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr window, IntPtr after, int x, int y, int width, int height, uint flags);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr window, out Rect rect);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint dx, uint dy, uint data, UIntPtr extra);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
}
'@
}
[void][DirtyWorkWindow]::SetProcessDPIAware()

function Click-PackagedGameWindow {
    param([int]$ProcessId, [double]$XFraction, [double]$YFraction)
    $game = Get-Process -Id $ProcessId -ErrorAction Stop
    if (-not $game.MainWindowHandle) { throw 'The game has no window.' }
    [void][DirtyWorkWindow]::SetForegroundWindow($game.MainWindowHandle)
    $rect = New-Object DirtyWorkWindow+Rect
    [void][DirtyWorkWindow]::GetWindowRect($game.MainWindowHandle, [ref]$rect)
    [void][DirtyWorkWindow]::SetCursorPos(
        [int]($rect.Left+($rect.Right-$rect.Left)*$XFraction),
        [int]($rect.Top+($rect.Bottom-$rect.Top)*$YFraction))
    Start-Sleep -Milliseconds 200
    [DirtyWorkWindow]::mouse_event(2,0,0,0,[UIntPtr]::Zero)
    Start-Sleep -Milliseconds 100
    [DirtyWorkWindow]::mouse_event(4,0,0,0,[UIntPtr]::Zero)
}

function Get-PackagedGameWindows {
    param([string]$GameFolder)
    $fullFolder = (Resolve-Path -LiteralPath $GameFolder).Path
    Get-Process -Name 'ArenaGame*' -ErrorAction SilentlyContinue | Where-Object {
        $_.MainWindowHandle -ne 0 -and $_.Path -and
        $_.Path.StartsWith($fullFolder, [StringComparison]::OrdinalIgnoreCase)
    }
}

function Send-PackagedGameKey {
    param([int]$ProcessId, [int]$VirtualKey)
    $game = Get-Process -Id $ProcessId -ErrorAction Stop
    if (-not $game.MainWindowHandle) { throw 'The game has no window.' }
    [void][DirtyWorkWindow]::SetForegroundWindow($game.MainWindowHandle)
    [void][DirtyWorkWindow]::PostMessage($game.MainWindowHandle, 0x100, [IntPtr]$VirtualKey, [IntPtr]0)
    Start-Sleep -Milliseconds 100
    [void][DirtyWorkWindow]::PostMessage($game.MainWindowHandle, 0x101, [IntPtr]$VirtualKey, [IntPtr]0)
}

function Send-PackagedGameText {
    param([int]$ProcessId, [string]$Text)
    $game = Get-Process -Id $ProcessId -ErrorAction Stop
    foreach ($character in $Text.ToCharArray()) {
        [void][DirtyWorkWindow]::PostMessage($game.MainWindowHandle, 0x102, [IntPtr][int]$character, [IntPtr]0)
    }
}

function Save-PackagedGameScreenshot {
    param([int]$ProcessId, [string]$FilePath)
    Add-Type -AssemblyName System.Drawing
    $game = Get-Process -Id $ProcessId -ErrorAction Stop
    [void][DirtyWorkWindow]::SetForegroundWindow($game.MainWindowHandle)
    Start-Sleep -Milliseconds 400
    $rect = New-Object DirtyWorkWindow+Rect
    [void][DirtyWorkWindow]::GetWindowRect($game.MainWindowHandle, [ref]$rect)
    $bitmap = New-Object Drawing.Bitmap ($rect.Right-$rect.Left), ($rect.Bottom-$rect.Top)
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
        $bitmap.Save($FilePath, [Drawing.Imaging.ImageFormat]::Png)
    } finally { $graphics.Dispose(); $bitmap.Dispose() }
}

function Close-PackagedGameWindow {
    param([int]$ProcessId)
    $game = Get-Process -Id $ProcessId -ErrorAction Stop
    [void][DirtyWorkWindow]::PostMessage($game.MainWindowHandle, 0x10, [IntPtr]0, [IntPtr]0)
}
