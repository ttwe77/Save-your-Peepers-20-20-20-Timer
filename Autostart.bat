@echo off
chcp 65001 >nul
:: 设置工作目录为脚本自身所在路径
cd /d "%~dp0"

:: 调试信息输出
echo ============================================
echo Autostart.bat 调试信息
echo ============================================
echo 工作目录: %cd%
echo 当前时间: %date% %time%
echo.

:: 列出当前目录下的所有exe文件
echo 当前目录下的可执行文件:
dir *.exe /b
echo.

:: 遍历工作目录，启动所有非AutostartManager.exe的可执行文件
echo 开始启动程序...
echo ============================================
set count=0
for %%f in (*.exe) do (
    if /i not "%%f"=="AutostartManager.exe" (
        set /a count+=1
        echo [%count%] 正在启动: %%f
        start "" "%%f"
        if !errorlevel!==0 (
            echo     启动成功
        ) else (
            echo     启动失败，错误代码: !errorlevel!
        )
    ) else (
        echo 跳过文件: %%f (排除列表)
    )
)

echo ============================================
echo 启动完成
echo 总共启动了 %count% 个程序

:: 如果启动了0个程序，则执行python main.py
if %count%==0 (
    echo 没有找到可执行文件，尝试启动python main.py...
    python main.py
    if !errorlevel!==0 (
        echo     python main.py 启动成功
    ) else (
        echo     python main.py 启动失败，错误代码: !errorlevel!
    )
)

echo ============================================