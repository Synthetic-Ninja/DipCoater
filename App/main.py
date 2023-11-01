from PyQt6.QtWidgets import QWidget
from PyQt6.QtWidgets import QApplication
from PyQt6 import uic
from output import Ui_MainWindow



if __name__ == '__main__':
    app = QApplication([])
    window = Ui_MainWindow()
    window.show()
    app.exec()