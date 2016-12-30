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

class Ui_windowAddAction(object):
    def setupUi(self, windowAddAction):
        windowAddAction.setObjectName(_fromUtf8("windowAddAction"))
        windowAddAction.resize(981, 574)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(windowAddAction.sizePolicy().hasHeightForWidth())
        windowAddAction.setSizePolicy(sizePolicy)
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
        windowAddAction.setPalette(palette)
        font = QtGui.QFont()
        font.setPointSize(12)
        windowAddAction.setFont(font)
        icon = QtGui.QIcon()
        icon.addPixmap(QtGui.QPixmap(_fromUtf8(":/logo2.ico")), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        windowAddAction.setWindowIcon(icon)
        windowAddAction.setAutoFillBackground(True)
        windowAddAction.setStyleSheet(_fromUtf8("QGroupBox { \n"
"     border: 0; \n"
" }"))
        self.centralwidget = QtGui.QWidget(windowAddAction)
        self.centralwidget.setObjectName(_fromUtf8("centralwidget"))
        self.gridLayout = QtGui.QGridLayout(self.centralwidget)
        self.gridLayout.setHorizontalSpacing(20)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.lvNewActionAllScripts = QtGui.QListWidget(self.centralwidget)
        font = QtGui.QFont()
        font.setPointSize(9)
        self.lvNewActionAllScripts.setFont(font)
        self.lvNewActionAllScripts.setObjectName(_fromUtf8("lvNewActionAllScripts"))
        self.gridLayout.addWidget(self.lvNewActionAllScripts, 2, 1, 1, 1)
        self.lvNewActionSelectedScripts = QtGui.QListWidget(self.centralwidget)
        font = QtGui.QFont()
        font.setPointSize(9)
        self.lvNewActionSelectedScripts.setFont(font)
        self.lvNewActionSelectedScripts.setObjectName(_fromUtf8("lvNewActionSelectedScripts"))
        self.gridLayout.addWidget(self.lvNewActionSelectedScripts, 2, 3, 1, 1)
        self.labNewActionName = QtGui.QLabel(self.centralwidget)
        font = QtGui.QFont()
        font.setPointSize(9)
        self.labNewActionName.setFont(font)
        self.labNewActionName.setObjectName(_fromUtf8("labNewActionName"))
        self.gridLayout.addWidget(self.labNewActionName, 0, 0, 1, 1)
        self.txtNewActionName = QtGui.QLineEdit(self.centralwidget)
        font = QtGui.QFont()
        font.setPointSize(9)
        self.txtNewActionName.setFont(font)
        self.txtNewActionName.setObjectName(_fromUtf8("txtNewActionName"))
        self.gridLayout.addWidget(self.txtNewActionName, 0, 1, 1, 1)
        self.groupBox = QtGui.QGroupBox(self.centralwidget)
        self.groupBox.setStyleSheet(_fromUtf8(""))
        self.groupBox.setTitle(_fromUtf8(""))
        self.groupBox.setObjectName(_fromUtf8("groupBox"))
        self.verticalLayout = QtGui.QVBoxLayout(self.groupBox)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        spacerItem = QtGui.QSpacerItem(20, 40, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        self.verticalLayout.addItem(spacerItem)
        self.btnNewActionPickScript = QtGui.QPushButton(self.groupBox)
        font = QtGui.QFont()
        font.setPointSize(9)
        self.btnNewActionPickScript.setFont(font)
        self.btnNewActionPickScript.setObjectName(_fromUtf8("btnNewActionPickScript"))
        self.verticalLayout.addWidget(self.btnNewActionPickScript)
        self.btnNewActionDropScript = QtGui.QPushButton(self.groupBox)
        font = QtGui.QFont()
        font.setPointSize(9)
        self.btnNewActionDropScript.setFont(font)
        self.btnNewActionDropScript.setObjectName(_fromUtf8("btnNewActionDropScript"))
        self.verticalLayout.addWidget(self.btnNewActionDropScript)
        spacerItem1 = QtGui.QSpacerItem(20, 40, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        self.verticalLayout.addItem(spacerItem1)
        self.gridLayout.addWidget(self.groupBox, 2, 2, 1, 1)
        self.btnNewActionSave = QtGui.QPushButton(self.centralwidget)
        self.btnNewActionSave.setMinimumSize(QtCore.QSize(0, 30))
        self.btnNewActionSave.setMaximumSize(QtCore.QSize(16777215, 30))
        font = QtGui.QFont()
        font.setPointSize(9)
        self.btnNewActionSave.setFont(font)
        self.btnNewActionSave.setObjectName(_fromUtf8("btnNewActionSave"))
        self.gridLayout.addWidget(self.btnNewActionSave, 3, 3, 1, 1)
        self.groupBox_2 = QtGui.QGroupBox(self.centralwidget)
        self.groupBox_2.setTitle(_fromUtf8(""))
        self.groupBox_2.setObjectName(_fromUtf8("groupBox_2"))
        self.horizontalLayout = QtGui.QHBoxLayout(self.groupBox_2)
        self.horizontalLayout.setContentsMargins(0, -1, -1, -1)
        self.horizontalLayout.setObjectName(_fromUtf8("horizontalLayout"))
        self.btnNewActionColor = QtGui.QPushButton(self.groupBox_2)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.btnNewActionColor.sizePolicy().hasHeightForWidth())
        self.btnNewActionColor.setSizePolicy(sizePolicy)
        self.btnNewActionColor.setText(_fromUtf8(""))
        self.btnNewActionColor.setObjectName(_fromUtf8("btnNewActionColor"))
        self.horizontalLayout.addWidget(self.btnNewActionColor)
        self.labNewActionColor = QtGui.QLabel(self.groupBox_2)
        self.labNewActionColor.setObjectName(_fromUtf8("labNewActionColor"))
        self.horizontalLayout.addWidget(self.labNewActionColor)
        self.gridLayout.addWidget(self.groupBox_2, 0, 3, 1, 1)
        windowAddAction.setCentralWidget(self.centralwidget)
        self.actionLoad_interface = QtGui.QAction(windowAddAction)
        self.actionLoad_interface.setObjectName(_fromUtf8("actionLoad_interface"))
        self.actionSave_interface = QtGui.QAction(windowAddAction)
        self.actionSave_interface.setObjectName(_fromUtf8("actionSave_interface"))

        self.retranslateUi(windowAddAction)
        QtCore.QMetaObject.connectSlotsByName(windowAddAction)

    def retranslateUi(self, windowAddAction):
        windowAddAction.setWindowTitle(_translate("windowAddAction", "Add or update a variable", None))
        self.labNewActionName.setText(_translate("windowAddAction", "Name", None))
        self.btnNewActionPickScript.setText(_translate("windowAddAction", ">>", None))
        self.btnNewActionDropScript.setText(_translate("windowAddAction", "<<", None))
        self.btnNewActionSave.setText(_translate("windowAddAction", "Add / Update Action", None))
        self.labNewActionColor.setText(_translate("windowAddAction", "Pick a color for this action", None))
        self.actionLoad_interface.setText(_translate("windowAddAction", "Load interface", None))
        self.actionSave_interface.setText(_translate("windowAddAction", "Save interface", None))

import images_rc
