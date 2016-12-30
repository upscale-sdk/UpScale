# -----------------------------------------------------------------------------
# This file is part of P-SOCRATES Analyzer (http://www.p-socrates.eu).
#
# (C) Copyright 2016 Instituto Superior de Engenharia do Porto
#
# Author: Vincent Nelis <nelis@isep.ipp.pt>
# 
# P-SOCRATES Analyzer is free software; you can redistribute it
# and/or modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation, 
# (with a special exception described below).
# 
# Linking this code statically or dynamically with other modules is
# making a combined work based on this code.  Thus, the terms and
# conditions of the GNU General Public License cover the whole
# combination.
# 
# As a special exception, the copyright holders of this library give you
# permission to link this code with independent modules to produce an
# executable, regardless of the license terms of these independent
# modules, and to copy and distribute the resulting executable under
# terms of your choice, provided that you also meet, for each linked
# independent module, the terms and conditions of the license of that
# module.  An independent module is a module which is not derived from
# or based on this library.  If you modify this code, you may extend
# this exception to your version of the code, but you are not
# obligated to do so.  If you do not wish to do so, delete this
# exception statement from your version.
# 
# P-SOCRATES Analyzer is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License version 2 for more details.
# 
# You should have received a copy of the GNU General Public License
# version 2 along with P-SOCRATES Analyzer; if not, write to the
# Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA.
# 
# -----------------------------------------------------------------------------

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)

class Ui_windowAddVariable(object):
    def setupUi(self, windowAddVariable):
        windowAddVariable.setObjectName(_fromUtf8("windowAddVariable"))
        windowAddVariable.resize(580, 370)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(windowAddVariable.sizePolicy().hasHeightForWidth())
        windowAddVariable.setSizePolicy(sizePolicy)
        palette = QtGui.QPalette()
        brush = QtGui.QBrush(QtGui.QColor(255, 255, 255))
        brush.setStyle(QtCore.Qt.SolidPattern)
        palette.setBrush(QtGui.QPalette.Active, QtGui.QPalette.Base, brush)
        brush = QtGui.QBrush(QtGui.QColor(255, 255, 255))
        brush.setStyle(QtCore.Qt.SolidPattern)
        palette.setBrush(QtGui.QPalette.Active, QtGui.QPalette.Window, brush)
        brush = QtGui.QBrush(QtGui.QColor(255, 255, 255))
        brush.setStyle(QtCore.Qt.SolidPattern)
        palette.setBrush(QtGui.QPalette.Inactive, QtGui.QPalette.Base, brush)
        brush = QtGui.QBrush(QtGui.QColor(255, 255, 255))
        brush.setStyle(QtCore.Qt.SolidPattern)
        palette.setBrush(QtGui.QPalette.Inactive, QtGui.QPalette.Window, brush)
        brush = QtGui.QBrush(QtGui.QColor(255, 255, 255))
        brush.setStyle(QtCore.Qt.SolidPattern)
        palette.setBrush(QtGui.QPalette.Disabled, QtGui.QPalette.Base, brush)
        brush = QtGui.QBrush(QtGui.QColor(255, 255, 255))
        brush.setStyle(QtCore.Qt.SolidPattern)
        palette.setBrush(QtGui.QPalette.Disabled, QtGui.QPalette.Window, brush)
        windowAddVariable.setPalette(palette)
        font = QtGui.QFont()
        font.setPointSize(9)
        windowAddVariable.setFont(font)
        icon = QtGui.QIcon()
        icon.addPixmap(QtGui.QPixmap(_fromUtf8(":/logo2.ico")), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        windowAddVariable.setWindowIcon(icon)
        windowAddVariable.setAutoFillBackground(True)
        self.centralwidget = QtGui.QWidget(windowAddVariable)
        self.centralwidget.setObjectName(_fromUtf8("centralwidget"))
        self.gridLayout = QtGui.QGridLayout(self.centralwidget)
        self.gridLayout.setHorizontalSpacing(20)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.labNewVarName = QtGui.QLabel(self.centralwidget)
        font = QtGui.QFont()
        font.setPointSize(9)
        self.labNewVarName.setFont(font)
        self.labNewVarName.setObjectName(_fromUtf8("labNewVarName"))
        self.gridLayout.addWidget(self.labNewVarName, 0, 0, 1, 1)
        self.txtNewVarName = QtGui.QLineEdit(self.centralwidget)
        font = QtGui.QFont()
        font.setPointSize(9)
        self.txtNewVarName.setFont(font)
        self.txtNewVarName.setObjectName(_fromUtf8("txtNewVarName"))
        self.gridLayout.addWidget(self.txtNewVarName, 0, 2, 1, 1)
        self.labNewVarType = QtGui.QLabel(self.centralwidget)
        font = QtGui.QFont()
        font.setPointSize(9)
        self.labNewVarType.setFont(font)
        self.labNewVarType.setObjectName(_fromUtf8("labNewVarType"))
        self.gridLayout.addWidget(self.labNewVarType, 1, 0, 1, 1)
        self.labNewVarValue = QtGui.QLabel(self.centralwidget)
        font = QtGui.QFont()
        font.setPointSize(9)
        self.labNewVarValue.setFont(font)
        self.labNewVarValue.setObjectName(_fromUtf8("labNewVarValue"))
        self.gridLayout.addWidget(self.labNewVarValue, 2, 0, 1, 1)
        self.cbNewVarType = QtGui.QComboBox(self.centralwidget)
        font = QtGui.QFont()
        font.setPointSize(9)
        self.cbNewVarType.setFont(font)
        self.cbNewVarType.setObjectName(_fromUtf8("cbNewVarType"))
        self.cbNewVarType.addItem(_fromUtf8(""))
        self.cbNewVarType.addItem(_fromUtf8(""))
        self.cbNewVarType.addItem(_fromUtf8(""))
        self.gridLayout.addWidget(self.cbNewVarType, 1, 2, 1, 1)
        self.txtNewVarValue = QtGui.QLineEdit(self.centralwidget)
        font = QtGui.QFont()
        font.setPointSize(9)
        self.txtNewVarValue.setFont(font)
        self.txtNewVarValue.setObjectName(_fromUtf8("txtNewVarValue"))
        self.gridLayout.addWidget(self.txtNewVarValue, 2, 2, 1, 1)
        self.labNewVarDescription = QtGui.QLabel(self.centralwidget)
        font = QtGui.QFont()
        font.setPointSize(9)
        self.labNewVarDescription.setFont(font)
        self.labNewVarDescription.setObjectName(_fromUtf8("labNewVarDescription"))
        self.gridLayout.addWidget(self.labNewVarDescription, 4, 0, 1, 1)
        self.txtNewVarDescription = QtGui.QTextEdit(self.centralwidget)
        font = QtGui.QFont()
        font.setPointSize(9)
        self.txtNewVarDescription.setFont(font)
        self.txtNewVarDescription.setObjectName(_fromUtf8("txtNewVarDescription"))
        self.gridLayout.addWidget(self.txtNewVarDescription, 4, 2, 1, 1)
        self.btnReturnNewVariable = QtGui.QPushButton(self.centralwidget)
        self.btnReturnNewVariable.setMinimumSize(QtCore.QSize(0, 30))
        self.btnReturnNewVariable.setMaximumSize(QtCore.QSize(16777215, 30))
        font = QtGui.QFont()
        font.setPointSize(9)
        self.btnReturnNewVariable.setFont(font)
        self.btnReturnNewVariable.setObjectName(_fromUtf8("btnReturnNewVariable"))
        self.gridLayout.addWidget(self.btnReturnNewVariable, 5, 2, 1, 1)
        self.labNewVarColor = QtGui.QLabel(self.centralwidget)
        self.labNewVarColor.setObjectName(_fromUtf8("labNewVarColor"))
        self.gridLayout.addWidget(self.labNewVarColor, 3, 0, 1, 1)
        self.btnNewVarColor = QtGui.QPushButton(self.centralwidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.btnNewVarColor.sizePolicy().hasHeightForWidth())
        self.btnNewVarColor.setSizePolicy(sizePolicy)
        self.btnNewVarColor.setAutoFillBackground(True)
        self.btnNewVarColor.setText(_fromUtf8(""))
        self.btnNewVarColor.setObjectName(_fromUtf8("btnNewVarColor"))
        self.gridLayout.addWidget(self.btnNewVarColor, 3, 2, 1, 1)
        windowAddVariable.setCentralWidget(self.centralwidget)
        self.actionLoad_interface = QtGui.QAction(windowAddVariable)
        self.actionLoad_interface.setObjectName(_fromUtf8("actionLoad_interface"))
        self.actionSave_interface = QtGui.QAction(windowAddVariable)
        self.actionSave_interface.setObjectName(_fromUtf8("actionSave_interface"))

        self.retranslateUi(windowAddVariable)
        QtCore.QMetaObject.connectSlotsByName(windowAddVariable)
        windowAddVariable.setTabOrder(self.txtNewVarName, self.txtNewVarValue)
        windowAddVariable.setTabOrder(self.txtNewVarValue, self.txtNewVarDescription)
        windowAddVariable.setTabOrder(self.txtNewVarDescription, self.btnReturnNewVariable)

    def retranslateUi(self, windowAddVariable):
        windowAddVariable.setWindowTitle(_translate("windowAddVariable", "Add or update a variable", None))
        self.labNewVarName.setText(_translate("windowAddVariable", "Name", None))
        self.labNewVarType.setText(_translate("windowAddVariable", "Type", None))
        self.labNewVarValue.setText(_translate("windowAddVariable", "Value", None))
        self.cbNewVarType.setItemText(0, _translate("windowAddVariable", "String", None))
        self.cbNewVarType.setItemText(1, _translate("windowAddVariable", "Password", None))
        self.cbNewVarType.setItemText(2, _translate("windowAddVariable", "List", None))
        self.labNewVarDescription.setText(_translate("windowAddVariable", "Description", None))
        self.btnReturnNewVariable.setText(_translate("windowAddVariable", "Add / Update Variable", None))
        self.labNewVarColor.setText(_translate("windowAddVariable", "Color", None))
        self.actionLoad_interface.setText(_translate("windowAddVariable", "Load interface", None))
        self.actionSave_interface.setText(_translate("windowAddVariable", "Save interface", None))

import images_rc
