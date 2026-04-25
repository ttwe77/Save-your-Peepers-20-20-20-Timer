以下是更新后的 `README.md`，合并了您列出的新版改进点，并保持**中文优先**的三语言结构。您可以将此内容直接替换原文件。

# 守护你的眼睛 | Save Your Peepers | Бережем глазки (20-20-20 定时器)

<a id="chinese"></a>
<details>
<summary><h2>🇨🇳 中文 (点击展开/折叠)</h2></summary>

一个智能、可定制且足够“烦人”的定时器，利用著名的 **20-20-20 规则** 保护你的眼睛：*每 20 分钟，看 20 英尺（约 6 米）远的地方 20 秒*。非常适合开发者、游戏玩家以及任何长时间盯着屏幕的人。

### 新版亮点 ✨
- **中文字体优先**：自动识别系统中文字体并优先显示，告别乱码。
- **实时字体切换**：设置窗口可随时更换字体，主界面及弹窗字体同步更新。
- **智能验证码**：休息结束需输入 4 位数字，**逐位校验**、错误即清空并提示，**退格键**可删除，进度用圆点（●/○）显示。
- **全屏抢焦点**：使用 Windows API 强力抢夺焦点（`AttachThreadInput`、`SetForegroundWindow` 等），并每 800ms 持续抢焦点，应对各类全屏游戏/应用。
- **弹窗增强**：背景固定黑色、文字固定白色（确保醒目），按 `ESC` 直接跳过本轮。
- **启动即托盘**：程序启动后立即隐藏到系统托盘，双击图标恢复窗口，退出时彻底关闭。
- **跳过时播放音效**：点击“跳过本轮”也会播放随机提示音。
- **音效不重复**：记录上次播放的音效，避免连续相同；音量随机 0.6~1.0。
- **完整周期计数**：只有输入正确验证码后才增加“已完成周期数”。

### 功能特点
* **自定义声音**：应用内置 9 种声音，首次运行时会自动在 `.exe` 旁边创建 `sounds` 文件夹。把你自己的 `.mp3`、`.wav` 或 `.ogg` 文件放进去，休息时随机播放！
* **严格覆盖模式**：全屏覆盖层强制提醒你移开视线。可启用“需要验证码”，休息结束后必须输入随机 4 位数字才能解锁屏幕。
* **全局热键**：即使在全屏游戏中，也可以用自定义热键暂停或跳过当前周期。
* **系统托盘**：程序启动即最小化到托盘，不占任务栏。双击图标恢复主窗口。
* **智能提醒**：休息开始前 10 秒，在屏幕指定角落弹出温和通知（位置可自定义）。

### 使用方法（下载即运行）
1. 转到右侧的 **[Releases](../../releases)** 标签页。
2. 下载 `EyeCare_OneFile.exe`（或其他名称的 `.exe` 文件）。
3. 将它放在你喜欢的文件夹中，然后双击运行！
> **注意**：由于程序未进行代码签名，Windows SmartScreen 可能弹出蓝色警告。请点击 **“更多信息”** -> **“仍要运行”**。  
> *如果全局热键在某些游戏中不生效，请以管理员身份运行 `.exe`。*

### 自定义选项
点击主界面的 **“设置”** 按钮即可调整：
* **专注与休息时间**：调整工作分钟数和休息秒数。
* **覆盖层与验证码**：关闭全屏覆盖层（仅后台提醒），或启用验证码解锁以获得最大自律。
* **提醒位置**：选择“即将休息”提示框出现的位置（右下、左下、右上、左上）。
* **热键**：点击按钮并按下你喜欢的组合键（例如 `ctrl+shift+p`）。
* **主题与语言**：深色/浅色主题，以及中文/英文/俄语界面切换。
* **字体**：从系统字体列表中选择任意字体，实时预览。

### 编译指南（开发者）
使用以下命令可自行打包：

**PyInstaller 单文件模式**  
```bash
pyinstaller --onefile --windowed --name "EyeCare_OneFile" --icon="icon.ico" --add-data "icon.ico;." --add-data "icon.png;." --add-data "sounds;sounds" --hidden-import PIL._tkinter_finder --hidden-import customtkinter main.py
```

**Nuitka 文件夹模式**  
```bash
nuitka --standalone --windows-disable-console --enable-plugin=tk-inter --output-filename=EyeCare_Folder.exe --windows-icon-from-ico=icon.ico --include-data-file=icon.ico=icon.ico --include-data-file=icon.png=icon.png --include-data-dir=sounds=sounds --include-module=PIL._tkinter_finder --include-module=customtkinter --output-dir=dist_folder\EyeCare_Folder main.py
```

</details>

<a id="english"></a>

<details>
<summary><h2>🇬🇧 English (Click to expand / collapse)</h2></summary>

A smart, customizable, and slightly "annoying" timer to protect your eyes using the famous **20-20-20 rule**: *Every 20 minutes, look at something 20 feet away for 20 seconds.* Perfect for developers, gamers, and anyone who spends hours staring at a screen.

### New in this version ✨
- **Chinese font priority** – Automatically detects and prioritizes Chinese system fonts, no more garbled text.
- **Live font switching** – Change font in settings, updates main UI and popups instantly.
- **Enhanced CAPTCHA** – Enter a 4‑digit code **digit by digit**; wrong entry clears and prompts retry, **Backspace** supported, progress shown as dots (●/○).
- **Aggressive focus stealing** – Uses Windows API (`AttachThreadInput`, `SetForegroundWindow`, etc.) and repeats every 800ms to fight full‑screen games/apps.
- **Popup improvements** – Fixed black background + white text (always visible), press `ESC` to skip current cycle.
- **Start in tray** – App minimizes to system tray immediately on launch, double‑click to restore.
- **Sound on skip** – Clicking "Skip Cycle" also plays a random notification sound.
- **No repeated sounds** – Remembers last played sound to avoid consecutive duplicates; random volume 0.6～1.0.
- **Correct cycle counting** – Cycle counter increments only after successful code entry (if code required).

### Features
* **Custom Sounds:** 9 built‑in sounds included. On first run, a `sounds` folder is created next to the `.exe`. Drop your own `.mp3`/`.wav`/`.ogg` files – the app will play them randomly during breaks!
* **Strict Overlay Mode:** Full‑screen overlay forces you to look away. Enable the "Require Code" feature to type a random 4‑digit code after each break.
* **Global Hotkeys:** Pause or skip cycles from anywhere, even in full‑screen games (customizable).
* **System Tray:** Hides to tray on startup, keeps taskbar clean. Double‑click to open.
* **Smart Warnings:** Gentle notification 10 seconds before break, appears in your chosen screen corner.

### How to Use (Install and run)
1. Go to the **[Releases](../../releases)** tab on the right.
2. Download `EyeCare_OneFile.exe` (or any `.exe` name you prefer).
3. Put it in a folder of your choice and run it!
> **Note:** Since this is a custom‑built app, Windows SmartScreen might show a blue warning. Click **"More info"** -> **"Run anyway"**.  
> *If global hotkeys don't work in certain games, run the `.exe` as Administrator.*

### What You Can Customize
Click the **"Settings"** button in the app to change:
* **Focus & Rest Time:** Adjust work minutes and rest seconds.
* **Overlay & Code:** Turn off the overlay (background timer only) or enable the unlock code for maximum strictness.
* **Warning Corner:** Choose where the 10‑second pre‑break warning appears (Bottom Right, Top Left, etc.).
* **Hotkeys:** Click the button and press your preferred key combination.
* **Theme & Language:** Dark/Light mode and EN/RU/ZH localization.
* **Font:** Pick any font from your system list – live preview.

### Build Instructions (for developers)
Use the following commands to package the app yourself:

**PyInstaller single‑file**  
```bash
pyinstaller --onefile --windowed --name "EyeCare_OneFile" --icon="icon.ico" --add-data "icon.ico;." --add-data "icon.png;." --add-data "sounds;sounds" --hidden-import PIL._tkinter_finder --hidden-import customtkinter main.py
```

**Nuitka folder mode**  
```bash
nuitka --standalone --windows-disable-console --enable-plugin=tk-inter --output-filename=EyeCare_Folder.exe --windows-icon-from-ico=icon.ico --include-data-file=icon.ico=icon.ico --include-data-file=icon.png=icon.png --include-data-dir=sounds=sounds --include-module=PIL._tkinter_finder --include-module=customtkinter --output-dir=dist_folder\EyeCare_Folder main.py
```

</details>

<a id="russian"></a>
<details>
<summary><h2>🇷🇺 Русский (Нажми, чтобы развернуть / свернуть)</h2></summary>

Умный, настраиваемый и в меру бесячий таймер для защиты зрения по знаменитому правилу **20-20-20**: *Каждые 20 минут отрывайся от экрана и смотри вдаль на 20 футов (6 метров) в течение 20 секунд.* Идеально для программистов, геймеров и всех, кто живет за компом.

### Что нового ✨
- **Приоритет кириллических шрифтов** – автоматически определяет и ставит в начало списка шрифты, поддерживающие кириллицу.
- **Живая смена шрифта** – меняй шрифт в настройках, он сразу применяется ко всему интерфейсу и оверлею.
- **Умная капча** – 4 цифры вводятся **по одной**; ошибка мгновенно сбрасывает ввод и показывает подсказку, **Backspace** работает, прогресс отображается точками (●/○).
- **Агрессивный захват фокуса** – использует Windows API (`AttachThreadInput`, `SetForegroundWindow` и т.д.) и повторяет попытку каждые 800 мс, чтобы перебить полноэкранные игры/приложения.
- **Улучшенный оверлей** – фиксированный чёрный фон + белый текст (всегда видно), нажатие `ESC` пропускает текущий цикл.
- **Старт в трее** – программа сразу прячется в системный трей, двойной клик – развернуть.
- **Звук при пропуске** – кнопка «Скипнуть цикл» тоже играет случайный звук.
- **Неповторяющиеся звуки** – запоминается последний сыгранный звук, чтобы избежать повторов; громкость случайная 0.6～1.0.
- **Правильный счётчик циклов** – цикл засчитывается только после успешного ввода кода (если код включён).

### Главные фичи
* **Свои звуки:** В таймер уже вшито 9 звуков, но при первом запуске он сам создаст папку `sounds` рядом с `.exe`. Просто закинь туда любые свои `.mp3`, `.wav` или `.ogg` треки, и таймер будет рандомно их включать!
* **Бесячий оверлей:** Программа перекроет весь экран, чтобы ты точно дал глазам отдохнуть. Можно включить фичу "Требовать код" — тогда таймер не отпустит, пока не введешь 4 случайные цифры с экрана.
* **Глобальные хоткеи:** Ставь на паузу или скипай цикл откуда угодно (даже из полноэкранной игры).
* **Системный трей:** Таймер прячется рядом с часами и не мозолит глаза на панели задач. Двойной клик — развернуть.
* **Умные уведомления:** За 10 секунд до отдыха появляется плавное предупреждение (место его появления можно менять).

### Как использовать (Просто скачать и запустить)
1. Перейди во вкладку **[Releases](../../releases)** (Релизы) справа.
2. Скачай `EyeCare_OneFile.exe` (или любой другой `.exe`).
3. Положи его в любую удобную папку и запусти!
> **Важно:** Так как это самописная программа без цифровой подписи, синий экран Windows (SmartScreen) может выдать предупреждение. Просто нажми **«Подробнее»** -> **«Выполнить в любом случае»**.  
> *Если глобальные хоткеи не работают в некоторых играх — запусти `.exe` от имени Администратора.*

### Что можно менять внутри
Кнопка **"Настройки"** позволяет кастомизировать почти всё:
* **Время:** Настрой свои интервалы фокуса (в минутах) и отдыха (в секундах).
* **Жесткость:** Отключи оверлей, если нужен просто таймер в фоне, или вруби ввод кода для максимальной дисциплины.
* **Предупреждения:** Выбери, в каком углу будет появляться окно "Скоро отдых".
* **Хоткеи:** Нажми на кнопку настройки и введи любую свою комбинацию клавиш.
* **Внешний вид:** Темная/Светлая тема и переключение языков (RU/EN/ZH).
* **Шрифт:** Выбери любой шрифт из списка – изменение происходит сразу.

### Сборка из исходников (для разработчиков)

**PyInstaller (один файл)**  
```bash
pyinstaller --onefile --windowed --name "EyeCare_OneFile" --icon="icon.ico" --add-data "icon.ico;." --add-data "icon.png;." --add-data "sounds;sounds" --hidden-import PIL._tkinter_finder --hidden-import customtkinter main.py
```

**Nuitka (папка)**  
```bash
nuitka --standalone --windows-disable-console --enable-plugin=tk-inter --output-filename=EyeCare_Folder.exe --windows-icon-from-ico=icon.ico --include-data-file=icon.ico=icon.ico --include-data-file=icon.png=icon.png --include-data-dir=sounds=sounds --include-module=PIL._tkinter_finder --include-module=customtkinter --output-dir=dist_folder\EyeCare_Folder main.py
```

</details>

<img width="399" height="557" alt="Image" src="https://github.com/user-attachments/assets/8e22f82a-777c-4b8a-89ab-ccb28163f62b" /> <img width="385" height="693" alt="Image" src="https://github.com/user-attachments/assets/b37ab0f6-d8bd-4448-9e42-dd429d242954" />

