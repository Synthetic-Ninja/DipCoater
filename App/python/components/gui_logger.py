import datetime
from PyQt6 import QtWidgets


class GuiLogger:
    def __init__(self, text_browser: QtWidgets.QTextBrowser):
        self._log_ui = text_browser

    @staticmethod
    def __prepare_log(text: str, mode: str):
        styles = {
            'warning': '#ffa500',
            'error': '#ff0000',
            'success': '#a1e330',
            'info': '#FFFFFF',
            'debug': '#039dfc'
        }

        text_pattern = (
            f'<span style=\" color: {styles[mode]};\">'
            f'[{datetime.datetime.now()}]'
            f'[{mode.upper()}]: {text}</span>'
        )

        return text_pattern

    def print_log(self, text, mode):
        self._log_ui.append(self.__prepare_log(text=text, mode=mode))

    def warning(self, text):
        self.print_log(text, 'warning')

    def info(self, text):
        self.print_log(text, 'info')

    def error(self, text):
        self.print_log(text, 'error')

    def success(self, text):
        self.print_log(text, 'success')

    def debug(self, text):
        self.print_log(text, 'debug')
