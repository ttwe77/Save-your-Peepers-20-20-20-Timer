#This project is forked from Mythlon/Save-your-Peepers-20-20-20-Timer, which is licensed under the MIT License.
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
        "pos_tl": "Вверху слева"
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
        "pos_tl": "Top Left"},
    "zh": {
        "title": "守护双眼👁️",
        "focus": "专注时段",
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
        "pos_tl": "左上角"
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
        self.font_family = "Microsoft YaHei"
        self.total_cycles = 0

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
            except Exception as e:
                print(f"Ошибка при загрузке настроек: {e}")

        ctk.set_appearance_mode(self.theme)
        self.font_options = self.get_system_fonts()
        if self.font_family not in self.font_options:
            self.font_family = self.font_options[0] if self.font_options else "Helvetica"

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
            "total_cycles": self.total_cycles
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
        fonts = sorted(set(tkfont.families()))
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
        self.waiting_for_code = True
        self.show_unlock_code()

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
                    self.play_random_sound()
                    if self.use_overlay:
                        self.show_overlay()
                else:
                    self.add_successful_cycle()
                    self.hide_overlay()
                    self.is_working_phase = True
                    self.time_left = self.work_duration
                    self.play_random_sound()
                    self.update_phase_label()

            self.time_label.configure(text=self.format_time(self.time_left))

        self.after(1000, self.update_timer)

    def show_warning(self):
        if self.warning_window is None or not self.warning_window.winfo_exists():
            self.warning_window = ctk.CTkToplevel(self)
            self.warning_window.overrideredirect(True)
            self.warning_window.attributes("-topmost", True)
            self.warning_window.attributes("-alpha", 0.0)

            bg_color = "#333333" if ctk.get_appearance_mode() == "Dark" else "#EEEEEE"
            self.warning_window.configure(fg_color=bg_color)

            ww, wh = 160, 50
            sw, sh = self.winfo_screenwidth(), self.winfo_screenheight()

            padding_x = 20
            padding_y = 60

            if self.warn_position == "pos_br":
                x = sw - ww - padding_x;
                y = sh - wh - padding_y
            elif self.warn_position == "pos_bl":
                x = padding_x;
                y = sh - wh - padding_y
            elif self.warn_position == "pos_tr":
                x = sw - ww - padding_x;
                y = padding_x
            elif self.warn_position == "pos_tl":
                x = padding_x;
                y = padding_x
            else:
                x = sw - ww - padding_x;
                y = sh - wh - padding_y

            self.warning_window.geometry(f"{ww}x{wh}+{x}+{y}")

            lbl = ctk.CTkLabel(self.warning_window, text=self.t("warning_text"),
                               text_color="#FFB84D", font=self.get_font(14, "bold"))
            lbl.pack(expand=True, fill="both")

            self.fade_warning(0.0, target=0.9, step=0.05)

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
        if self.overlay_window is None or not self.overlay_window.winfo_exists():
            self.overlay_window = ctk.CTkToplevel(self)
            self.overlay_window.overrideredirect(True)
            self.overlay_window.attributes("-topmost", True)
            self.overlay_window.attributes("-alpha", 0.0)

            bg_color = "black" if ctk.get_appearance_mode() == "Light" else "white"
            text_color = "white" if ctk.get_appearance_mode() == "Light" else "black"
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

            self.fade_in_overlay(0.0)

        self.force_window_focus()
        self.start_focus_loop()

    def fade_in_overlay(self, current_alpha):
        if self.overlay_window and self.overlay_window.winfo_exists():
            new_alpha = current_alpha + 0.05
            if new_alpha <= 0.5:
                self.overlay_window.attributes("-alpha", new_alpha)
                self.after(30, self.fade_in_overlay, new_alpha)

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
        if self.overlay_window and self.overlay_window.winfo_exists():
            self.overlay_window.deiconify()
            self.overlay_window.lift()
            self.overlay_window.attributes('-topmost', True)
            self.overlay_window.focus_force()
            self.overlay_window.grab_set()

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
            self.force_window_focus()
            self.overlay_window.after(800, self._focus_loop)

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
        settings_win = ctk.CTkToplevel(self)
        settings_win.title(self.t("set_title"))
        settings_win.geometry("380x660")
        settings_win.attributes("-topmost", True)
        settings_win.grab_set()

        if hasattr(self, 'ico_path') and os.path.exists(self.ico_path):
            settings_win.after(200, lambda: settings_win.iconbitmap(self.ico_path))

        def live_theme_change(choice):
            ctk.set_appearance_mode(choice)

        def live_font_change(choice):
            self.font_family = choice
            self.update_ui_fonts()

        def live_lang_change(choice):
            if choice == "RU":
                self.lang = "ru"
            elif choice == "EN":
                self.lang = "en"
            elif choice == "ZH":
                self.lang = "zh"
            self.update_ui_texts()
            settings_win.title(self.t("set_title"))
            lbl_theme.configure(text=self.t("theme"))
            lbl_lang.configure(text=self.t("lang"))
            lbl_work.configure(text=self.t("set_work"))
            lbl_rest.configure(text=self.t("set_rest"))
            overlay_switch.configure(text=self.t("set_overlay"))
            code_switch.configure(text=self.t("set_code"))
            warning_switch.configure(text=self.t("set_warning"))
            lbl_warn_pos.configure(text=self.t("set_warn_pos"))

            pos_vals = [self.t("pos_br"), self.t("pos_bl"), self.t("pos_tr"), self.t("pos_tl")]
            pos_menu.configure(values=pos_vals)
            pos_menu.set(self.t(self.warn_position))

            lbl_hk_pause.configure(text=self.t("set_hk_pause"))
            lbl_hk_skip.configure(text=self.t("set_hk_skip"))
            btn_save.configure(text=self.t("set_save"))
            lbl_font.configure(text=self.t("font"))

            for btn in (self.btn_bind_pause, self.btn_bind_skip):
                if btn.cget("text") in [TRANSLATIONS["ru"]["set_press_hk"], TRANSLATIONS["en"]["set_press_hk"]]:
                    btn.configure(text=self.t("set_press_hk"))

        frame_top = ctk.CTkFrame(settings_win, fg_color="transparent")
        frame_top.pack(pady=10, fill="x", padx=20)

        lbl_theme = ctk.CTkLabel(frame_top, text=self.t("theme"))
        lbl_theme.pack(side="left")
        theme_menu = ctk.CTkOptionMenu(frame_top, values=["Dark", "Light"], width=90, command=live_theme_change)
        theme_menu.set(ctk.get_appearance_mode())
        theme_menu.pack(side="left", padx=(5, 10))

        lbl_lang = ctk.CTkLabel(frame_top, text=self.t("lang"))
        lbl_lang.pack(side="left")
        lang_menu = ctk.CTkOptionMenu(frame_top, values=["RU", "EN", "ZH"], width=70, command=live_lang_change)
        if self.lang == "ru":
            lang_menu.set("RU")
        elif self.lang == "en":
            lang_menu.set("EN")
        else:
            lang_menu.set("ZH")
        lang_menu.pack(side="left", padx=5)

        lbl_font = ctk.CTkLabel(settings_win, text=self.t("font"))
        lbl_font.pack(pady=(5, 0))
        font_menu = ctk.CTkOptionMenu(settings_win, values=self.font_options, width=300, command=live_font_change)
        font_menu.set(self.font_family if self.font_family in self.font_options else (self.font_options[0] if self.font_options else self.font_family))
        font_menu.pack()

        lbl_work = ctk.CTkLabel(settings_win, text=self.t("set_work"))
        lbl_work.pack(pady=(5, 0))
        work_entry = ctk.CTkEntry(settings_win, justify="center")
        work_entry.insert(0, str(self.work_duration // 60))
        work_entry.pack()

        lbl_rest = ctk.CTkLabel(settings_win, text=self.t("set_rest"))
        lbl_rest.pack(pady=(5, 0))
        rest_entry = ctk.CTkEntry(settings_win, justify="center")
        rest_entry.insert(0, str(self.rest_duration))
        rest_entry.pack()

        switch_frame = ctk.CTkFrame(settings_win, fg_color="transparent")
        switch_frame.pack(pady=10)

        overlay_switch = ctk.CTkSwitch(switch_frame, text=self.t("set_overlay"))
        overlay_switch.select() if self.use_overlay else overlay_switch.deselect()
        overlay_switch.pack(anchor="w", pady=5)

        code_switch = ctk.CTkSwitch(switch_frame, text=self.t("set_code"))
        code_switch.select() if self.require_code else code_switch.deselect()
        code_switch.pack(anchor="w", pady=5)

        warning_switch = ctk.CTkSwitch(switch_frame, text=self.t("set_warning"))
        warning_switch.select() if self.use_warning else warning_switch.deselect()
        warning_switch.pack(anchor="w", pady=5)

        pos_frame = ctk.CTkFrame(settings_win, fg_color="transparent")
        pos_frame.pack(pady=5)
        lbl_warn_pos = ctk.CTkLabel(pos_frame, text=self.t("set_warn_pos"))
        lbl_warn_pos.pack(side="left", padx=5)

        pos_values = [self.t("pos_br"), self.t("pos_bl"), self.t("pos_tr"), self.t("pos_tl")]
        pos_menu = ctk.CTkOptionMenu(pos_frame, values=pos_values, width=140)
        pos_menu.set(self.t(self.warn_position))
        pos_menu.pack(side="left")

        lbl_hk_pause = ctk.CTkLabel(settings_win, text=self.t("set_hk_pause"))
        lbl_hk_pause.pack(pady=(5, 0))
        self.btn_bind_pause = ctk.CTkButton(settings_win, text=self.hotkey_pause,
                                            command=lambda: self.record_hotkey(self.btn_bind_pause, 'hotkey_pause'))
        self.btn_bind_pause.pack()

        lbl_hk_skip = ctk.CTkLabel(settings_win, text=self.t("set_hk_skip"))
        lbl_hk_skip.pack(pady=(5, 0))
        self.btn_bind_skip = ctk.CTkButton(settings_win, text=self.hotkey_skip,
                                           command=lambda: self.record_hotkey(self.btn_bind_skip, 'hotkey_skip'))
        self.btn_bind_skip.pack()

        def save_settings():
            try:
                new_work = int(work_entry.get()) * 60
                new_rest = int(rest_entry.get())
            except ValueError:
                new_work = self.work_duration
                new_rest = self.rest_duration

            self.use_overlay = overlay_switch.get() == 1
            self.require_code = code_switch.get() == 1
            self.use_warning = warning_switch.get() == 1
            self.font_family = font_menu.get()

            chosen_pos_text = pos_menu.get()
            if chosen_pos_text in [TRANSLATIONS["ru"]["pos_br"], TRANSLATIONS["en"]["pos_br"]]:
                self.warn_position = "pos_br"
            elif chosen_pos_text in [TRANSLATIONS["ru"]["pos_bl"], TRANSLATIONS["en"]["pos_bl"]]:
                self.warn_position = "pos_bl"
            elif chosen_pos_text in [TRANSLATIONS["ru"]["pos_tr"], TRANSLATIONS["en"]["pos_tr"]]:
                self.warn_position = "pos_tr"
            elif chosen_pos_text in [TRANSLATIONS["ru"]["pos_tl"], TRANSLATIONS["en"]["pos_tl"]]:
                self.warn_position = "pos_tl"

            time_changed = (new_work != self.work_duration) or (new_rest != self.rest_duration)

            self.work_duration = new_work
            self.rest_duration = new_rest

            self.save_settings_to_file()
            self.register_hotkeys()

            if time_changed:
                self.skip_cycle()
            else:
                self.update_phase_label()

            settings_win.destroy()

        btn_save = ctk.CTkButton(settings_win, text=self.t("set_save"), command=save_settings, fg_color="green",
                                 hover_color="darkgreen")
        btn_save.pack(pady=15)

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

    def hide_window(self):
        self.withdraw()
        if not self.tray_icon:
            menu = pystray.Menu(
                pystray.MenuItem(self.t("tray_show"), self.show_window, default=True),
                pystray.MenuItem(self.t("tray_quit"), self.quit_app)
            )
            image = self.get_tray_icon()
            self.tray_icon = pystray.Icon("20-20-20", image, self.t("title"), menu)
            threading.Thread(target=self.tray_icon.run, daemon=True).start()

    def show_window(self, icon, item):
        icon.stop()
        self.tray_icon = None
        self.after(0, self.deiconify)

    def quit_app(self, icon, item):
        icon.stop()
        self.quit()
        os._exit(0)


if __name__ == "__main__":
    app = TimerApp()
    app.mainloop()