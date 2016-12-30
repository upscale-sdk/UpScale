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

from wAddAction import Ui_windowAddAction

class windowAddAction(QMainWindow):
	def __init__(self, callback, actionToPerform, runtime, rowID=-1, name=""):
		QMainWindow.__init__(self)
		self.ui = Ui_windowAddAction()
		self.ui.setupUi(self)
		self.setWindowIcon(QtGui.QIcon(':/logo2.ico'))
		self.callback = callback
		self.actionToPerform = actionToPerform
		self.runtime = runtime
		self.actionColor = "#000000"
		for action in self.runtime.listActions:
			if (action.name == name):
				self.action = action
				self.actionColor = action.color
				break
		self.rowID = rowID		
		self.selectedCommands = []
		
		self.ui.txtNewActionName.setText(str(name))
		self.ui.txtNewActionName.installEventFilter(self)
		self.ui.btnNewActionColor.clicked.connect(lambda: self.pickVarColor())
		self.ui.btnNewActionSave.clicked.connect(lambda: self.returnAction(callback, actionToPerform))
		self.ui.btnNewActionPickScript.clicked.connect(self.pickScript)
		self.ui.lvNewActionAllScripts.doubleClicked.connect(self.pickScript)
		self.ui.btnNewActionDropScript.clicked.connect(self.dropScript)
		self.ui.lvNewActionSelectedScripts.doubleClicked.connect(self.dropScript)	
		# Initialize the left-hand list of all commands and actions available 	
		for command in self.runtime.listCommands:		
			self.ui.lvNewActionAllScripts.addItem(str(command.name))
			line = self.ui.lvNewActionAllScripts.item(len(self.ui.lvNewActionAllScripts) - 1)
			foreground, background = command.display()				
			line.setForeground(foreground)
			line.setBackground(background)
		for action in self.runtime.listActions:
			self.ui.lvNewActionAllScripts.addItem(str(action.name))
			line = self.ui.lvNewActionAllScripts.item(len(self.ui.lvNewActionAllScripts) - 1)			
			foreground, background = action.display()				
			line.setForeground(foreground)
			line.setBackground(background)		
		if (actionToPerform == "new"): 
			self.setWindowTitle('Create a new action')
			self.ui.btnNewActionSave.setText("Create the action")
		elif (actionToPerform == "update"): 
			# print all commands of this action
			self.setWindowTitle('Update action: ' + name)
			self.ui.txtNewActionName.setText(name)		
			self.ui.btnNewActionSave.setText("Update the action")
			self.ui.btnNewActionColor.setStyleSheet("background-color: " + self.actionColor)
			for itemName in self.action.commandlist:
				self.selectedCommands.append(itemName)
			self.refreshPanelSelectedCommands()
		# set up the QListWidget with the commands to make it "drag and drop" capable
		self.ui.lvNewActionSelectedScripts.setAcceptDrops(True)
		self.ui.lvNewActionSelectedScripts.setDragEnabled(True)
		self.ui.lvNewActionSelectedScripts.setSelectionMode(QAbstractItemView.SingleSelection)
		self.ui.lvNewActionSelectedScripts.setDropIndicatorShown(True)
		self.ui.lvNewActionSelectedScripts.setDragDropMode(QAbstractItemView.InternalMove)
		self.ui.lvNewActionSelectedScripts.viewport().installEventFilter(self)
		
		self.resetFontSize(Options.Global_Font_Size)		
		
	def resetFontSize(self, size):        
		font = self.ui.labNewActionName.font()
		font.setPointSize(size)
		self.ui.labNewActionName.setFont(font)
		self.ui.lvNewActionAllScripts.setFont(font)
		self.ui.lvNewActionSelectedScripts.setFont(font)
		self.ui.txtNewActionName.setFont(font)
		self.ui.btnNewActionDropScript.setFont(font)
		self.ui.btnNewActionPickScript.setFont(font)
		self.ui.btnNewActionSave.setFont(font)
		self.ui.labNewActionColor.setFont(font)
	
	def pickVarColor(self):
		color = QColorDialog.getColor(QtCore.Qt.green, self)
		if color.isValid():
			self.actionColor  = color.name()			
			self.ui.btnNewActionColor.setStyleSheet("background-color: " + self.actionColor )	
			
	def eventFilter(self, source, event):
		if (event.type() == QtCore.QEvent.KeyPress and source is self.ui.txtNewActionName):
			pressedkey = QKeyEvent(event)
			if (pressedkey.key() == Qt.Key_Enter) or (pressedkey.key() == Qt.Key_Return):
				self.returnAction(self.callback, self.actionToPerform)
		
		elif (event.type() == QtCore.QEvent.Drop and source is self.ui.lvNewActionSelectedScripts.viewport()):
			# get index to insert at
			insertPos = event.pos()
			fromList = event.source() # is QListWidget = self.ui.lvNewActionSelectedScripts, source is QWidget = self.ui.lvNewActionSelectedScripts.viewport()			
			toRow = fromList.row(fromList.itemAt(insertPos))
			itemName = self.ui.lvNewActionSelectedScripts.currentItem().text()
			fromRow = self.ui.lvNewActionSelectedScripts.currentRow()	
			if fromRow < toRow: toRow = toRow + 1
			else: fromRow = fromRow + 1
			self.selectedCommands.insert(toRow, itemName)
			self.selectedCommands.pop(fromRow)			
			self.refreshPanelSelectedCommands()
			return True
		return QMainWindow.eventFilter(self, source, event)

	def refreshPanelSelectedCommands(self):
		self.ui.lvNewActionSelectedScripts.clear()
		itemIndex = 0		
		for itemName in self.selectedCommands:
			itemFound = False
			for command in self.runtime.listCommands:			
				if (command.name == itemName):					
					self.ui.lvNewActionSelectedScripts.insertItem(itemIndex, itemName)
					line = self.ui.lvNewActionSelectedScripts.item(itemIndex)											
					foreground, background = command.display()				
					line.setForeground(foreground)
					line.setBackground(background)
					itemFound = True								
					itemIndex = itemIndex + 1
					break
			if itemFound is False:
				for action in self.runtime.listActions:
					if (action.name == itemName):
						self.ui.lvNewActionSelectedScripts.insertItem(itemIndex, itemName)
						line = self.ui.lvNewActionSelectedScripts.item(itemIndex)											
						foreground, background = action.display()
						line.setForeground(foreground)
						line.setBackground(background)
						itemFound = True
						itemIndex = itemIndex + 1
						break
			if itemFound is False:
				self.ui.lvNewActionSelectedScripts.insertItem(itemIndex,"### ERROR: command or action not found: '" + itemName + "'")
				itemIndex = itemIndex + 1
			

	def pickScript(self):		
		currentItem = self.ui.lvNewActionAllScripts.currentItem()
		itemName = currentItem.text()	
		self.selectedCommands.append(itemName)	
		self.refreshPanelSelectedCommands()
		
	def dropScript(self):
		currentRow = self.ui.lvNewActionSelectedScripts.currentRow()		
		x = self.selectedCommands.pop(currentRow)		
		self.refreshPanelSelectedCommands()
	
	def isValid(self, actionid, name, actiontype):
		response = True
		if actiontype is "new":
			for action in self.runtime.listActions:
				if action.name == name:
					QMessageBox().critical(self, "Alert", "Another action called " + name + " already exists. Choose another name or delete that action first.")
					response = False
					break
		if actiontype is "update":
			for action in self.runtime.listActions:
				if action.rowID != actionid and action.name == name:
					QMessageBox().critical(self, "Alert", "Another action called " + name + " already exists. Choose another name or delete that action first.")
					response = False
					break
		if ";" in name: 
			QMessageBox().critical(self, "Alert", "An action name cannot contain a semi-column.")
			response = False
		if name == "": 
			QMessageBox().critical(self, "Alert", "An action must have a name.")
			response = False
		try: 
			xxx = str(name).decode('ascii')
		except:
			QMessageBox().critical(self, "Alert", "The name of the action contains illegal character. Please stick to Unicode.")
			response = False
		return response
	
	def returnAction(self, callback, actiontype):
		if (actiontype == "new"):
			name = self.ui.txtNewActionName.text()
			color = self.actionColor
			commandlist = []
			for index in range(self.ui.lvNewActionSelectedScripts.count()):				
				currentItem = str(self.ui.lvNewActionSelectedScripts.item(index).text())				
				commandlist.append(currentItem)
			description = ""
			if self.isValid(-1, name, actiontype):	
				self.callback(name, commandlist, color, description)
				self.close()
		elif (actiontype == "update"):
			actionid = self.rowID
			name = self.ui.txtNewActionName.text()
			color = self.actionColor
			commandlist = []
			for index in range(self.ui.lvNewActionSelectedScripts.count()):
				currentItem = str(self.ui.lvNewActionSelectedScripts.item(index).text())				
				commandlist.append(currentItem)				
			description = ""
			if self.isValid(actionid, name, actiontype):
				self.callback(actionid, name, commandlist, color, description)
				self.close()
				