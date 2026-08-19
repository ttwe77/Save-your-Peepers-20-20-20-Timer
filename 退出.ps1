# force-exit-domain.ps1
# 需要以管理员身份运行

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public class NativeMethods {
    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr OpenDesktop(string lpszDesktop, uint dwFlags, bool fInherit, uint dwDesiredAccess);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool SwitchDesktop(IntPtr hDesktop);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool CloseDesktop(IntPtr hDesktop);
}
"@

# 权限常量：DESKTOP_SWITCHDESKTOP = 0x0100，但为保险使用全部访问权限 0x01FF
$DESKTOP_ALL_ACCESS = 0x01FF

# 尝试切换到默认桌面
$hDefault = [NativeMethods]::OpenDesktop("Default", 0, $false, $DESKTOP_ALL_ACCESS)
if ($hDefault -ne [IntPtr]::Zero) {
    $result = [NativeMethods]::SwitchDesktop($hDefault)
    if ($result) {
        Write-Host "已切换回默认桌面"
    } else {
        Write-Host "切换桌面失败，错误码: $([System.Runtime.InteropServices.Marshal]::GetLastWin32Error())"
    }
    [NativeMethods]::CloseDesktop($hDefault)
} else {
    Write-Host "无法打开 Default 桌面，错误码: $([System.Runtime.InteropServices.Marshal]::GetLastWin32Error())"
    # 尝试枚举桌面并选择一个非 "DomainExpansion" 的桌面（更复杂，此处略）
}

# 强制结束 lyzk 进程（如果存在）
Get-Process -Name "lyzk" -ErrorAction SilentlyContinue | Stop-Process -Force
if ($?) {
    Write-Host "已终止 lyzk 进程"
} else {
    Write-Host "未找到 lyzk 进程或已退出"
}