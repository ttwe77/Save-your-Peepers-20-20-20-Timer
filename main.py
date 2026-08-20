#This project is forked from Mythlon/Save-your-Peepers-20-20-20-Timer, which is licensed under the MIT License.(https://github.com/Mythlon/Save-your-Peepers-20-20-20-Timer)
import customtkinter as ctk
import keyboard
import pygame
import os
import sys
import random
import string
import threading
import time
import pystray
import json
import shutil
import tkinter.font as tkfont
from PIL import Image, ImageDraw, ImageTk



def get_base_path():
    """Возвращает путь к папке, где лежит сам .exe (для внешних звуков и настроек)"""
    if getattr(sys, 'frozen', False):
        return os.path.dirname(sys.executable)
    return os.path.dirname(os.path.abspath(__file__))


def resource_path(relative_path):
    """Возвращает путь к вшитым ресурсам внутри .exe (иконка и базовые звуки)"""
    try:
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")
    return os.path.join(base_path, relative_path)



TRANSLATIONS = {
    "ru": {
        "title": "Бережем глазки 👁️",
        "focus": "Фокус",
        "pause": "Пауза",
        "rest_overlay": "Смотри вдаль!",
        "rest_no_overlay": "Отдых (Без экрана)",
        "btn_pause": "Пауза",
        "btn_resume": "Возобновить",
        "btn_skip": "Скипнуть цикл",
        "btn_settings": "Настройки",
        "info_hotkeys": "Пауза: {pause} | Скип: {skip}",
        "cycles_count": "🔥 Завершено циклов: {count}",
        "overlay_main": "Оторвись от экрана!",
        "overlay_time": "Осталось: {time} сек.",
        "overlay_type_code": "Введи код на клавиатуре",
        "set_title": "Настройки",
        "set_work": "Фокус (минуты):",
        "set_rest": "Упражнение (секунды):",
        "set_overlay": "Бесячий оверлей",
        "set_code": "Требовать ввод кода",
        "set_warning": "Уведомление (за 10 сек)",
        "set_warn_pos": "Угол:",
        "set_hk_pause": "Хоткей Паузы:",
        "set_hk_skip": "Хоткей Скипа:",
        "set_save": "Сохранить",
        "set_press_hk": "Нажми комбинацию...",
        "theme": "Тема:",
        "lang": "Язык:",
        "font": "Шрифт:",
        "tray_show": "Развернуть",
        "tray_quit": "Закрыть полностью",
        "warning_text": "🤯 Скоро отдых!",
        "pos_br": "Внизу справа",
        "pos_bl": "Внизу слева",
        "pos_tr": "Вверху справа",
        "pos_tl": "Вверху слева",
        "pos_center": "По центру",
        "pos_top_center": "Вверху по центру",
        "pos_bottom_center": "Внизу по центру",
        "set_auto_start": "Автозапуск",
        "btn_rest_now": "Отдохнуть сейчас",
        "set_overlay_mode": "Режим оверлея:",
        "overlay_fullscreen": "Полноэкранный",
        "overlay_securedesktop": "Безопасный рабочий стол"
    },
    "en": {
        "title": "Save Your Peepers 👁️",
        "focus": "Focus",
        "pause": "Paused",
        "rest_overlay": "Look away!",
        "rest_no_overlay": "Rest (No screen)",
        "btn_pause": "Pause",
        "btn_resume": "Resume",
        "btn_skip": "Skip Cycle",
        "btn_settings": "Settings",
        "info_hotkeys": "Pause: {pause} | Skip: {skip}",
        "cycles_count": "🔥 Completed cycles: {count}",
        "overlay_main": "Look away from the screen!",
        "overlay_time": "{time} sec left",
        "overlay_type_code": "Type the code on keyboard",
        "set_title": "Settings",
        "set_work": "Focus (minutes):",
        "set_rest": "Exercise (seconds):",
        "set_overlay": "Annoying Overlay",
        "set_code": "Require Code",
        "set_warning": "Warning (10s before)",
        "set_warn_pos": "Corner:",
        "set_hk_pause": "Pause Hotkey:",
        "set_hk_skip": "Skip Hotkey:",
        "set_save": "Save",
        "set_press_hk": "Press combination...",
        "theme": "Theme:",
        "lang": "Language:",
        "font": "Font:",
        "tray_show": "Show",
        "tray_quit": "Quit",
        "warning_text": "👀 Rest soon!",
        "pos_br": "Bottom Right",
        "pos_bl": "Bottom Left",
        "pos_tr": "Top Right",
        "pos_tl": "Top Left",
        "pos_center": "Center",
        "pos_top_center": "Top Center",
        "pos_bottom_center": "Bottom Center",
        "set_auto_start": "Auto Start",
        "btn_rest_now": "Rest Now",
        "set_overlay_mode": "Overlay Mode:",
        "overlay_fullscreen": "Fullscreen",
        "overlay_securedesktop": "Secure Desktop"},
    "zh": {
        "title": "守护双眼👁️",
        "focus": "下次提醒",
        "pause": "已暂停",
        "rest_overlay": "远眺休息！",
        "rest_no_overlay": "休息（无弹窗）",
        "btn_pause": "暂停",
        "btn_resume": "恢复",
        "btn_skip": "跳过本轮",
        "btn_settings": "设置",
        "info_hotkeys": "暂停：{pause} | 跳过：{skip}\n强制全屏时可按ESC退出",
        "cycles_count": "🔥 已完成周期数：{count}",
        "overlay_main": "您已持续用眼过久\n休息一会吧！\n请将注意力集中在至少6米远的地方！",
        "overlay_time": "剩余：{time} 秒",
        "overlay_type_code": "请在键盘输入验证码",
        "set_title": "设置",
        "set_work": "专注时长（分钟）：",
        "set_rest": "休息时长（秒）：",
        "set_overlay": "强制提醒弹窗",
        "set_code": "需输入验证码",
        "set_warning": "休息提醒（提前10秒）",
        "set_warn_pos": "提醒位置：",
        "set_hk_pause": "暂停快捷键：",
        "set_hk_skip": "跳过快捷键：",
        "set_save": "保存",
        "set_press_hk": "按下快捷键组合...",
        "theme": "主题：",
        "lang": "语言：",
        "font": "字体：",
        "tray_show": "显示主界面",
        "tray_quit": "彻底退出",
        "warning_text": "即将休息",
        "pos_br": "右下角",
        "pos_bl": "左下角",
        "pos_tr": "右上角",
        "pos_tl": "左上角",
        "pos_center": "中间",
        "pos_top_center": "中上方",
        "pos_bottom_center": "中下方",
        "set_auto_start": "开机自启动",
        "btn_rest_now": "立即休息",
        "set_overlay_mode": "弹窗模式：",
        "overlay_fullscreen": "全屏弹窗",
        "overlay_securedesktop": "安全桌面"
    }
}

ctk.set_default_color_theme("blue")


class TimerApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.base_dir = get_base_path()
        self.settings_file = os.path.join(self.base_dir, "settings.json")
        self.sounds_folder = os.path.join(self.base_dir, "sounds")

        self.load_settings()

        self.title(self.t("title"))
        self.geometry("400x520")
        self.resizable(False, False)

        try:
            if os.name == 'nt':
                import ctypes
                myappid = 'my_perfect_timer.20_20_20.1_0'
                ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(myappid)

            icon_path = resource_path("icon.png")
            if os.path.exists(icon_path):
                img = Image.open(icon_path)
                self.ico_path = os.path.join(self.base_dir, "icon.ico")
                img.save(self.ico_path, format="ICO", sizes=[(64, 64)])
                self.iconbitmap(self.ico_path)
                self.app_icon = ImageTk.PhotoImage(img)
                self.iconphoto(False, self.app_icon)
        except Exception as e:
            print(f"Не удалось установить иконку окна: {e}")

        self.protocol("WM_DELETE_WINDOW", self.hide_window)

        pygame.mixer.init()

        if not os.path.exists(self.sounds_folder):
            os.makedirs(self.sounds_folder)


        internal_sounds_dir = resource_path("sounds")
        if os.path.exists(internal_sounds_dir) and internal_sounds_dir != self.sounds_folder:
            for file_name in os.listdir(internal_sounds_dir):
                if file_name.lower().endswith(('.mp3', '.wav', '.ogg')):
                    src_file = os.path.join(internal_sounds_dir, file_name)
                    dst_file = os.path.join(self.sounds_folder, file_name)
                    if not os.path.exists(dst_file):
                        shutil.copy2(src_file, dst_file)

        # Состояние
        self.is_paused = False
        self.is_working_phase = True
        self.time_left = self.work_duration

        self.overlay_window = None
        self.warning_window = None
        self.waiting_for_code = False
        self._focus_loop_active = False
        self.current_code = ""
        self.typed_code = ""
        self.tray_icon = None
        self.last_played_sound = None
        self._screen_dimension_method = "未初始化"  # 屏幕尺寸获取方法记录
        self._securedesktop_running = False
        
        # 创建托盘图标（程序启动时立即显示）
        self.create_tray_icon()

        # Цвета
        self.color_focus = ("#0059b3", "#4DA6FF")
        self.color_rest = ("#b30000", "#FF6666")
        self.color_pause = ("#b37700", "#FFB84D")

        # --- ИНТЕРФЕЙС ---
        self.phase_label = ctk.CTkLabel(self, text=self.t("focus"), font=self.get_font(20, "bold"),
                                        text_color=self.color_focus)
        self.phase_label.pack(pady=(20, 0))

        self.time_label = ctk.CTkLabel(self, text=self.format_time(self.time_left), font=self.get_font(56, "bold"))
        self.time_label.pack(pady=10)

        self.pause_button = ctk.CTkButton(self, text=self.t("btn_pause"), command=self.toggle_pause)
        self.pause_button.pack(pady=5)

        self.skip_button = ctk.CTkButton(self, text=self.t("btn_skip"), command=self.skip_cycle, fg_color="#8B0000",
                                         hover_color="#A52A2A")
        self.skip_button.pack(pady=5)

        self.rest_now_button = ctk.CTkButton(self, text=self.t("btn_rest_now"), command=self.rest_now,
                                             fg_color="#006400", hover_color="#008000")
        self.rest_now_button.pack(pady=5)

        self.settings_button = ctk.CTkButton(self, text=self.t("btn_settings"), command=self.open_settings,
                                             fg_color="#444444")
        self.settings_button.pack(pady=5)

        self.cycles_label = ctk.CTkLabel(self, text=self.t("cycles_count").format(count=self.total_cycles),
                                         font=self.get_font(14, "bold"), text_color="#FFB84D")
        self.cycles_label.pack(side="bottom", pady=(0, 15))

        self.info_label = ctk.CTkLabel(self, text=self.t("info_hotkeys").format(pause=self.hotkey_pause,
                                                                                skip=self.hotkey_skip),
                                       font=self.get_font(12), text_color="gray")
        self.info_label.pack(side="bottom", pady=(0, 5))
        self.update_ui_fonts()

        self.register_hotkeys()
        self.update_timer()

    def load_settings(self):
        self.lang = "zh"
        self.work_duration = 1200
        self.rest_duration = 20
        self.use_overlay = True
        self.require_code = True
        self.use_warning = True
        self.warn_position = "pos_br"
        self.hotkey_pause = 'ctrl+shift+p'
        self.hotkey_skip = 'ctrl+shift+s'
        self.theme = "Dark"
        self.font_family = "Microsoft YaHei UI"
        self.total_cycles = 0
        self.auto_start = False
        self.overlay_mode = "fullscreen"

        print(f"[DEBUG] 开始加载设置，默认 warn_position: {self.warn_position}")

        if os.path.exists(self.settings_file):
            try:
                with open(self.settings_file, "r", encoding="utf-8") as f:
                    data = json.load(f)
                    self.lang = data.get("lang", self.lang)
                    self.work_duration = data.get("work_duration", self.work_duration)
                    self.rest_duration = data.get("rest_duration", self.rest_duration)
                    self.use_overlay = data.get("use_overlay", self.use_overlay)
                    self.require_code = data.get("require_code", self.require_code)
                    self.use_warning = data.get("use_warning", self.use_warning)
                    self.warn_position = data.get("warn_position", self.warn_position)
                    self.hotkey_pause = data.get("hotkey_pause", self.hotkey_pause)
                    self.hotkey_skip = data.get("hotkey_skip", self.hotkey_skip)
                    self.theme = data.get("theme", self.theme)
                    self.font_family = data.get("font_name", self.font_family)
                    self.total_cycles = data.get("total_cycles", self.total_cycles)
                    self.auto_start = data.get("auto_start", self.auto_start)
                    self.overlay_mode = data.get("overlay_mode", self.overlay_mode)
                    
                    print(f"[DEBUG] 从设置文件加载的 warn_position: {self.warn_position}")
            except Exception as e:
                print(f"Ошибка при загрузке настроек: {e}")
        else:
            print(f"[DEBUG] 设置文件不存在，使用默认值: {self.warn_position}")

        ctk.set_appearance_mode(self.theme)
        self.font_options = self.get_system_fonts()
        if self.font_family not in self.font_options:
            self.font_family = self.font_options[0] if self.font_options else "Helvetica"

    def open_autostart_manager(self):
        import subprocess
        autostart_exe = os.path.join(self.base_dir, "AutostartManager.exe")
        if os.path.exists(autostart_exe):
            try:
                subprocess.Popen([autostart_exe], shell=True)
            except Exception as e:
                print(f"无法启动AutostartManager: {e}")

    def save_settings_to_file(self):
        data = {
            "lang": self.lang,
            "work_duration": self.work_duration,
            "rest_duration": self.rest_duration,
            "use_overlay": self.use_overlay,
            "require_code": self.require_code,
            "use_warning": self.use_warning,
            "warn_position": self.warn_position,
            "hotkey_pause": self.hotkey_pause,
            "hotkey_skip": self.hotkey_skip,
            "theme": ctk.get_appearance_mode(),
            "font_name": self.font_family,
            "total_cycles": self.total_cycles,
            "auto_start": self.auto_start,
            "overlay_mode": self.overlay_mode
        }
        try:
            with open(self.settings_file, "w", encoding="utf-8") as f:
                json.dump(data, f, ensure_ascii=False, indent=4)
        except Exception as e:
            print(f"Ошибка при сохранении: {e}")

    def update_cycles_ui(self):
        self.cycles_label.configure(text=self.t("cycles_count").format(count=self.total_cycles))

    def add_successful_cycle(self):
        self.total_cycles += 1
        self.update_cycles_ui()
        self.save_settings_to_file()

    def t(self, key):
        return TRANSLATIONS[self.lang][key]

    def get_system_fonts(self):
        # 隐藏带 @ 前缀的字体（这些是 Windows 内部竖排字体，普通场景不适用）
        fonts = [f for f in sorted(set(tkfont.families())) if not f.startswith('@')]
        preferred = [
            "Microsoft YaHei", "微软雅黑", "SimHei", "黑体", "KaiTi", "楷体", "宋体", "SimSun",
            "NSimSun", "Microsoft JhengHei", "华文细黑", "方正兰亭黑", "PingFang SC"
        ]

        def sort_key(name):
            lower = name.lower()
            priority = 0 if any(pref.lower() in lower for pref in preferred) or any("\u4e00" <= ch <= "\u9fff" for ch in name) else 1
            return (priority, lower)

        return sorted(fonts, key=sort_key)

    def get_font(self, size, weight="normal"):
        return (self.font_family, size, weight)

    def update_ui_fonts(self):
        try:
            self.phase_label.configure(font=self.get_font(20, "bold"))
            self.time_label.configure(font=self.get_font(56, "bold"))
            self.cycles_label.configure(font=self.get_font(14, "bold"))
            self.info_label.configure(font=self.get_font(12))
        except Exception:
            pass

        if self.overlay_window and self.overlay_window.winfo_exists():
            self.overlay_main_label.configure(font=self.get_font(60, "bold"))
            self.overlay_time_label.configure(font=self.get_font(40))

        if self.warning_window and self.warning_window.winfo_exists():
            for child in self.warning_window.winfo_children():
                if isinstance(child, ctk.CTkLabel):
                    child.configure(font=self.get_font(14, "bold"))

    def update_ui_texts(self):
        self.title(self.t("title"))
        self.pause_button.configure(text=self.t("btn_resume") if self.is_paused else self.t("btn_pause"))
        self.skip_button.configure(text=self.t("btn_skip"))
        self.rest_now_button.configure(text=self.t("btn_rest_now"))
        self.settings_button.configure(text=self.t("btn_settings"))
        self.info_label.configure(text=self.t("info_hotkeys").format(pause=self.hotkey_pause, skip=self.hotkey_skip))
        self.update_cycles_ui()
        self.update_phase_label()

    def register_hotkeys(self):
        keyboard.unhook_all()
        keyboard.add_hotkey(self.hotkey_pause, self.toggle_pause)
        keyboard.add_hotkey(self.hotkey_skip, self.skip_cycle)
        self.info_label.configure(text=self.t("info_hotkeys").format(pause=self.hotkey_pause, skip=self.hotkey_skip))

    def play_random_sound(self):
        sounds = [f for f in os.listdir(self.sounds_folder) if f.lower().endswith(('.mp3', '.wav', '.ogg'))]
        if not sounds: return
        if len(sounds) > 1 and self.last_played_sound in sounds:
            sounds.remove(self.last_played_sound)
        chosen_sound = random.choice(sounds)
        self.last_played_sound = chosen_sound
        sound_path = os.path.join(self.sounds_folder, chosen_sound)
        try:
            pygame.mixer.music.set_volume(random.uniform(0.6, 1.0))
            pygame.mixer.music.load(sound_path)
            pygame.mixer.music.play()
        except Exception as e:
            print(f"Ошибка аудио: {e}")

    def toggle_pause(self):
        self.is_paused = not self.is_paused
        if self.is_paused:
            self.pause_button.configure(text=self.t("btn_resume"))
            self.phase_label.configure(text=self.t("pause"), text_color=self.color_pause)
        else:
            self.pause_button.configure(text=self.t("btn_pause"))
            self.update_phase_label()

    def skip_cycle(self):
        self.play_random_sound()
        # 安全桌面模式下，验证码由 securedesktop.exe 处理，无需 Python 层验证
        if self.require_code and self.overlay_mode != "securedesktop":
            self.waiting_for_code = True
            self.show_unlock_code()
        else:
            # 安全桌面模式下如果 securedesktop.exe 正在运行，结束它
            if self.overlay_mode == "securedesktop" and self._securedesktop_running:
                self._securedesktop_running = False
            self.hide_overlay()
            self.is_working_phase = True
            self.time_left = self.work_duration
            self.update_phase_label()

    def rest_now(self):
        """立即休息"""
        if self.is_paused or not self.is_working_phase:
            return
        self.hide_warning(instant=False)
        self.is_working_phase = False
        self.time_left = self.rest_duration
        self.update_phase_label()
        if self.overlay_mode != "securedesktop":
            self.play_random_sound()
        if self.use_overlay:
            self.show_overlay()

    def update_phase_label(self):
        if self.is_paused: return
        if self.is_working_phase:
            self.phase_label.configure(text=self.t("focus"), text_color=self.color_focus)
        else:
            txt = self.t("rest_overlay") if self.use_overlay else self.t("rest_no_overlay")
            self.phase_label.configure(text=txt, text_color=self.color_rest)

    def format_time(self, seconds):
        mins, secs = divmod(seconds, 60)
        return f"{mins:02d}:{secs:02d}"

    def update_timer(self):
        if not self.is_paused and not self.waiting_for_code:
            if self.time_left > 0:
                self.time_left -= 1

                if self.is_working_phase and self.use_warning and self.time_left == 10:
                    self.play_random_sound()
                    self.show_warning()

                if not self.is_working_phase and self.overlay_window:
                    self.overlay_time_label.configure(text=self.t("overlay_time").format(time=self.time_left))
            else:
                self.hide_warning(instant=False)

                if self.is_working_phase:
                    self.is_working_phase = False
                    self.time_left = self.rest_duration
                    self.update_phase_label()
                    if self.overlay_mode != "securedesktop":
                        self.play_random_sound()
                    if self.use_overlay:
                        self.show_overlay()
                else:
                    # 安全桌面模式下，由 securedesktop.exe 控制结束时机
                    if self.overlay_mode == "securedesktop" and self._securedesktop_running:
                        pass
                    else:
                        self.add_successful_cycle()
                        self.hide_overlay()
                        self.is_working_phase = True
                        self.time_left = self.work_duration
                        if self.overlay_mode != "securedesktop":
                            self.play_random_sound()
                        self.update_phase_label()

            self.time_label.configure(text=self.format_time(self.time_left))

        self.after(1000, self.update_timer)

    def show_warning(self):
        if self.warning_window is None or not self.warning_window.winfo_exists():
            # 调试日志：开始创建警告窗口
            print(f"[DEBUG] 开始创建警告窗口，位置配置: {self.warn_position}")
            
            self.warning_window = ctk.CTkToplevel(self)
            self.warning_window.overrideredirect(True)
            self.warning_window.attributes("-topmost", True)
            self.warning_window.attributes("-alpha", 0.0)

            bg_color = "#333333" if ctk.get_appearance_mode() == "Dark" else "#EEEEEE"
            self.warning_window.configure(fg_color=bg_color)

            # 创建标签（必须先创建，才能获取实际尺寸）
            lbl = ctk.CTkLabel(self.warning_window, text=self.t("warning_text"),
                               text_color="#FFB84D", font=self.get_font(14, "bold"))
            lbl.pack(expand=True, fill="both")

            # 强制更新布局，获取窗口实际宽高 - 使用更可靠的方法
            self.warning_window.update_idletasks()
            
            # 方法1: 使用winfo_width/height
            actual_width1 = self.warning_window.winfo_width()
            actual_height1 = self.warning_window.winfo_height()
            
            # 方法2: 使用winfo_reqwidth/reqheight（请求的尺寸）
            actual_width2 = self.warning_window.winfo_reqwidth()
            actual_height2 = self.warning_window.winfo_reqheight()
            
            # 方法3: 使用几何信息获取
            geometry = self.warning_window.geometry()
            geometry_parts = geometry.split('+')
            if len(geometry_parts) >= 1:
                size_part = geometry_parts[0]
                if 'x' in size_part:
                    actual_width3, actual_height3 = map(int, size_part.split('x'))
                else:
                    actual_width3, actual_height3 = actual_width1, actual_height1
            else:
                actual_width3, actual_height3 = actual_width1, actual_height1
            
            # 选择最大的尺寸作为实际尺寸（避免窗口太小）
            actual_width = max(actual_width1, actual_width2, actual_width3, 150)  # 最小150像素
            actual_height = max(actual_height1, actual_height2, actual_height3, 60)  # 最小60像素
            
            print(f"[DEBUG] 窗口尺寸获取方法: winfo={actual_width1}x{actual_height1}, req={actual_width2}x{actual_height2}, geometry={actual_width3}x{actual_height3}")
            print(f"[DEBUG] 最终窗口尺寸: {actual_width}x{actual_height}")

            # 获取屏幕尺寸 - 使用更可靠的方法
            sw, sh = self.get_screen_dimensions()
            padding_x = 60
            padding_y = 60

            # 调试日志：显示获取的尺寸信息
            print(f"[DEBUG] 屏幕尺寸: {sw}x{sh}, 窗口尺寸: {actual_width}x{actual_height}")
            print(f"[DEBUG] 使用屏幕尺寸获取方法: {self._screen_dimension_method}")

            # 根据用户设置的位置计算坐标（使用实际窗口尺寸）
            if self.warn_position == "pos_br":
                x = sw - actual_width - padding_x
                y = sh - actual_height - padding_y
                print(f"[DEBUG] 右下角位置: x={x}, y={y}")
            elif self.warn_position == "pos_bl":
                x = padding_x
                y = sh - actual_height - padding_y
                print(f"[DEBUG] 左下角位置: x={x}, y={y}")
            elif self.warn_position == "pos_tr":
                x = sw - actual_width - padding_x
                y = padding_y
                print(f"[DEBUG] 右上角位置: x={x}, y={y}")
            elif self.warn_position == "pos_tl":
                x = padding_x
                y = padding_y
                print(f"[DEBUG] 左上角位置: x={x}, y={y}")
            elif self.warn_position == "pos_center":
                x = (sw - actual_width) // 2
                y = (sh - actual_height) // 2
                print(f"[DEBUG] 中间位置: x={x}, y={y}")
            elif self.warn_position == "pos_top_center":
                x = (sw - actual_width) // 2
                y = padding_y
                print(f"[DEBUG] 上中位置: x={x}, y={y}")
            elif self.warn_position == "pos_bottom_center":
                x = (sw - actual_width) // 2
                y = sh - actual_height - padding_y
                print(f"[DEBUG] 下中位置: x={x}, y={y}")
            else:
                # 默认右下角
                x = sw - actual_width - padding_x
                y = sh - actual_height - padding_y
                print(f"[DEBUG] 默认右下角位置: x={x}, y={y}")

            # 仅移动窗口到正确位置（保持窗口实际大小不变）
            position_str = f"+{x}+{y}"
            print(f"[DEBUG] 设置窗口位置: {position_str}")
            self.warning_window.geometry(position_str)

            # 淡入显示
            self.fade_warning(0.0, target=0.9, step=0.05)
            
            # 最终验证位置
            self.warning_window.after(100, self._verify_warning_position)

    def get_screen_dimensions(self):
        """获取屏幕尺寸的多种方法，选择最可靠的一个"""
        methods = []
        
        # 方法1: 使用主窗口获取屏幕尺寸
        try:
            sw1 = self.winfo_screenwidth()
            sh1 = self.winfo_screenheight()
            methods.append(("主窗口", sw1, sh1))
        except:
            pass
            
        # 方法2: 使用警告窗口获取屏幕尺寸（原来的方法）
        try:
            if self.warning_window and self.warning_window.winfo_exists():
                sw2 = self.warning_window.winfo_screenwidth()
                sh2 = self.warning_window.winfo_screenheight()
                methods.append(("警告窗口", sw2, sh2))
        except:
            pass
            
        # 方法3: 使用系统API获取屏幕尺寸（最可靠）
        try:
            if os.name == 'nt':
                import ctypes
                user32 = ctypes.windll.user32
                sw3 = user32.GetSystemMetrics(0)  # SM_CXSCREEN
                sh3 = user32.GetSystemMetrics(1)  # SM_CYSCREEN
                methods.append(("系统API", sw3, sh3))
        except:
            pass
            
        # 方法4: 使用tkinter的Tk()根窗口获取屏幕尺寸
        try:
            import tkinter as tk
            root = tk.Tk()
            root.withdraw()  # 隐藏窗口
            sw4 = root.winfo_screenwidth()
            sh4 = root.winfo_screenheight()
            root.destroy()
            methods.append(("tk根窗口", sw4, sh4))
        except:
            pass
            
        # 选择最合理的尺寸（通常最大的尺寸是正确的）
        if methods:
            # 按面积排序，选择最大的
            methods.sort(key=lambda x: x[1] * x[2], reverse=True)
            best_method, best_sw, best_sh = methods[0]
            
            # 记录使用的方法
            self._screen_dimension_method = best_method
            
            print(f"[DEBUG] 可用屏幕尺寸方法: {methods}")
            print(f"[DEBUG] 选择的方法: {best_method}, 尺寸: {best_sw}x{best_sh}")
            
            return best_sw, best_sh
        else:
            # 回退到默认方法
            self._screen_dimension_method = "默认"
            return 1920, 1080  # 常见默认分辨率

    def _verify_warning_position(self):
        """验证警告窗口的实际位置"""
        if self.warning_window and self.warning_window.winfo_exists():
            actual_x = self.warning_window.winfo_x()
            actual_y = self.warning_window.winfo_y()
            actual_w = self.warning_window.winfo_width()
            actual_h = self.warning_window.winfo_height()
            print(f"[DEBUG] 实际窗口位置: x={actual_x}, y={actual_y}, 尺寸: {actual_w}x{actual_h}")

    def fade_warning(self, current_alpha, target, step):
        if self.warning_window and self.warning_window.winfo_exists():
            new_alpha = current_alpha + step

            if (step > 0 and new_alpha <= target) or (step < 0 and new_alpha >= target):
                self.warning_window.attributes("-alpha", new_alpha)
                self.after(30, self.fade_warning, new_alpha, target, step)
            else:
                self.warning_window.attributes("-alpha", target)
                if target <= 0.0:
                    self.warning_window.destroy()
                    self.warning_window = None

    def hide_warning(self, instant=False):
        if self.warning_window and self.warning_window.winfo_exists():
            if instant:
                self.warning_window.destroy()
                self.warning_window = None
            else:
                current_alpha = self.warning_window.attributes("-alpha")
                self.fade_warning(current_alpha, target=0.0, step=-0.05)

    def show_overlay(self):
        if self.overlay_mode == "securedesktop":
            self._show_securedesktop()
            return
        self._show_fullscreen_overlay()

    def _show_fullscreen_overlay(self):
        if self.overlay_window is None or not self.overlay_window.winfo_exists():
            self.overlay_window = ctk.CTkToplevel(self)
            self.overlay_window.overrideredirect(True)
            self.overlay_window.attributes("-topmost", True)
            self.overlay_window.attributes("-alpha", 0.0)

            # CAPTCHA overlay should always have dark background with light text for visibility
            bg_color = "black"
            text_color = "white"
            self.overlay_window.configure(fg_color=bg_color)

            w, h = self.winfo_screenwidth(), self.winfo_screenheight()
            self.overlay_window.geometry(f"{w}x{h}+0+0")

            self.overlay_main_label = ctk.CTkLabel(self.overlay_window, text=self.t("overlay_main"),
                                                   font=self.get_font(60, "bold"), text_color=text_color)
            self.overlay_main_label.pack(expand=True)

            self.overlay_time_label = ctk.CTkLabel(self.overlay_window,
                                                   text=self.t("overlay_time").format(time=self.time_left),
                                                   font=self.get_font(40), text_color=text_color)
            self.overlay_time_label.pack(pady=50)
            self.overlay_window.bind('<Escape>', lambda e: self.skip_cycle())
            # Alt+F4 绑定到与 ESC 相同的逻辑（修复全屏时按 Alt+F4 报错的问题）
            self.overlay_window.bind('<Alt-F4>', lambda e: self.skip_cycle())
            try:
                # 拦截窗口关闭消息（Alt+F4 触发），重定向到 skip_cycle
                self.overlay_window.protocol("WM_DELETE_WINDOW", self.skip_cycle)
            except Exception:
                pass
            # 监听窗口可见性/焦点变化，睡眠唤醒或显示器切换时重新校准全屏居中
            self.overlay_window.bind('<Visibility>', self._refocus_overlay_content)
            self.overlay_window.bind('<FocusIn>', self._refocus_overlay_content)
            self.overlay_window.bind('<Map>', self._refocus_overlay_content)

            self.fade_in_overlay(0.0)

        self.force_window_focus()
        self.start_focus_loop()

    def _show_securedesktop(self):
        """在安全桌面模式启动休息，运行 securedesktop.exe"""
        self._securedesktop_running = True

        def run_securedesktop():
            exe_path = os.path.join(self.base_dir, "securedesktop.exe")
            if not os.path.exists(exe_path):
                print("[WARNING] securedesktop.exe 未找到，回退到全屏弹窗")
                self.after(0, self._show_fullscreen_overlay)
                return

            self.save_settings_to_file()
            import subprocess
            try:
                subprocess.run([exe_path], shell=True)
            except Exception as e:
                print(f"运行 securedesktop.exe 失败: {e}")
                self.after(0, self._show_fullscreen_overlay)
                return

            self.after(0, self._on_securedesktop_finished)

        threading.Thread(target=run_securedesktop, daemon=True).start()

    def _on_securedesktop_finished(self):
        """securedesktop.exe 退出时调用，切换到工作阶段"""
        self._securedesktop_running = False
        if not self.is_working_phase:
            self.add_successful_cycle()
            self.is_working_phase = True
            self.time_left = self.work_duration
            if self.overlay_mode != "securedesktop":
                self.play_random_sound()
            self.update_phase_label()
            self.time_label.configure(text=self.format_time(self.time_left))

    def fade_in_overlay(self, current_alpha):
        if not (self.overlay_window and self.overlay_window.winfo_exists()):
            return
        try:
            new_alpha = current_alpha + 0.05
            if new_alpha <= 0.5:
                self.overlay_window.attributes("-alpha", new_alpha)
                self.after(30, self.fade_in_overlay, new_alpha)
        except Exception:
            # 窗口在淡入过程中被销毁时安全退出
            pass

    def show_unlock_code(self):
        self.current_code = ''.join(random.choices(string.digits, k=4))
        self.typed_code = ""
        
        if self.overlay_window is None or not self.overlay_window.winfo_exists():
            self.show_overlay()
            self.overlay_window.attributes("-alpha", 0.5)
        else:
            self.start_focus_loop()
        
        self.overlay_main_label.configure(text=f"{self.current_code}", font=self.get_font(100, "bold"), text_color="white")
        self.overlay_time_label.configure(text=self.t("overlay_type_code"), font=self.get_font(30))
        self.overlay_window.bind('<Key>', self.handle_key_press)
        
        self.force_window_focus()
    
    def force_window_focus(self):
        """强制窗口聚焦到前台（包括从全屏应用抢焦点）"""
        if not (self.overlay_window and self.overlay_window.winfo_exists()):
            return
        try:
            self.overlay_window.deiconify()
            self.overlay_window.lift()
            self.overlay_window.attributes('-topmost', True)
            self.overlay_window.focus_force()
            self.overlay_window.grab_set()
        except Exception:
            # 窗口在并发场景下被销毁，直接返回
            return

        try:
            if os.name == 'nt':
                import ctypes
                from ctypes import wintypes

                hwnd = self.overlay_window.winfo_id()

                ctypes.windll.user32.AllowSetForegroundWindow(wintypes.DWORD(-1))

                foreground_hwnd = ctypes.windll.user32.GetForegroundWindow()
                current_thread = ctypes.windll.kernel32.GetCurrentThreadId()
                foreground_thread = ctypes.windll.user32.GetWindowThreadProcessId(
                    foreground_hwnd, None
                )

                ctypes.windll.user32.AttachThreadInput(
                    current_thread, foreground_thread, True
                )

                ctypes.windll.user32.BringWindowToTop(hwnd)
                ctypes.windll.user32.SetForegroundWindow(hwnd)
                ctypes.windll.user32.SetActiveWindow(hwnd)
                ctypes.windll.user32.SetWindowPos(
                    hwnd, -1, 0, 0, 0, 0, 0x0002 | 0x0001
                )
                ctypes.windll.user32.ShowWindow(hwnd, 3)
                ctypes.windll.user32.SwitchToThisWindow(hwnd, True)

                ctypes.windll.user32.AttachThreadInput(
                    current_thread, foreground_thread, False
                )
        except Exception:
            pass

    def start_focus_loop(self):
        """在弹窗显示期间持续抢焦点（应对全屏应用）"""
        self._focus_loop_active = True
        self._focus_loop()

    def _focus_loop(self):
        if self._focus_loop_active and self.overlay_window and self.overlay_window.winfo_exists():
            try:
                self.force_window_focus()
                # 定期检查屏幕尺寸，关闭显示器/睡眠唤醒后自动重置全屏并重新居中
                self._ensure_overlay_fullscreen()
                self.overlay_window.after(800, self._focus_loop)
            except Exception as e:
                print(f"Focus loop error: {e}")
                self.stop_focus_loop()

    def _ensure_overlay_fullscreen(self):
        """确保遮罩层覆盖整个屏幕并让内容重新居中（修复关闭显示器/睡眠唤醒后文字未居中）"""
        if not (self.overlay_window and self.overlay_window.winfo_exists()):
            return
        try:
            sw = self.overlay_window.winfo_screenwidth()
            sh = self.overlay_window.winfo_screenheight()
            # 屏幕尺寸异常（显示器关闭/睡眠中）时跳过
            if sw <= 0 or sh <= 0:
                return
            current_w = self.overlay_window.winfo_width()
            current_h = self.overlay_window.winfo_height()
            cur_x = self.overlay_window.winfo_x()
            cur_y = self.overlay_window.winfo_y()
            # 当窗口尺寸或位置与屏幕不一致时（例如睡眠唤醒后多屏配置变化），强制重置
            if current_w != sw or current_h != sh or cur_x != 0 or cur_y != 0:
                self.overlay_window.geometry(f"{sw}x{sh}+0+0")
                # 强制 pack 重新计算居中
                self.overlay_window.update_idletasks()
        except Exception:
            pass

    def _refocus_overlay_content(self, event=None):
        """窗口获得焦点/可见性变化时，重新校准全屏尺寸以保证文字居中"""
        if self.overlay_window and self.overlay_window.winfo_exists():
            try:
                self._ensure_overlay_fullscreen()
            except Exception:
                pass

    def stop_focus_loop(self):
        self._focus_loop_active = False

    def handle_key_press(self, event):
        char = event.char
        if char.isdigit():
            # 限制输入长度，防止无限输入
            if len(self.typed_code) < len(self.current_code):
                # 立即检查当前输入是否正确
                expected_char = self.current_code[len(self.typed_code)]
                if char == expected_char:
                    # 输入正确，继续
                    self.typed_code += char
                    
                    # 显示输入进度
                    progress_text = "●" * len(self.typed_code) + "○" * (len(self.current_code) - len(self.typed_code))
                    self.overlay_time_label.configure(text=f"{self.t('overlay_type_code')}\n{progress_text}", font=self.get_font(30))
                    
                    # 检查是否输入完整
                    if len(self.typed_code) == len(self.current_code):
                        # 验证码正确，关闭窗口
                        self.hide_overlay()
                        self.is_working_phase = True
                        self.time_left = self.work_duration
                        self.waiting_for_code = False
                        self.update_phase_label()
                        self.time_label.configure(text=self.format_time(self.time_left))
                else:
                    # 输入错误，立即报错
                    self.typed_code = ""
                    self.overlay_main_label.configure(text=self.current_code, font=self.get_font(100, "bold"))
                    self.overlay_time_label.configure(text=f"{self.t('overlay_type_code')}\n输入错误！请重新开始输入", font=self.get_font(30))
                    # 重新聚焦窗口
                    self.force_window_focus()
        elif event.keysym == 'BackSpace' and len(self.typed_code) > 0:
            # 支持退格键删除
            self.typed_code = self.typed_code[:-1]
            
            # 更新显示
            display_text = self.typed_code + "●" * (len(self.current_code) - len(self.typed_code))
            self.overlay_main_label.configure(text=display_text, font=self.get_font(100, "bold"))
            
            # 更新进度显示
            progress_text = "●" * len(self.typed_code) + "○" * (len(self.current_code) - len(self.typed_code))
            self.overlay_time_label.configure(text=f"{self.t('overlay_type_code')}\n{progress_text}", font=self.get_font(30))

    def hide_overlay(self):
        self.stop_focus_loop()
        if self.overlay_window and self.overlay_window.winfo_exists():
            self.overlay_window.destroy()
            self.overlay_window = None

    def open_settings(self):
        import subprocess
        import os
        from tkinter import messagebox

        settings_exe = os.path.join(self.base_dir, "Settings.exe")
        if os.path.exists(settings_exe):
            try:
                subprocess.Popen([settings_exe], shell=True)
            except Exception as e:
                messagebox.showerror("错误", f"无法启动设置程序：{e}")
        else:
            messagebox.showerror("错误", "未找到 Settings.exe，请确保该文件位于程序目录下。")

    def record_hotkey(self, button, attr_name):
        button.configure(text=self.t("set_press_hk"))

        def listen():
            time.sleep(0.2)
            hk = keyboard.read_hotkey(suppress=False)
            self.after(0, lambda: self.finish_record(button, attr_name, hk))

        threading.Thread(target=listen, daemon=True).start()

    def finish_record(self, button, attr_name, hotkey):
        setattr(self, attr_name, hotkey)
        button.configure(text=hotkey)

    def get_tray_icon(self):
        icon_path = resource_path("icon.png")
        if os.path.exists(icon_path):
            try:
                return Image.open(icon_path)
            except Exception as e:
                print(f"Не удалось загрузить иконку {icon_path}: {e}")

        image = Image.new('RGB', (64, 64), color=(0, 89, 179))
        draw = ImageDraw.Draw(image)
        draw.rectangle((16, 16, 48, 48), fill="white")
        return image

    def create_tray_icon(self):
        """创建托盘图标（程序启动时立即显示）"""
        if not self.tray_icon:
            menu = pystray.Menu(
                pystray.MenuItem(self.t("tray_show"), self.show_window, default=True),
                pystray.MenuItem(self.t("tray_quit"), self.quit_app)
            )
            image = self.get_tray_icon()
            self.tray_icon = pystray.Icon("20-20-20", image, self.t("title"), menu)
            threading.Thread(target=self.tray_icon.run, daemon=True).start()

    def hide_window(self):
        self.withdraw()
        # 确保托盘图标存在（如果尚未创建）
        if not self.tray_icon:
            self.create_tray_icon()

    def show_window(self, icon, item):
        # 不停止托盘图标，只显示窗口
        self.after(0, self.deiconify)

    def quit_app(self, icon, item):
        icon.stop()
        self.quit()
        os._exit(0)


if __name__ == "__main__":
    app = TimerApp()
    app.mainloop()