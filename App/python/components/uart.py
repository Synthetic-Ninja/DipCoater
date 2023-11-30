import time

import serial
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


class UartConnector:
    def __init__(self, logger: GuiLogger):
        self.logger = logger
        self.serial_port = None
        self.connection = False

    def make_connection(self, path_to_serial_file: str) -> None:
        self.logger.info('Opening serial as 115200 baud')
        self.serial_port = serial.Serial(path_to_serial_file, 115200)
        try:
            self.serial_port.close()
            self.serial_port.open()
        except serial.SerialException as exc:
            self.logger.error(str(exc))
            self.serial_port.close()
            raise RuntimeError("Ошибка открытия serial")
        self.logger.info('Serial opened...')
        self.serial_port.reset_input_buffer()
        self.logger.info('Sending SYN')
        self.logger.info('Waiting ACK byte from device')
        return
        while True:
            self.serial_port.write(bytes.fromhex('F0'))
            for i in range(10000):
                if self.serial_port.in_waiting > 0:
                    raw_byte = self.serial_port.read()
                    if raw_byte == bytes.fromhex('F1'):
                        self.logger.info('ACK byte received')
                        self.serial_port.write(bytes.fromhex('F2'))
                        self.connection = True
                        self.logger.success('Connection established')
                    return

    def load_settings(self, settings: Settings) -> None:
        if not self.connection:
            raise RuntimeError("Serial не открыт")

        self.logger.info('Writing settings')
        self.serial_port.write(bytes.fromhex('11'))
        self.serial_port.write(bytes(settings))
        self.logger.success('Settings sent...')

