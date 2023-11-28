import re

from PyQt6 import QtWidgets
from PyQt6 import QtCore
from PyQt6 import QtGui


class BaseCommand(QtWidgets.QFrame):
    command_name = None

    def __init__(
            self,
            callback_func: callable,
            update_callback_func: callable,
    ):
        if self.__class__.command_name is None:
            raise ValueError('"command_name" is not Undefined')

        super().__init__()
        self.setGeometry(QtCore.QRect(20, 50, 361, 61))
        self.setStyleSheet("background-color: rgb(40, 41, 65);")
        pushButton = QtWidgets.QPushButton(parent=self)
        pushButton.setGeometry(QtCore.QRect(330, 20, 21, 21))
        pushButton.setStyleSheet(
            "QPushButton{\n"
            "background-color: rgba(182, 149, 243, 0.5);\n"
            "border 1px: solid;\n"
            "border-color:  rgb(255, 0, 25);\n"
            "border-radius: 10px;\n"
            "}"
        )

        self.name_label = QtWidgets.QLabel(parent=self)
        self.name_label.setGeometry(QtCore.QRect(10, 10, 61, 41))
        self.name_label.setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)
        self.name_label.setText(self.command_name)

        pushButton.setText("")
        icon = QtGui.QIcon()
        icon.addPixmap(QtGui.QPixmap("ui/icons/close.png"),
                       QtGui.QIcon.Mode.Normal, QtGui.QIcon.State.Off)
        pushButton.setIcon(icon)
        pushButton.setIconSize(QtCore.QSize(10, 10))
        pushButton.clicked.connect(callback_func)

        self.update_func = update_callback_func

    def get_command_args(self) -> dict:
        raise NotImplementedError(
            f"get_command_args not implemented in {self.__class__.__name__}"
        )


class CommandIdleUS(BaseCommand):
    command_name = 'IDLE_US'

    def __init__(
            self,
            callback_func: callable,
            update_callback_func: callable,
            idling_time: int | float = 0,
    ):
        super().__init__(callback_func, update_callback_func)

        self.idling_edit = QtWidgets.QLineEdit(parent=self)
        self.idling_edit.setGeometry(QtCore.QRect(80, 10, 190, 41))
        self.idling_edit.setStyleSheet(
            "QLineEdit{\n"
            "background-color: rgba(37, 39, 61, 240);\n"
            "border: 1px solid;\n"
            "border-color: rgb(182, 149, 243);\n"
            "}\n"
            "\n"
            ""
        )
        self.idling_edit.setText(str(int(idling_time)))

        self.idling_edit.setAlignment(QtCore.Qt.AlignmentFlag.AlignVCenter)
        self.idling_edit.setAlignment(QtCore.Qt.AlignmentFlag.AlignHCenter)

        self.idling_edit.textChanged.connect(self.text_change_handle)

        self.idling_edit_label = QtWidgets.QLabel(parent=self)
        self.idling_edit_label.setGeometry(QtCore.QRect(280, 10, 30, 41))
        self.idling_edit_label.setText('мкс.')

    def text_change_handle(self) -> None:
        self.validate_idling_edit()
        self.update_func()

    def validate_idling_edit(self) -> None:
        pattern = r'^\d+$'
        idling_edit_value = self.idling_edit.text()
        res = bool(re.match(pattern, idling_edit_value))
        if idling_edit_value and not res:
            QtWidgets.QMessageBox.warning(self, 'Ошибка', 'Невалидные данные')
            self.idling_edit.setText(str(0))

    def get_command_args(self) -> dict:
        idling_time = float(self.idling_edit.text() or '0')
        return {
            'command': self.command_name, 'args': [idling_time]
        }


class MovementCommand(BaseCommand):
    command_name = None

    def __init__(
            self,
            callback_func: callable,
            update_callback_func: callable,
            distance_mm: float = 0.0,
            speed_mm: float = 0.0,
    ):
        super().__init__(callback_func, update_callback_func)

        edit_stylesheet = (
            "QLineEdit{\n"
            "background-color: rgba(37, 39, 61, 240);\n"
            "border: 1px solid;\n"
            "border-color: rgb(182, 149, 243);\n"
            "}\n"
            "\n"
            ""
        )

        # DISTANCE FIELD
        self.distance_edit = QtWidgets.QLineEdit(parent=self)
        self.distance_edit.setGeometry(QtCore.QRect(80, 10, 80, 41))
        self.distance_edit.setStyleSheet(edit_stylesheet)
        self.distance_edit.setText(str(distance_mm))
        self.distance_edit.setAlignment(QtCore.Qt.AlignmentFlag.AlignVCenter)
        self.distance_edit.setAlignment(QtCore.Qt.AlignmentFlag.AlignHCenter)
        self.distance_edit.textChanged.connect(
            lambda _: self.text_change_handle(self.distance_edit)
        )
        self.distance_edit_label = QtWidgets.QLabel(parent=self)
        self.distance_edit_label.setGeometry(QtCore.QRect(165, 10, 30, 41))
        self.distance_edit_label.setText('мм.')

        # SPEED FIELD
        self.speed_edit = QtWidgets.QLineEdit(parent=self)
        self.speed_edit.setGeometry(QtCore.QRect(200, 10, 80, 41))
        self.speed_edit.setStyleSheet(edit_stylesheet)
        self.speed_edit.setText(str(speed_mm))
        self.speed_edit.setAlignment(QtCore.Qt.AlignmentFlag.AlignVCenter)
        self.speed_edit.setAlignment(QtCore.Qt.AlignmentFlag.AlignHCenter)
        self.speed_edit.textChanged.connect(
            lambda _: self.text_change_handle(self.speed_edit)
        )
        self.speed_edit_label = QtWidgets.QLabel(parent=self)
        self.speed_edit_label.setGeometry(QtCore.QRect(285, 10, 35, 41))
        self.speed_edit_label.setText('мм/c.')

    def text_change_handle(self, field: QtWidgets.QLineEdit) -> None:
        self.validate_float(field)
        self.update_func()

    def validate_float(self, field: QtWidgets.QLineEdit) -> None:
        pattern = r'^(\d+(\.\d*)?|\.\d+)(\d+)?$'
        idling_edit_value = field.text()
        res = bool(re.match(pattern, idling_edit_value))
        if idling_edit_value and not res:
            QtWidgets.QMessageBox.warning(self, 'Ошибка', 'Невалидные данные')
            field.setText(str(0.0))

    def get_command_args(self) -> dict:
        distance = float(self.distance_edit.text() or '0')
        speed = float(self.speed_edit.text() or '0')
        return {
            'command': self.command_name,
            'args': [
                distance,
                speed,
            ]
        }


class CommandUP(MovementCommand):
    command_name = 'UP'


class CommandDown(MovementCommand):
    command_name = 'DOWN'



