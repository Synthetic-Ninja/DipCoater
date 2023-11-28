import os.path
import sys
import json.decoder
import marshmallow.exceptions
from PyQt6 import QtWidgets
from PyQt6 import QtCore
from PyQt6.QtCore import QSize

from output import Ui_MainWindow
from schemas import ProgramSchema
from schemas import Program
from commands import CommandIdleUS
from commands import CommandUP
from commands import CommandDown


class MyWindow(QtWidgets.QMainWindow):
    def __init__(self):
        # UI INITIALIZATION
        super(MyWindow, self).__init__()

        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        # SET FIRST START PAGE
        self._go_to_page(self.ui.create_program_page)

        self._binding_butons()

        # ATTRS FOR LOGIC INITIALIZATION
        self.program_file_path: str = ''
        self.device_file_path: str = ''
        self.program: dict = {}

        self.ui.listWidget.setVerticalScrollBar(self.ui.verticalScrollBar)

    @staticmethod
    def _button_handler(
            buttons: tuple[QtWidgets.QPushButton],
            target_button: QtWidgets.QPushButton,
            handler: callable,
    ):
        inactive_btn_style = """
        QPushButton{
            background-color: rgba(182, 149, 243, 0);
            border: 1px solid rgba(182, 149, 243, 0);
            border-radius: 20px;
        }
        """

        active_btn_style = """
            QPushButton{
                background-color: rgb(182, 149, 243);
                border: 1px solid;
                border-radius: 20px;
            }
            """
        for button in buttons:
            button.setStyleSheet(
                active_btn_style
                if button is target_button
                else inactive_btn_style
            )
        handler()

    def _binding_butons(self) -> None:

        MENU_BUTTONS = (
            self.ui.ProgramModeBtn,
            self.ui.SettingsBtn,
            self.ui.ControllerProgrammingBtn,
        )
        # BINDING MODES CHANGE BUTTONS

        self.ui.ProgramModeBtn.clicked.connect(
            lambda _: self._button_handler(
                MENU_BUTTONS,
                self.ui.ProgramModeBtn,
                self.go_to_create_program_page,
            )

        )

        self.ui.SettingsBtn.clicked.connect(
            lambda _: self._button_handler(
                MENU_BUTTONS,
                self.ui.SettingsBtn,
                self.go_to_settings_page,
            )
        )

        self.ui.ControllerProgrammingBtn.clicked.connect(
            lambda _: self._button_handler(
                MENU_BUTTONS,
                self.ui.ControllerProgrammingBtn,
                self.go_to_device_configuration_page,
            )
        )

        # BINDING OPEN FILE BUTTON
        self.ui.UploudFileBtn.clicked.connect(self.open_file_dialog)
        self.ui.LoadProgramBtn.clicked.connect(self.load_program_file)

        # BINDING SAVE PROGRAM BUTTON
        self.ui.SaveProgramBtn.clicked.connect(self.save_program_handle)

        # BINDING ADD COMMAND
        self.ui.AddCommandBtn.clicked.connect(self.add_command_handle)

    def _go_to_page(self, page: QtWidgets.QWidget) -> None:
        self.ui.stackedWidget.setCurrentWidget(page)

    def go_to_create_program_page(self) -> None:
        self._go_to_page(self.ui.create_program_page)

    def go_to_settings_page(self) -> None:
        self._go_to_page(self.ui.settings_page)

    def go_to_device_configuration_page(self) -> None:
        self._go_to_page(self.ui.device_configuration_page)

    def _raise_error(self, error_msg: str) -> None:
        QtWidgets.QMessageBox.critical(self, "Ошибка:", error_msg,)

    def _raise_warning(self, error_msg: str) -> None:
        QtWidgets.QMessageBox.warning(self, "Предупреждение:", error_msg,)

    def open_file_dialog(self):
        file_path, *_ = QtWidgets.QFileDialog.getOpenFileName(
            self,
            "QFileDialog.getOpenFileName()",
            "",
            "All Files (*);;Python Files (*.py)",
        )
        if file_path:
            self.program_file_path = file_path
            name = '/'.join(self.program_file_path.split('/')[-2:])
            self.ui.ProgramFileNameLabel.setText(name)

    def load_program_file(self):
        if not self.program_file_path:
            return
        try:
            with open(self.program_file_path, 'r') as file:
                file_data = file.read()
                program: Program = ProgramSchema().loads(file_data)

        except (
                json.decoder.JSONDecodeError,
                marshmallow.exceptions.ValidationError
        ) as err:
            msg = f'Не удалось прочитать данные из файла\n Ошибка: {err}'
            self._raise_error(msg)
            return

        self.ui.ProgramNameEdit.setText(self.program_file_path.split('/')[-1])
        self.ui.ProgramVersionEdit.setText(program.version)

        self.ui.listWidget.clear()
        errored_parsed_commands = 0
        for command in program.program_body['commands_list']:
            command_name = command['command']
            command_args = command['args']
            try:
                self.add_command(command_name, *command_args)
            except ValueError:
                errored_parsed_commands += 1
                continue
        if errored_parsed_commands:
            msg = (
                f"Количество команд, обработанных с ошибками, которые"
                f" не были добавлены: {errored_parsed_commands}"
            )
            self._raise_warning(msg)

    def update_executing_time(self) -> None:
        time = 0
        for row_index in range(self.ui.listWidget.count()):
            item = self.ui.listWidget.item(row_index)
            command_item_args = (
                self.ui.listWidget.itemWidget(item).get_command_args()
            )
            command_name = command_item_args['command']
            command_args = command_item_args['args']

            match command_name:
                case 'UP':
                    if command_args[1]:
                        command_time = command_args[0] / command_args[1]
                    else:
                        command_time = 0
                    time += command_time

                case 'DOWN':
                    if command_args[1]:
                        command_time = command_args[0] / command_args[1]
                    else:
                        command_time = 0
                    time += command_time

                case 'IDLE_US':
                    time += command_args[0] / 1000000

        self.ui.ExecutionTimeLabel.setText(f'≈ {time:.2f} с.')

    def parse_commands(self) -> dict:

        commands_len = self.ui.listWidget.count()
        commands_list = []
        for row_index in range(self.ui.listWidget.count()):
            item = self.ui.listWidget.item(row_index)
            commands_list.append(
                self.ui.listWidget.itemWidget(item).get_command_args()
            )

        program_body = {
            'program_body': {
                'commands_len': commands_len,
                'commands_list': commands_list,
            }
        }

        return program_body

    def remove_command(self, item: QtWidgets.QListWidgetItem) -> None:
        self.ui.listWidget.takeItem(self.ui.listWidget.row(item))
        self.ui.CommandsCountLabel.setText(str(self.ui.listWidget.count()))
        self.update_executing_time()

    def add_command(
            self,
            command_name: str,
            *args,
    ) -> None:
        match command_name:
            case 'UP':
                command = CommandUP

            case 'DOWN':
                command = CommandDown

            case 'IDLE_US':
                command = CommandIdleUS

            case _:
                self._raise_error(f"Команда {command_name} не найдена")
                raise ValueError
        item = QtWidgets.QListWidgetItem()
        item.setSizeHint(QSize(371, 81))
        try:
            command = command(
                lambda _: self.remove_command(item),
                self.update_executing_time,
                *args,
            )
        except TypeError:
            msg = f"Неопознанные аргументы для команды {command.command_name}"
            self._raise_error(msg)
            raise ValueError

        self.ui.listWidget.addItem(item)
        self.ui.listWidget.setItemWidget(item, command)
        self.ui.CommandsCountLabel.setText(str(self.ui.listWidget.count()))
        self.update_executing_time()

    def add_command_handle(self) -> None:
        items = ['UP', 'DOWN', 'IDLE_US']
        item, ok = QtWidgets.QInputDialog().getItem(
            self,
            'Выберите команду',
            'Команда:',
            items, 0, False)
        if ok and item:
            self.add_command(item)

    def save_program_handle(self) -> None:
        if not (program_name := self.ui.ProgramNameEdit.text()):
            self._raise_error("Отсутствует имя программы")
            return
        if not (program_version := self.ui.ProgramVersionEdit.text()):
            self._raise_error("Отсутствует версия программы")
            return
        if not self.ui.listWidget.count():
            self._raise_error("Отсутствуют команды для программы")
            return

        if not program_name.endswith('.json'):
            program_name += '.json'

        dir_path = QtWidgets.QFileDialog.getExistingDirectory()
        file_path = f'{dir_path}/{program_name}'
        if os.path.exists(file_path):
            i = 0
            copy_path = ''
            raw_prog_name, *_ = program_name.split('.json')
            while True:
                copy_path = f'{dir_path}/{raw_prog_name}({i}).json'
                if not os.path.exists(copy_path):
                    break
                i += 1
            file_path = copy_path
        with open(file_path, 'w') as file:
            program = {'version': program_version}
            program.update(self.parse_commands())
            dumped_program = ProgramSchema().dumps(program)
            file.write(dumped_program)

        self.program_file_path = file_path
        file_name = '/'.join(file_path.split('/')[-2:])
        self.ui.ProgramFileNameLabel.setText(file_name)


def main():
    app = QtWidgets.QApplication(sys.argv)
    w = MyWindow()
    w.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    exit(main())
