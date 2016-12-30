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

from PyQt4.QtCore import *
from PyQt4.QtGui import *

from Options import *

import images_rc

from wAddVariable import Ui_windowAddVariable

class windowAddVariable(QMainWindow):
	def __init__(self, callback, actionToPerform, runtime, rowID=-1, name="", vartype="", value="", color="#000000", description=""):
		QMainWindow.__init__(self)
		self.ui = Ui_windowAddVariable()
		self.ui.setupUi(self)
		self.setWindowIcon(QtGui.QIcon(':/logo2.ico'))
		self.callback = callback
		self.runtime = runtime
		self.ui.txtNewVarName.setText(str(name))
		
		# 0 = String, 1 = Password, 2 = List 
		if str(vartype) == "String": self.ui.cbNewVarType.setCurrentIndex(0)
		elif str(vartype) == "Password": self.ui.cbNewVarType.setCurrentIndex(1)
		elif str(vartype) == "List": self.ui.cbNewVarType.setCurrentIndex(2)
		self.ui.txtNewVarValue.setText(str(value))		
		self.ui.txtNewVarDescription.setText(str(description))		
		self.ui.btnNewVarColor.setStyleSheet("background-color: " + color)		
		self.variableColor = color
			
		self.ui.txtNewVarName.returnPressed.connect(lambda: self.returnVariable(actionToPerform, rowID))		
		self.ui.txtNewVarValue.returnPressed.connect(lambda: self.returnVariable(actionToPerform, rowID))
		self.ui.btnNewVarColor.clicked.connect(lambda: self.pickVarColor())				
		self.ui.btnReturnNewVariable.clicked.connect(lambda: self.returnVariable(actionToPerform, rowID))
		self.ui.cbNewVarType.currentIndexChanged.connect(self.updateForm)		
		if (actionToPerform == "new"): self.setWindowTitle('Create a new variable')
		elif (actionToPerform == "update"): self.setWindowTitle('Update variable: ' + name)
		self.updateForm()
	
		self.resetFontSize(Options.Global_Font_Size)
		
	def resetFontSize(self, size):        
		font = self.ui.labNewVarDescription.font()
		font.setPointSize(size)
		self.ui.labNewVarDescription.setFont(font)
		self.ui.labNewVarName.setFont(font)
		self.ui.labNewVarType.setFont(font)
		self.ui.labNewVarValue.setFont(font)
		self.ui.labNewVarColor.setFont(font)
		self.ui.btnReturnNewVariable.setFont(font)
		self.ui.btnNewVarColor.setFont(font)
		self.ui.cbNewVarType.setFont(font)
		self.ui.txtNewVarDescription.setFont(font)
		self.ui.txtNewVarName.setFont(font)
		self.ui.txtNewVarValue.setFont(font)
	
	def pickVarColor(self):
		color = QColorDialog.getColor(QtCore.Qt.green, self)
		if color.isValid():
			self.variableColor = color.name()			
			self.ui.btnNewVarColor.setStyleSheet("background-color: " + self.variableColor)			
			
	def isValid(self, varid, name, actionToPerform):
		response = True
		if actionToPerform is "new":
			for variable in self.runtime.listVariables:
				if variable.name == name:
					QMessageBox().critical(self, "Alert", "Another variable called " + name + " already exists. Choose another name or delete that variable first.")
					response = False
					break
		if actionToPerform is "update":
			for variable in self.runtime.listVariables:
				if variable.rowID != varid and variable.name == name:
					QMessageBox().critical(self, "Alert", "Another variable called " + name + " already exists. Choose another name or delete that variable first.")
					response = False
					break
		if ";" in name: 
			QMessageBox().critical(self, "Alert", "A variable name cannot contain a semi-column.")
			response = False
		if name == "": 
			QMessageBox().critical(self, "Alert", "A variable must have a name.")
			response = False
		try: 
			xxx = str(name).decode('ascii')
		except:
			QMessageBox().critical(self, "Alert", "The name of the variable contains illegal character. Please stick to Unicode.")
			response = False			
		return response
		
	def updateForm(self):
		if self.ui.cbNewVarType.currentText() == "Password": self.ui.txtNewVarValue.setEchoMode(QLineEdit.Password)
		else: self.ui.txtNewVarValue.setEchoMode(QLineEdit.Normal)
		
	def returnVariable(self, actionToPerform, rowID):
		if (actionToPerform == "new"):
			name = self.ui.txtNewVarName.text()
			vartype = self.ui.cbNewVarType.currentText()
			value = self.ui.txtNewVarValue.text()
			color = self.variableColor
			description = self.ui.txtNewVarDescription.toPlainText()
			if self.isValid(-1, name, actionToPerform):	
				self.callback(name, vartype, value, color, description)
				self.close()
		elif (actionToPerform == "update"):
			varid = rowID
			name = self.ui.txtNewVarName.text()
			vartype = self.ui.cbNewVarType.currentText()
			value = self.ui.txtNewVarValue.text()
			color = self.variableColor
			description = self.ui.txtNewVarDescription.toPlainText()
			if self.isValid(varid, name, actionToPerform):		
				self.callback(varid, name, vartype, value, color, description)
				self.close()