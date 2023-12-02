import time

import serial
from PyQt6.QtCore import QThread
from PyQt6.QtCore import pyqtSignal
from PyQt6.QtCore import pyqtSlot

from python.components.gui_logger import GuiLogger


class Settings:
    steps_per_mm = 100
    max_steps_count = 5600
    driver_steps_division = 8
    invert_direction = 1
    invert_enable = 1
    debug = 0

    def __init__(
            self,
            steps_per_mm: int,
            max_steps_count: int,
            driver_steps_division: int,
            invert_direction: int,
            invert_enable: int,
            debug: int,
    ):
        self.steps_per_mm = steps_per_mm
        self.max_steps_count = max_steps_count
        self.driver_steps_division = driver_steps_division
        self.invert_direction = invert_direction
        self.invert_enable = invert_enable
        self.debug = debug

    def __bytes__(self):
        setting_on_bytes = bytes()
        setting_on_bytes += self.steps_per_mm.to_bytes(
            4,
            'little',
            signed=False,
        )
        setting_on_bytes += self.max_steps_count.to_bytes(
            4,
            'little',
            signed=False,
        )
        setting_on_bytes += self.driver_steps_division.to_bytes(
            1,
            'little',
            signed=False,
        )
        setting_on_bytes += self.invert_direction.to_bytes(
            1,
            'little',
            signed=False,
        )
        setting_on_bytes += self.invert_enable.to_bytes(
            1,
            'little',
            signed=False,
        )
        setting_on_bytes += self.debug.to_bytes(
            1,
            'little',
            signed=False,
        )

        return setting_on_bytes


class BaseUartThread(QThread):
    send_warning = pyqtSignal(str)
    send_info = pyqtSignal(str)
    send_error = pyqtSignal(str)
    send_debug = pyqtSignal(str)
    send_success = pyqtSignal(str)


class ConnectorUartThread(BaseUartThread):
    disconnect_signal = pyqtSignal()
    progress_bar_change = pyqtSignal(int)
    progress_bar_change_visible = pyqtSignal(bool)
    update_connection = pyqtSignal(bool)

    def __init__(self):
        super().__init__()
        self.serial_port = None
        self.disconnect_param = False

    def run(self) -> None:
        self.progress_bar_change.emit(0)
        self.progress_bar_change_visible.emit(True)
        try:
            self.serial_port.close()
            self.serial_port.open()
        except serial.SerialException as exc:
            self.send_error.emit(str(exc))
            if self.serial_port:
                self.serial_port.close()
            self.send_error.emit("Ошибка открытия serial")
            self.disconnect_signal.emit()
            self.progress_bar_change_visible.emit(False)
            return
        self.send_info.emit('Serial открыт...')
        self.serial_port.reset_input_buffer()
        self.send_info.emit('Ожидание устройства...')
        self.progress_bar_change.emit(25)
        while True:
            if self.disconnect_param:
                self.progress_bar_change_visible.emit(False)
                return
            try:
                if self.serial_port.in_waiting > 0:
                    raw_byte = self.serial_port.read()
                    if raw_byte == bytes.fromhex('F5'):
                        break
            except OSError:
                self.send_error.emit("Serial был отключен")
                self.progress_bar_change_visible.emit(False)
                self.disconnect_signal.emit()
                return

        self.send_info.emit('Отправка SYN')
        self.progress_bar_change.emit(50)
        self.serial_port.reset_input_buffer()
        self.serial_port.write(bytes.fromhex('F0'))
        self.send_info.emit('Ожидание ACK байта от устройства')
        while True:
            if self.disconnect_param:
                self.progress_bar_change_visible.emit(False)
                return
            try:
                if self.serial_port.in_waiting > 0:
                    raw_byte = self.serial_port.read()
                    if raw_byte == bytes.fromhex('F1'):
                        self.send_info.emit('ACK байт получен')
                        self.progress_bar_change.emit(75)
                        self.serial_port.write(bytes.fromhex('F2'))
                        self.progress_bar_change.emit(100)
                        self.send_success.emit('Соединение установлено')
                        self.progress_bar_change_visible.emit(False)
                        self.update_connection.emit(True)
                        return
                    else:
                        self.send_error.emit('Невалидный ACK байт')
                        self.progress_bar_change_visible.emit(False)
                        self.disconnect_signal.emit()
            except OSError:
                self.send_error.emit("Serial был отключен")
                self.progress_bar_change_visible.emit(False)
                self.disconnect_signal.emit()
                return

    def set_serial_port(self, serial_port: serial.Serial) -> None:
        self.serial_port = serial_port

    @pyqtSlot()
    def make_disconnect(self) -> None:
        self.disconnect_param = True

    def make_connect(self) -> None:
        self.disconnect_param = False

    def is_disconnected(self) -> bool:
        return self.disconnect_param


class SettingsWorkerThread(BaseUartThread):
    def __init__(self):
        super().__init__()

    def run(self):
        ...


class UartException(Exception):
    ...


class UartConnector:
    def __init__(self,
                 logger: GuiLogger,
                 disconnect_callback: callable,
                 progress_bar):
        self.logger = logger
        self.serial_port = None
        self.connection = False
        self.progress_bar = progress_bar

        self.disconnect_callback = disconnect_callback
        self.connector_thread = ConnectorUartThread()
        self.connector_thread.send_info.connect(logger.info)
        self.connector_thread.send_debug.connect(logger.debug)
        self.connector_thread.send_success.connect(logger.success)
        self.connector_thread.send_warning.connect(logger.warning)
        self.connector_thread.send_error.connect(logger.error)
        self.connector_thread.disconnect_signal.connect(disconnect_callback)
        self.connector_thread.progress_bar_change.connect(
            self.progress_bar.setValue,
        )
        self.connector_thread.progress_bar_change_visible.connect(
            self.progress_bar.setVisible,
        )
        self.connector_thread.update_connection.connect(
            self.set_connection,
        )

    def make_connection(self, path_to_serial_file: str) -> None:
        self.logger.info('Opening serial as 115200 baud')
        self.progress_bar.setVisible(True)
        try:
            self.serial_port = serial.Serial(
                path_to_serial_file,
                115200
            )
        except serial.SerialException as exc:
            self.disconnect_callback()
            self.logger.error(str(exc))
            return

        if not self.serial_port:
            return

        if self.connector_thread.is_disconnected():
            self.connector_thread.make_connect()

        self.connector_thread.set_serial_port(self.serial_port)
        self.connector_thread.start()

    def set_connection(self, value: bool) -> None:
        self.connection = value

    def load_settings(self, settings: Settings) -> None:
        if not self.connection:
            raise UartException("Serial не открыт")
        self.logger.info('Writing settings')
        self.serial_port.write(bytes.fromhex('11'))
        self.serial_port.write(bytes(settings))
        self.logger.success('Settings sent...')

    def disconnect(self) -> None:
        self.logger.info('Отключение...')
        if self.connector_thread.isRunning():
            self.connector_thread.make_disconnect()
            self.serial_port.reset_input_buffer()

        if self.connection:
            if self.serial_port:
                self.serial_port.write(bytes.fromhex('12'))
            self.connection = False

        if self.serial_port:
            self.serial_port.close()
            self.serial_port = None
