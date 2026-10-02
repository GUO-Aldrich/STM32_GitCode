param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('build','rebuild','flash','buildAndFlash','clean')]
    [string]$Action
)

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32 {
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc enumProc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, System.Text.StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll", SetLastError = true)] public static extern uint SendInput(uint nInputs, INPUT[] pInputs, int cbSize);
    [StructLayout(LayoutKind.Sequential)]
    public struct INPUT {
        public uint type;
        public INPUTUNION U;
    }
    [StructLayout(LayoutKind.Explicit)]
    public struct INPUTUNION {
        [FieldOffset(0)] public KEYBDINPUT ki;
    }
    [StructLayout(LayoutKind.Sequential)]
    public struct KEYBDINPUT {
        public ushort wVk;
        public ushort wScan;
        public uint dwFlags;
        public uint time;
        public IntPtr dwExtraInfo;
    }
    public const uint INPUT_KEYBOARD = 1;
    public const uint KEYEVENTF_KEYUP = 0x0002;
    public const ushort VK_MENU = 0x12;
    public const ushort VK_CONTROL = 0x11;
    public const ushort VK_SHIFT = 0x10;
    public static void PressKey(ushort vk) {
        INPUT[] down = { new INPUT { type = INPUT_KEYBOARD, U = new INPUTUNION { ki = new KEYBDINPUT { wVk = vk } } } };
        INPUT[] up = { new INPUT { type = INPUT_KEYBOARD, U = new INPUTUNION { ki = new KEYBDINPUT { wVk = vk, dwFlags = KEYEVENTF_KEYUP } } } };
        SendInput(1, down, Marshal.SizeOf(typeof(INPUT)));
        System.Threading.Thread.Sleep(30);
        SendInput(1, up, Marshal.SizeOf(typeof(INPUT)));
    }
    public static void PressHotkey(params ushort[] keys) {
        foreach (var k in keys) {
            INPUT[] down = { new INPUT { type = INPUT_KEYBOARD, U = new INPUTUNION { ki = new KEYBDINPUT { wVk = k } } } };
            SendInput(1, down, Marshal.SizeOf(typeof(INPUT)));
            System.Threading.Thread.Sleep(15);
        }
        for (int i = keys.Length - 1; i >= 0; i--) {
            INPUT[] up = { new INPUT { type = INPUT_KEYBOARD, U = new INPUTUNION { ki = new KEYBDINPUT { wVk = keys[i], dwFlags = KEYEVENTF_KEYUP } } } };
            SendInput(1, up, Marshal.SizeOf(typeof(INPUT)));
            System.Threading.Thread.Sleep(15);
        }
    }
}
"@

$hwnd = [IntPtr]::Zero
[Win32]::EnumWindows({ param($h, $l)
    $sb = [System.Text.StringBuilder]::new(256)
    [Win32]::GetWindowText($h, $sb, 256) | Out-Null
    if ($sb.ToString() -match "STM32F407_Default.*STM32-EIDE") {
        $script:hwnd = $h
    }
    return $true
}, [IntPtr]::Zero)

if ($hwnd -eq [IntPtr]::Zero) {
    Write-Host "ERROR: Cannot find VS Code EIDE window!" -ForegroundColor Red
    exit 1
}

Write-Host "Found EIDE window HWND=$hwnd" -ForegroundColor Green
[Win32]::SetForegroundWindow($hwnd) | Out-Null
Start-Sleep -Milliseconds 400

switch ($Action) {
    'build' {
        Write-Host "[EIDE] Build (F7)" -ForegroundColor Cyan
        [Win32]::PressKey(0x76)
    }
    'rebuild' {
        Write-Host "[EIDE] Rebuild" -ForegroundColor Cyan
        [Win32]::PressHotkey([Win32]::VK_CONTROL, [Win32]::VK_SHIFT, 0x76)
    }
    'flash' {
        Write-Host "[EIDE] Flash (Ctrl+Alt+D)" -ForegroundColor Cyan
        [Win32]::PressHotkey([Win32]::VK_CONTROL, [Win32]::VK_MENU, 0x44)
    }
    'buildAndFlash' {
        Write-Host "[EIDE] Build (F7)..." -ForegroundColor Cyan
        [Win32]::PressKey(0x76)
    }
    'clean' {
        Write-Host "[EIDE] Clean" -ForegroundColor Cyan
    }
}
Write-Host "OK" -ForegroundColor Green