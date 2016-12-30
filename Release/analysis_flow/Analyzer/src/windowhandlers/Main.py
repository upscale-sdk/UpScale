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


from __future__ import print_function

import sys
import re
import images_rc
import CodeSyntax

from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *

from wMain import Ui_MainWindow
from AddVariable import windowAddVariable
from AddAction import windowAddAction
from wDragAndDropButton import DragAndDropButton


from RuntimeData import *

from CommandR import *
from CommandSSH import *
from CommandSFTP import *
from CommandShell import *
from CommandPython import *
from CommandQuickTrace import *

from Action import Action
from Variable import Variable

from Options import *

import xml.etree.ElementTree as ET
import ntpath # to extract the basename from a path

class mainWindow(QMainWindow):    
    def __init__(self):
        QMainWindow.__init__(self)
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        
        # Go Full screen
        
        #self.showFullScreen()
        self.showMaximized()
        
        self.setWindowTitle('QuickTrace')
        self.setWindowIcon(QtGui.QIcon(':/logo2.ico'))
        self.runtime = RuntimeData()          
        self.currentInterface = ""
        self.ui.btnNewVariable.clicked.connect(self.newVariable)
        self.ui.btnNewAction.clicked.connect(self.newAction)
        self.ui.lvCommands.itemClicked.connect(self.displayCommand)        
        #self.ui.lvCommands.itemDoubleClicked.connect(self.playCommand)
        self.ui.txtCommandName.textChanged.connect(self.updateCommand)
        self.ui.cbCommandType.currentIndexChanged.connect(self.updateCommandType)
        self.ui.cbCommandOperation.currentIndexChanged.connect(self.updateCommand)        
        self.ui.txtCommandDescription.textChanged.connect(self.updateCommand)
        self.ui.chkbCommandStop.stateChanged.connect(self.updateCommand)
        self.ui.txtCommandCode.textChanged.connect(self.updateCommand)
        
        self.ui.btnCommandDelete.clicked.connect(self.deleteCommand)
        self.ui.btnCommandNew.clicked.connect(self.newCommand)        
        
        self.ui.txtOptionRLocation.textChanged.connect(self.updateRLocation)
        self.ui.txtOptionPythonLocation.textChanged.connect(self.updatePythonLocation)
        
        # Set the scroll area of the variable to accept the drag and drop (to swap variable positions)
        self.ui.scrollAreaWidgetContents.setAcceptDrops(True)
        self.ui.scrollAreaWidgetContents.installEventFilter(self)                
        
        # Set the scroll area of the actions to accept the drag and drop (to swap action positions)
        self.ui.scrollAreaWidgetContents_2.setAcceptDrops(True)
        self.ui.scrollAreaWidgetContents_2.installEventFilter(self)
        
        # set up the QListWidget with the commands to make it "drag and drop" capable
        self.ui.lvCommands.setAcceptDrops(True)
        self.ui.lvCommands.setDragEnabled(True)
        self.ui.lvCommands.setSelectionMode(QAbstractItemView.SingleSelection)
        self.ui.lvCommands.setDropIndicatorShown(True)
        self.ui.lvCommands.setDragDropMode(QAbstractItemView.InternalMove)
        self.ui.lvCommands.viewport().installEventFilter(self)

        # Give the 1000th row of the variable list the ability to stretch. This pushes all the other variables to the top of the layout        
        self.ui.gridLayout_4.setRowStretch(1000,1)
        # Give the 1000th row of the action list the ability to stretch. This pushes all the other actions to the top of the layout
        self.ui.gridLayout_6.setRowStretch(1000,1)
        
        font = QtGui.QFont()
        font.setPointSize(Options.Global_Font_Size)        
        
        newFile = QAction(QIcon(":/new.ico"), 'New Interface', self)
        newFile.setShortcut('Ctrl+N')
        newFile.setStatusTip('Create a new interface')
        newFile.setFont(font)
        newFile.triggered.connect(self.newInterface)
                
        openFile = QAction(QIcon(":/open.png"), 'Open Interface', self)
        openFile.setShortcut('Ctrl+O')
        openFile.setStatusTip('Open an interface')
        openFile.setFont(font)
        openFile.triggered.connect(self.loadInterface)
        
        saveFile = QAction(QIcon(":/save.png"), 'Save Interface', self)
        saveFile.setShortcut('Ctrl+S')
        saveFile.setStatusTip('Save the current interface')
        saveFile.setFont(font)
        saveFile.triggered.connect(lambda: self.saveInterface(self.currentInterface))
        
        saveAsFile = QAction(QIcon(":/saveas.ico"), 'Save Interface As ...', self)
        saveAsFile.setShortcut('Shift+Ctrl+S')
        saveAsFile.setStatusTip('Save the current interface as ...')
        saveAsFile.setFont(font)
        saveAsFile.triggered.connect(lambda: self.saveInterface(""))        
        
        menubar = self.menuBar()
        fileMenu = menubar.addMenu('&File')        
        fileMenu.addAction(newFile)
        fileMenu.addAction(openFile)
        fileMenu.addAction(saveFile)
        fileMenu.addAction(saveAsFile)  
                
        self.ui.mainTabs.setCurrentIndex(1)

        self.resetFontSize(Options.Global_Font_Size)
    
    def updateRLocation(self):
        Options.R_Location = self.ui.txtOptionRLocation.text()
    
    def updatePythonLocation(self):
        Options.Python_Location = self.ui.txtOptionPythonLocation.text()        
        
    def resetFontSize(self, size):        
        font = self.ui.btnCommandNew.font()
        font.setPointSize(size)
        self.ui.labActionCommandsHeader.setFont(font)
        self.ui.labCommandAlertMsg.setFont(font)
        self.ui.labCommandCodeHeader.setFont(font)
        self.ui.labCommandDescription.setFont(font)
        self.ui.labCommandListHeader.setFont(font)
        self.ui.labCommandName.setFont(font)
        self.ui.labCommandOperation.setFont(font)
        self.ui.labCommandType.setFont(font)
        self.ui.labOptionRLocation.setFont(font)
        self.ui.labActionListHeader.setFont(font)
        self.ui.chkbCommandStop.setFont(font)
        self.ui.groupCommandInformation.setFont(font)        
        self.ui.mainTabs.setFont(font)
        self.ui.btnCommandNew.setFont(font)
        self.ui.btnCommandDelete.setFont(font)
        self.ui.btnNewAction.setFont(font)
        self.ui.btnNewVariable.setFont(font)
        self.ui.lvActionCommands.setFont(font)
        self.ui.lvCommands.setFont(font)
        self.ui.cbCommandOperation.setFont(font)
        self.ui.cbCommandType.setFont(font)
        self.ui.labActionProgress.setFont(font)
        self.ui.txtActionTerminal.setFont(font)
        self.ui.txtCommandCode.setFont(font)
        self.ui.txtCommandDescription.setFont(font)
        self.ui.txtCommandName.setFont(font)
        self.ui.labOptionRLocation.setFont(font)
        self.ui.txtOptionRLocation.setFont(font)
        self.ui.labOptionPythonLocation.setFont(font)
        self.ui.txtOptionPythonLocation.setFont(font)
        
    def playCommand(self):  
        name = self.ui.lvCommands.currentItem().text()
        self.runtime.selectedCommand = self.ui.lvCommands.currentRow()
        for command in self.runtime.listCommands:
            if (command.name == name):                        
                if isinstance(command, CommandR) is True:
                    self.ui.txtActionTerminal.setText("")
                    self.ui.mainTabs.setCurrentIndex(3)
                    self.executeCommand(command)
                    self.statusBar().showMessage('Command "' + name + '" played.')                    
                break                
        
    def updateCommandType(self):
        self.ui.cbCommandOperation.clear()
        selectedItem = self.ui.cbCommandType.currentText() 
        if selectedItem == "SSH":
            self.ui.cbCommandOperation.setEnabled(True)
            self.ui.labCommandOperation.setEnabled(True)
            self.ui.cbCommandOperation.addItem("Remote Command[s]")
            self.ui.cbCommandOperation.addItem("Open")
            self.ui.cbCommandOperation.addItem("Close")            
            foreground, background = CommandSSH.display()
        elif selectedItem == "SFTP":
            self.ui.cbCommandOperation.setEnabled(True)
            self.ui.labCommandOperation.setEnabled(True)            
            self.ui.cbCommandOperation.addItem("Open")
            self.ui.cbCommandOperation.addItem("Close")
            self.ui.cbCommandOperation.addItem("Get")
            self.ui.cbCommandOperation.addItem("Put")
            foreground, background = CommandSFTP.display()        
        elif selectedItem == "R":
            self.ui.cbCommandOperation.setEnabled(False)
            self.ui.labCommandOperation.setEnabled(False)
            foreground, background = CommandR.display()
        elif selectedItem == "Shell":
            self.ui.cbCommandOperation.setEnabled(False)
            self.ui.labCommandOperation.setEnabled(False)
            foreground, background = CommandShell.display()
        elif selectedItem == "Python":
            self.ui.cbCommandOperation.setEnabled(False)
            self.ui.labCommandOperation.setEnabled(False)
            foreground, background = CommandPython.display()
        elif selectedItem == "QuickTrace":
            self.ui.cbCommandOperation.setEnabled(True)
            self.ui.labCommandOperation.setEnabled(True)            
            self.ui.cbCommandOperation.addItem("Set Variable[s]")
            self.ui.cbCommandOperation.addItem("Increment Variable")
            self.ui.cbCommandOperation.addItem("If xxx then GoTo")            
            foreground, background = CommandQuickTrace.display()        
        cmdName = self.ui.txtCommandName.text()        
        for index in range(self.ui.lvCommands.count()):                    
            line = self.ui.lvCommands.item(index)
            if line.text() == cmdName:                    
                line.setForeground(foreground)
                line.setBackground(background)        
                break            
        # Set the foreground, background of the name input fields        
        self.runtime.updateCommandActive = False                
        foreR, foreG, foreB, _ = foreground.color().getRgb()
        backR, backG, backB, _ = background.color().getRgb()
        self.ui.txtCommandName.setStyleSheet("background-color: rgb(" + str(backR) + ", " + str(backG) + ", "    + str(backB) + "); color: rgb(" + str(foreR) + ", " + str(foreG) + ", "    + str(foreB) + ");")
        self.runtime.updateCommandActive = True
        self.updateCommand()
                
    def eventFilter(self, source, event):
        if (event.type() == QtCore.QEvent.Drop and source is self.ui.lvCommands.viewport()):
            # get index to insert at
            insertPos = event.pos()
            fromList = event.source() # is QListWidget = self.ui.lvCommands, source is QWidget = self.ui.lvCommands.viewport()            
            toRow = fromList.row(fromList.itemAt(insertPos))
            cmdName = self.ui.lvCommands.currentItem().text()
            fromRow = self.ui.lvCommands.currentRow()
            if fromRow < toRow: toRow = toRow + 1
            else: fromRow = fromRow + 1
            # get the command corresponding to that command name
            cmdToBeMoved=None
            for command in self.runtime.listCommands:
                if command.name == cmdName:
                    cmdToBeMoved = command
                    break       
            # Move the command in the model (the command list)              
            self.runtime.listCommands.insert(toRow, cmdToBeMoved)
            self.runtime.listCommands.pop(fromRow)            
            # reload the list of commands
            self.ui.lvCommands.clear()
            indexItem = 0
            for command in self.runtime.listCommands:                
                self.ui.lvCommands.insertItem(indexItem, command.name)
                line = self.ui.lvCommands.item(indexItem)
                foreground, background = command.display()
                line.setForeground(foreground)
                line.setBackground(background)
                indexItem = indexItem + 1            
            self.statusBar().showMessage('Command "' + cmdName + '" moved.')
            return True

        elif (event.type() == QtCore.QEvent.DragEnter and source is self.ui.scrollAreaWidgetContents):
            # Accept to maintain the drag and drop operation in progress when moving over the scroll area with the variables
            event.accept()
            
        elif (event.type() == QtCore.QEvent.Drop and source is self.ui.scrollAreaWidgetContents):
            # Swap the variables             
            varname_src = ""
            varname_dest = ""
            dragbtn_src = event.source() # object to be dragged
            insertPos = event.pos() # Coordinates at which the dropping takes place
            # retrieve the name of the dragged variable 
            for row in range(1, len(self.runtime.listVariables) + 1):      
                dragbtn = self.ui.gridLayout_4.itemAtPosition(row, 0).widget()
                if dragbtn.objectName() == dragbtn_src.objectName():                    
                    varname_src = self.ui.gridLayout_4.itemAtPosition(row, 4).widget().text()                    
                    break
            # get the name of the variable on which the source variable is dropped
            for row in range(1, len(self.runtime.listVariables) + 1):            
                item = self.ui.gridLayout_4.itemAtPosition(row, 0).widget()
                position = item.pos()
                if (position.y() < insertPos.y() and position.y() + 35 > insertPos.y()):
                    varname_dest = self.ui.gridLayout_4.itemAtPosition(row, 4).widget().text()                    
                    break
            # Return if the user has tried to drop the object not over another variable (but in between)
            if (varname_src is "" or varname_dest is ""): return            
            self.runtime.swapVariablesPosition(varname_src, varname_dest)
            self.refreshVariablePanel()
            self.statusBar().showMessage('Variable "' + varname_src + '" moved.')                    
            return True  
        
        elif (event.type() == QtCore.QEvent.DragEnter and source is self.ui.scrollAreaWidgetContents_2):
            # Accept to maintain the drag and drop operation in progress when moving over the scroll area with the variables
            event.accept()
            
        elif (event.type() == QtCore.QEvent.Drop and source is self.ui.scrollAreaWidgetContents_2):
            # Swap the variables             
            acname_src = ""
            acname_dest = ""
            dragbtn_src = event.source() # object to be dragged            
            insertPos = event.pos() # Coordinates at which the dropping takes place
            # retrieve the name of the dragged action 
            for row in range(1, len(self.runtime.listActions) + 1):      
                dragbtn = self.ui.gridLayout_6.itemAtPosition(row, 0).widget()                
                if dragbtn.objectName() == dragbtn_src.objectName():                    
                    acname_src = self.ui.gridLayout_6.itemAtPosition(row, 5).widget().text()                    
                    break
            # get the name of the action on which the source action is dropped
            for row in range(1, len(self.runtime.listActions) + 1):            
                item = self.ui.gridLayout_6.itemAtPosition(row, 0).widget()
                position = item.pos()
                if (position.y() < insertPos.y() and position.y() + 35 > insertPos.y()):
                    acname_dest = self.ui.gridLayout_6.itemAtPosition(row, 5).widget().text()                    
                    break
            # Return if the user has tried to drop the object not over another action (but in between)
            if (acname_src is "" or acname_dest is ""): return  
            self.runtime.swapActionsPosition(acname_src, acname_dest)
            self.refreshActionPanel()
            self.statusBar().showMessage('Action "' + acname_src + '" moved.')                    
            return True 
        
        return QMainWindow.eventFilter(self, source, event)    
    
    def refreshActionPanel(self):        
        for row in reversed(range(self.ui.gridLayout_6.rowCount())):
            for column in reversed(range(self.ui.gridLayout_6.columnCount())):
                layout = self.ui.gridLayout_6.itemAtPosition(row, column)
                if layout is not None:
                    self.ui.gridLayout_6.removeItem(layout) 
                    layout.widget().setParent(None)                        
        # re-display the actions
        self.runtime.actionRowID = 1
        for action in self.runtime.listActions:
            action.rowID = self.runtime.actionRowID
            self.AddActionToPanel(action)
            
    def refreshVariablePanel(self):        
        for row in reversed(range(self.ui.gridLayout_4.rowCount())):
            for column in reversed(range(self.ui.gridLayout_4.columnCount())):
                layout = self.ui.gridLayout_4.itemAtPosition(row, column)
                if layout is not None:
                    self.ui.gridLayout_4.removeItem(layout) 
                    layout.widget().setParent(None)                       
                        
        # re-display the variables
        self.runtime.varRowID = 1
        for variable in self.runtime.listVariables:
            variable.rowID = self.runtime.varRowID
            self.AddVariableToPanel(variable)
        
    def newAction(self):        
        winAddAction = windowAddAction(self.newActionCallBack, "new", self.runtime)
        winAddAction.show()
    
    def newActionCallBack(self, name, commandlist, color, description):
        action = Action(self.runtime.actionRowID, name, commandlist, color, description)
        self.runtime.listActions.append(action)    
        self.refreshActionPanel()        
        self.statusBar().showMessage('Action "' + name + '" created.')
    
    def newCommand(self):                   
        # Create the new command
        name = " -- New command -- "
        found = True
        cpt = 1
        while found is True:
            found = False
            for command in self.runtime.listCommands:
                if command.name == name:
                    found = True
                    cpt = cpt + 1
                    name = " -- New command -- (" + str(cpt) + ")"  
                    break        
        description = ""
        code = ""
        newCmd = CommandShell(name, description, code)
        self.runtime.listCommands.append(newCmd)
        # Reset the text fields
        self.runtime.updateCommandActive = False
        self.runtime.selectedCommand = None
        self.ui.txtCommandName.setText(name)
        self.ui.txtCommandDescription.setText("")
        self.ui.txtCommandCode.setPlainText("")
        self.ui.cbCommandType.setCurrentIndex(0) # 0 = Shell, 1 = SSH, 2 = SFTP, 3 = R, 4 = QuickTrace 
        self.ui.cbCommandOperation.clear()
        self.ui.cbCommandOperation.setEnabled(False)
        self.ui.labCommandOperation.setEnabled(False)
        self.runtime.updateCommandActive = True        
        # Set the foreground, background of the name input fields        
        foreground, background = CommandShell.display()
        foreR, foreG, foreB, _ = foreground.color().getRgb()
        backR, backG, backB, _ = background.color().getRgb()
        self.ui.txtCommandName.setStyleSheet("background-color: rgb(" + str(backR) + ", " + str(backG) + ", "    + str(backB) + "); color: rgb(" + str(foreR) + ", " + str(foreG) + ", "    + str(foreB) + ");")                                
        # Add and Select the new command
        self.addItemToListWidget(self.ui.lvCommands, newCmd)                
        self.ui.lvCommands.setCurrentRow(self.ui.lvCommands.count() - 1)
        self.runtime.selectedCommand = self.ui.lvCommands.currentRow()
        self.updateCommand()
        self.statusBar().showMessage('New command created.')
            
    def deleteCommand(self):
        cmdDeleted = False
        cmdName = self.ui.txtCommandName.text()
        reply = QMessageBox.question(self, 'Confirm deletion', "Are you sure that you want to delete the command: " + cmdName + "?", QMessageBox.Yes | QMessageBox.No, QMessageBox.No)        
        if reply == QMessageBox.Yes:                       
            for command in self.runtime.listCommands:            
                if (command.name == cmdName):                        
                    deleted = self.runtime.deleteCommand(cmdName)
                    if (deleted == False): 
                        print("problem when deleting command from the runtime data structure")
                        print(cmdName)                
                    for index in range(self.ui.lvCommands.count()):
                        if (self.ui.lvCommands.item(index).text() == cmdName): 
                            self.ui.lvCommands.takeItem(index)
                            cmdDeleted = True
                            break        
            if cmdDeleted is True:             
                # Clear all the command fields        
                self.runtime.updateCommandActive = False
                self.runtime.selectedCommand = None
                self.ui.txtCommandName.setText("")
                self.ui.txtCommandDescription.setText("")
                self.ui.txtCommandCode.setPlainText("")
                self.runtime.updateCommandActive = True
                self.statusBar().showMessage('Command "' + cmdName + '" deleted.')    
    
    def updateCommand(self):            
        if self.runtime.updateCommandActive == True and self.runtime.selectedCommand != None and self.runtime.selectedCommand >= 0 and self.runtime.selectedCommand < len(self.runtime.listCommands):
            new_name = self.ui.txtCommandName.text()
            cmdModified = self.runtime.listCommands[self.runtime.selectedCommand]
            previous_name = cmdModified.name
            del self.runtime.listCommands[self.runtime.selectedCommand]            
            if self.ui.cbCommandType.currentText() == "SSH":
                newCmd = CommandSSH(new_name, self.ui.cbCommandOperation.currentText(), self.ui.txtCommandDescription.toPlainText(), self.ui.txtCommandCode.toPlainText(), self.ui.chkbCommandStop.isChecked())
            elif self.ui.cbCommandType.currentText() == "SFTP":
                newCmd = CommandSFTP(new_name, self.ui.cbCommandOperation.currentText(), self.ui.txtCommandDescription.toPlainText(), self.ui.txtCommandCode.toPlainText(), self.ui.chkbCommandStop.isChecked())
            elif self.ui.cbCommandType.currentText() == "R":
                newCmd = CommandR(new_name, self.ui.cbCommandOperation.currentText(), self.ui.txtCommandDescription.toPlainText(), self.ui.txtCommandCode.toPlainText(), self.ui.chkbCommandStop.isChecked())
            elif self.ui.cbCommandType.currentText() == "Shell":
                newCmd = CommandShell(new_name, self.ui.cbCommandOperation.currentText(), self.ui.txtCommandDescription.toPlainText(), self.ui.txtCommandCode.toPlainText(), self.ui.chkbCommandStop.isChecked())
            elif self.ui.cbCommandType.currentText() == "Python":
                newCmd = CommandPython(new_name, self.ui.cbCommandOperation.currentText(), self.ui.txtCommandDescription.toPlainText(), self.ui.txtCommandCode.toPlainText(), self.ui.chkbCommandStop.isChecked())                                                  
            elif self.ui.cbCommandType.currentText() == "QuickTrace":
                newCmd = CommandQuickTrace(new_name, self.ui.cbCommandOperation.currentText(), self.ui.txtCommandDescription.toPlainText(), self.ui.txtCommandCode.toPlainText(), self.ui.chkbCommandStop.isChecked())
            self.runtime.listCommands.insert(self.runtime.selectedCommand, newCmd)    
            # If the command name has been changed then the list lvCommand must be refreshed.
            if (new_name != previous_name):            
                line = self.ui.lvCommands.item(self.runtime.selectedCommand)
                line.setText(new_name)
                line.setFlags(line.flags() or Qt.ItemIsEditable)
                foreground, background = cmdModified.display()                        
                line.setForeground(foreground)
                line.setBackground(background)
            # Check the validity of the fields and the syntax
            self.ui.labCommandAlertMsg.setText("")
            error = False
            if "," in new_name: 
                self.ui.labCommandAlertMsg.setText("A command name cannot contain a comma!")
                error = True
            if error is False:
                if not newCmd.syntax.match(newCmd.code):
                    self.ui.labCommandAlertMsg.setText(newCmd.syntaxErrormsg)                    
                    error = True
            self.statusBar().showMessage('Command "' + new_name + '" saved.')        
        
    def displayCommand(self):        
        self.runtime.updateCommandActive = False
        name = self.ui.lvCommands.currentItem().text()        
        self.runtime.selectedCommand = self.ui.lvCommands.currentRow()
        for command in self.runtime.listCommands:
            if (command.name == name):                        
                # Display the command                
                self.ui.txtCommandName.setText(str(name))
                # 0 = Shell, 1 = SSH, 2 = SFTP, 3 = R, 4 = QuickTrace 
                if isinstance(command, CommandShell) is True:                    
                    self.ui.cbCommandType.setCurrentIndex(0)  
                    _ = CodeSyntax.ShellHighlighter(self.ui.txtCommandCode.document())  
                    self.ui.txtCommandCode.show()                                                  
                elif isinstance(command, CommandSSH) is True:                    
                    self.ui.cbCommandType.setCurrentIndex(1)
                    if str(command.operation) == "Remote Command[s]": self.ui.cbCommandOperation.setCurrentIndex(0)
                    elif str(command.operation) == "Open": self.ui.cbCommandOperation.setCurrentIndex(1)
                    elif str(command.operation) == "Close": self.ui.cbCommandOperation.setCurrentIndex(2) 
                    _ = CodeSyntax.ShellHighlighter(self.ui.txtCommandCode.document())  
                    self.ui.txtCommandCode.show() 
                elif isinstance(command, CommandSFTP) is True:                    
                    self.ui.cbCommandType.setCurrentIndex(2)
                    if str(command.operation) == "Open": self.ui.cbCommandOperation.setCurrentIndex(0)
                    elif str(command.operation) == "Close": self.ui.cbCommandOperation.setCurrentIndex(1)
                    elif str(command.operation) == "Get": self.ui.cbCommandOperation.setCurrentIndex(2)
                    elif str(command.operation) == "Put": self.ui.cbCommandOperation.setCurrentIndex(3)
                    _ = CodeSyntax.ShellHighlighter(self.ui.txtCommandCode.document())  
                    self.ui.txtCommandCode.show()  
                elif isinstance(command, CommandR) is True:                    
                    self.ui.cbCommandType.setCurrentIndex(3) 
                    _ = CodeSyntax.RHighlighter(self.ui.txtCommandCode.document())  
                    self.ui.txtCommandCode.show()
                elif isinstance(command, CommandPython) is True:                    
                    self.ui.cbCommandType.setCurrentIndex(4)     
                    _ = CodeSyntax.PythonHighlighter(self.ui.txtCommandCode.document())  
                    self.ui.txtCommandCode.show()
                elif isinstance(command, CommandQuickTrace) is True:                    
                    self.ui.cbCommandType.setCurrentIndex(5)      
                    if str(command.operation) == "Set Variable[s]": self.ui.cbCommandOperation.setCurrentIndex(0)          
                    elif str(command.operation) == "Increment Variable": self.ui.cbCommandOperation.setCurrentIndex(1)
                    elif str(command.operation) == "If xxx then GoTo": self.ui.cbCommandOperation.setCurrentIndex(2)
                    _ = CodeSyntax.ShellHighlighter(self.ui.txtCommandCode.document())  
                    self.ui.txtCommandCode.show()                
                self.ui.txtCommandDescription.setText(str(command.description))
                self.ui.chkbCommandStop.setChecked(command.stoponfailure)
                self.ui.txtCommandCode.setPlainText(command.code)        
                # Set the foreground, background of the name input fields        
                foreground, background = command.display()
                foreR, foreG, foreB, _ = foreground.color().getRgb()
                backR, backG, backB, _ = background.color().getRgb()
                self.ui.txtCommandName.setStyleSheet("background-color: rgb(" + str(backR) + ", " + str(backG) + ", "    + str(backB) + "); color: rgb(" + str(foreR) + ", " + str(foreG) + ", "    + str(foreB) + ");")                
                break
        self.runtime.updateCommandActive = True
        self.updateCommand()
        self.statusBar().showMessage('Command "' + name + '" displayed.')

    def displayAction(self):
        indexItem = self.ui.gridLayout_6.indexOf(self.sender())
        (row, _, _, _) = self.ui.gridLayout_6.getItemPosition(indexItem)
        # Retrieve the current action 
        currentAction = None                       
        for action in self.runtime.listActions:
            if (action.rowID == row):
                currentAction = action
                break
        if currentAction is None: return
        # Clear the list of commands        
        self.ui.lvActionCommands.clear()   
        
        if currentAction.showActionDetails is False:
            self.ui.labActionCommandsHeader.setText("List of commands of '" + currentAction.name[:16] + " [...]' (click to unfold actions)")
            # For each command of the current action    
            for cmdName in currentAction.commandlist:                
                cmdFound = False
                # Retrieve the command
                for command in self.runtime.listCommands:
                    if command.name == cmdName:
                        cmdFound = True
                        self.addItemToListWidget(self.ui.lvActionCommands, command)                            
                        break
                # If the command wasn't found
                if cmdFound is False:                    
                    # Search in the action list
                    acFound = False
                    for ac in self.runtime.listActions:
                        if ac.name == cmdName:
                            # The action is found
                            acFound = True                            
                            self.addItemToListWidget(self.ui.lvActionCommands, ac)
                            break                
                    # If the action wasn't found
                    if acFound is False:
                        # Create and display a fake "error" command
                        self.addItemToListWidget(self.ui.lvActionCommands, Command("### ERROR: command or action not found: '" + cmdName + "'"))
        else:
            self.ui.labActionCommandsHeader.setText("List of commands of '" + currentAction.name[:16] + " [...]' (click to fold actions)")
            # unroll all the actions within the action to be executed        
            tempList = currentAction.commandlist        
            listcommands = []
            containAction = True
            while containAction is True:
                containAction = False
                for item in tempList:
                    if item == currentAction.name:
                        self.addItemToListWidget(self.ui.lvActionCommands, Command("### ERROR: Action cannot be recursive! (or command and action cannot have the same name!)"))
                        currentAction.showActionDetails = not currentAction.showActionDetails
                        self.statusBar().showMessage('Action "' + currentAction.name + '" displayed.')
                        return
                    itemFoundAsCommand = False
                    for command in self.runtime.listCommands:
                        if command.name == item:
                            itemFoundAsCommand = True
                            listcommands.append(command.name)                            
                            break    
                    if itemFoundAsCommand is False:
                        itemFoundAsAction = False                        
                        for action in self.runtime.listActions:
                            if action.name == item:
                                itemFoundAsAction = True
                                containAction = True
                                actionToUnroll = action                                
                                break
                        if itemFoundAsAction is False:
                            self.addItemToListWidget(self.ui.lvActionCommands, Command("### ERROR: command or action not found: '" + item + "'"))
                            currentAction.showActionDetails = not currentAction.showActionDetails
                            self.statusBar().showMessage('Action "' + currentAction.name + '" displayed.')
                            return                        
                        for cmdName in actionToUnroll.commandlist:
                            listcommands.append(cmdName)
                tempList = listcommands
                listcommands = []            
            listcommands = tempList
            for cmdName in listcommands:
                for command in self.runtime.listCommands:
                    if cmdName == command.name:
                        self.addItemToListWidget(self.ui.lvActionCommands, command)
                        break
        currentAction.showActionDetails = not currentAction.showActionDetails
        self.statusBar().showMessage('Action "' + currentAction.name + '" displayed.')
        
    def AddActionToPanel(self, action):                
        if (self.runtime.actionRowID == 1):
            # add the option header
            self.ui.labOptionHeaderName = QLabel(self.ui.scrollAreaWidgetContents)
            sizePolicy = QSizePolicy(QSizePolicy.Preferred, QSizePolicy.Fixed)
            sizePolicy.setHorizontalStretch(0)
            sizePolicy.setVerticalStretch(0)
            sizePolicy.setHeightForWidth(self.ui.labOptionHeaderName.sizePolicy().hasHeightForWidth())
            self.ui.labOptionHeaderName.setSizePolicy(sizePolicy)
            self.ui.labOptionHeaderName.setMinimumSize(QtCore.QSize(0, 0))
            self.ui.labOptionHeaderName.setMaximumSize(QtCore.QSize(16777215, 25))
            font = QtGui.QFont()            
            font.setBold(True)
            font.setWeight(75)
            font.setPointSize(Options.Global_Font_Size)
            self.ui.labOptionHeaderName.setFont(font)
            self.ui.labOptionHeaderName.setAlignment(QtCore.Qt.AlignCenter)            
            self.ui.labOptionHeaderName.setText("Option")
            self.ui.labOptionHeaderName.setObjectName("labOptionHeaderName")
            self.ui.gridLayout_6.addWidget(self.ui.labOptionHeaderName, 0, 0, 1, 5, Qt.AlignTop)           
            
            # add the name header
            self.ui.labActionHeaderName = QLabel(self.ui.scrollAreaWidgetContents)
            sizePolicy = QSizePolicy(QSizePolicy.Preferred, QSizePolicy.Fixed)
            sizePolicy.setHorizontalStretch(0)
            sizePolicy.setVerticalStretch(0)
            sizePolicy.setHeightForWidth(self.ui.labActionHeaderName.sizePolicy().hasHeightForWidth())
            self.ui.labActionHeaderName.setSizePolicy(sizePolicy)
            self.ui.labActionHeaderName.setMinimumSize(QtCore.QSize(0, 0))
            self.ui.labActionHeaderName.setMaximumSize(QtCore.QSize(16777215, 25))
            font = QtGui.QFont()            
            font.setBold(True)
            font.setWeight(75)
            font.setPointSize(Options.Global_Font_Size)
            self.ui.labActionHeaderName.setFont(font)
            self.ui.labActionHeaderName.setAlignment(QtCore.Qt.AlignCenter)
            self.ui.labActionHeaderName.setText("Name")
            self.ui.labActionHeaderName.setObjectName("labActionHeaderName")
            self.ui.gridLayout_6.addWidget(self.ui.labActionHeaderName, 0, 5, Qt.AlignTop)
        
        # create the Drag and Drop button        
        self.ui.btnVarDragAndDrop = DragAndDropButton()
        sizePolicy = QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)        
        self.ui.gridLayout_6.addWidget(self.ui.btnVarDragAndDrop, self.runtime.actionRowID, 0, Qt.AlignTop)
        
        # create the delete button
        self.ui.btnActionDelete = QPushButton(self.ui.scrollAreaWidgetContents)
        sizePolicy = QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.ui.btnActionDelete.sizePolicy().hasHeightForWidth())
        self.ui.btnActionDelete.setSizePolicy(sizePolicy)
        self.ui.btnActionDelete.setMinimumSize(QtCore.QSize(35, 35))
        self.ui.btnActionDelete.setMaximumSize(QtCore.QSize(35, 35))
        self.ui.btnActionDelete.setText("")
        icon3 = QtGui.QIcon()
        icon3.addPixmap(QtGui.QPixmap(":/trash.ico"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.ui.btnActionDelete.setIcon(icon3)
        self.ui.btnActionDelete.setIconSize(QtCore.QSize(20, 20))
        self.ui.btnActionDelete.clicked.connect(self.deleteAction)
        self.ui.gridLayout_6.addWidget(self.ui.btnActionDelete, self.runtime.actionRowID, 1, Qt.AlignTop)
        
        # create the edit button
        self.ui.btnActionEdit = QPushButton(self.ui.scrollAreaWidgetContents)
        sizePolicy = QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.ui.btnActionEdit.sizePolicy().hasHeightForWidth())
        self.ui.btnActionEdit.setSizePolicy(sizePolicy)
        self.ui.btnActionEdit.setMinimumSize(QtCore.QSize(35, 35))
        self.ui.btnActionEdit.setMaximumSize(QtCore.QSize(35, 35))
        self.ui.btnActionEdit.setText("")
        icon2 = QtGui.QIcon()
        icon2.addPixmap(QtGui.QPixmap(":/pencil2.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.ui.btnActionEdit.setIcon(icon2)
        self.ui.btnActionEdit.setIconSize(QtCore.QSize(20, 20))
        self.ui.btnActionEdit.clicked.connect(self.updateAction)
        self.ui.gridLayout_6.addWidget(self.ui.btnActionEdit, self.runtime.actionRowID, 2, Qt.AlignTop)        
        
        # create the see button
        self.ui.btnActionSee = QPushButton(self.ui.scrollAreaWidgetContents)
        sizePolicy = QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.ui.btnActionSee.sizePolicy().hasHeightForWidth())
        self.ui.btnActionSee.setSizePolicy(sizePolicy)
        self.ui.btnActionSee.setMinimumSize(QtCore.QSize(35, 35))
        self.ui.btnActionSee.setMaximumSize(QtCore.QSize(35, 35))
        self.ui.btnActionSee.setText("")        
        icon1 = QtGui.QIcon()
        icon1.addPixmap(QtGui.QPixmap(":/see.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.ui.btnActionSee.setIcon(icon1)
        self.ui.btnActionSee.setIconSize(QtCore.QSize(20, 20))
        self.ui.btnActionSee.clicked.connect(self.displayAction)
        self.ui.gridLayout_6.addWidget(self.ui.btnActionSee, self.runtime.actionRowID, 3, Qt.AlignTop)
        
        # create the execute button
        self.ui.btnActionExecute = QPushButton(self.ui.scrollAreaWidgetContents)
        sizePolicy = QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.ui.btnActionExecute.sizePolicy().hasHeightForWidth())
        self.ui.btnActionExecute.setSizePolicy(sizePolicy)
        self.ui.btnActionExecute.setMinimumSize(QtCore.QSize(35, 35))
        self.ui.btnActionExecute.setMaximumSize(QtCore.QSize(35, 35))
        self.ui.btnActionExecute.setText("")        
        icon1 = QtGui.QIcon()
        icon1.addPixmap(QtGui.QPixmap(":/run.ico"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.ui.btnActionExecute.setIcon(icon1)
        self.ui.btnActionExecute.setIconSize(QtCore.QSize(20, 20))
        self.ui.btnActionExecute.clicked.connect(self.executeAction)
        self.ui.gridLayout_6.addWidget(self.ui.btnActionExecute, self.runtime.actionRowID, 4, Qt.AlignTop)
        
        # create the name label        
        self.ui.labActionName = QLabel(self.ui.scrollAreaWidgetContents)
        sizePolicy = QSizePolicy(QSizePolicy.Preferred, QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.ui.labActionName.sizePolicy().hasHeightForWidth())
        self.ui.labActionName.setMinimumSize(QtCore.QSize(0, 35))
        self.ui.labActionName.setMaximumSize(QtCore.QSize(16777215, 35))
        self.ui.labActionName.setSizePolicy(sizePolicy)        
        self.ui.labActionName.setText(str(action.name))
        self.ui.labActionName.setStyleSheet("color: " + action.color) 
        self.ui.labActionName.setAlignment(QtCore.Qt.AlignLeft) 
        font = self.ui.labActionName.font()
        font.setPointSize(Options.Global_Font_Size)
        self.ui.labActionName.setFont(font)       
        self.ui.gridLayout_6.addWidget(self.ui.labActionName, self.runtime.actionRowID, 5, Qt.AlignTop)
        
        self.ui.btnVarDragAndDrop.setObjectName("btnVarDragAndDrop_" + str(self.runtime.actionRowID))
        self.ui.btnActionDelete.setObjectName("btnActionDelete_" + str(self.runtime.actionRowID))
        self.ui.btnActionEdit.setObjectName("btnActionEdit_" + str(self.runtime.actionRowID))
        self.ui.btnActionSee.setObjectName("btnActionSee_" + str(self.runtime.actionRowID))
        self.ui.btnActionExecute.setObjectName("btnActionExecute_" + str(self.runtime.actionRowID))                
        self.ui.labActionName.setObjectName("labActionName_" + str(self.runtime.actionRowID))        
        
        self.runtime.actionRowID += 1
        
    def openInterfaceFile(self):    
        filename = QFileDialog.getOpenFileName(self, "Open Interface", 
                                                "/Users/vincent/Documents/OneDriveBusiness/Research/pWCET/toolchain/experiments/IMEM-wavefront/interface/",
                                                "Interfaces (*.xml);;All files (*)")
        self.statusBar().showMessage('Choose a file.')
        return filename
    
    def saveInterfaceFile(self, _initialdir, _title):
        filename = QFileDialog.getSaveFileName(self, "Save Interface", 
                                               "~/", 
                                               "Interfaces (*.xml);;All files (*)")
        self.statusBar().showMessage('Choose a file.')        
        return filename
    
    def deleteVariable(self):        
        indexItem = self.ui.gridLayout_4.indexOf(self.sender())
        (row, column, _, _) = self.ui.gridLayout_4.getItemPosition(indexItem)
        varName = self.ui.gridLayout_4.itemAtPosition(row, 4).widget().text()        
        reply = QMessageBox.question(self, 'Confirm deletion', "Are you sure that you want to delete the variable: " + varName + "?", QMessageBox.Yes | QMessageBox.No, QMessageBox.No)        
        if reply == QMessageBox.Yes:        
            for column in range(self.ui.gridLayout_4.columnCount()):
                # delete every widget in the row
                layout = self.ui.gridLayout_4.itemAtPosition(row, column)
                if layout is not None:
                    if (column == 4): 
                        varName = layout.widget().text()
                        deleted = self.runtime.deleteVariable(varName)
                        if (deleted == False): 
                            print("problem when deleting variable from the runtime datastructure")
                            print(varName)                    
                    layout.widget().deleteLater()
                    self.ui.gridLayout_4.removeItem(layout)
            if (len(self.runtime.listVariables) == 0):
                # delete the header
                for column in range(self.ui.gridLayout_4.columnCount()):
                    layout = self.ui.gridLayout_4.itemAtPosition(0, column)
                    if layout is not None:                
                        layout.widget().deleteLater()
                        self.ui.gridLayout_4.removeItem(layout)
            self.statusBar().showMessage('Variable "' + varName + '" deleted.')
    
    def deleteAction(self):
        indexItem = self.ui.gridLayout_6.indexOf(self.sender())
        (row, column, _, _) = self.ui.gridLayout_6.getItemPosition(indexItem)  
        acName = self.ui.gridLayout_6.itemAtPosition(row, 5).widget().text()        
        reply = QMessageBox.question(self, 'Confirm deletion', "Are you sure that you want to delete the action: " + acName + "?", QMessageBox.Yes | QMessageBox.No, QMessageBox.No)        
        if reply == QMessageBox.Yes:               
            for column in range(self.ui.gridLayout_6.columnCount()):
                # delete every widget in the row
                layout = self.ui.gridLayout_6.itemAtPosition(row, column)
                if layout is not None:
                    if (column == 5): 
                        acName = layout.widget().text()
                        deleted = self.runtime.deleteAction(acName)
                        if (deleted == False): 
                            print("problem when deleting action from the runtime datastructure")
                            print(acName)
                    layout.widget().deleteLater()
                    self.ui.gridLayout_6.removeItem(layout)
            if (len(self.runtime.listActions) == 0):
                # delete the header
                for column in range(self.ui.gridLayout_6.columnCount()):
                    layout = self.ui.gridLayout_6.itemAtPosition(0, column)
                    if layout is not None:                
                        layout.widget().deleteLater()
                        self.ui.gridLayout_6.removeItem(layout)        
            self.statusBar().showMessage('Action "' + acName + '" deleted.')
        
    def updateVariableListSelectedValue(self):
        indexItem = self.ui.gridLayout_4.indexOf(self.sender())
        (row, _, _, _) = self.ui.gridLayout_4.getItemPosition(indexItem)                    
        for variable in self.runtime.listVariables:
            if (variable.rowID == row):
                variable.selectedValue = self.sender().currentText()                
                break
    
    def updateVariable(self):
        indexItem = self.ui.gridLayout_4.indexOf(self.sender())
        (row, _, _, _) = self.ui.gridLayout_4.getItemPosition(indexItem)
        for variable in self.runtime.listVariables:
            if (variable.rowID == row):
                winAddVariable = windowAddVariable(self.updateVariableCallBack, "update", self.runtime, variable.rowID, variable.name, variable.type, variable.value, variable.color, variable.description)
                winAddVariable.show()
                break        
    
    def updateAction(self):
        indexItem = self.ui.gridLayout_6.indexOf(self.sender())
        (row, _, _, _) = self.ui.gridLayout_6.getItemPosition(indexItem)                    
        for action in self.runtime.listActions:
            if (action.rowID == row):
                winAddAction = windowAddAction(self.updateActionCallBack, "update", self.runtime, action.rowID, action.name)
                winAddAction.show()                
                break        
    
    def updateVariableCallBack(self, varid, name, vartype, value, color, description):        
        for variable in self.runtime.listVariables:
            if (variable.rowID == varid):
                variable.name = name
                variable.type = vartype
                variable.value = value
                if (vartype == "String" or vartype == "Password"): variable.selectedValue = value
                elif vartype == "List": variable.selectedValue = str(value).split(" ; ")[0]
                variable.color = color        
                variable.description = description
                # update the panel
                self.refreshVariablePanel()
                self.statusBar().showMessage('Variable "' + variable.name + '" updated.')
                break
    
    def updateActionCallBack(self, varid, name, commandlist, color, description):        
        for action in self.runtime.listActions:
            if (action.rowID == varid):
                action.name = name
                action.commandlist = commandlist
                action.color = color
                action.description = description
                # update the panel
                self.refreshActionPanel()
                self.statusBar().showMessage('Action "' + action.name + '" updated.')
                break
        
    def newVariable(self):        
        winAddVariable = windowAddVariable(self.newVariableCallBack, "new", self.runtime)
        winAddVariable.show()    
            
    def newVariableCallBack(self, name, vartype, value, color, description):
        var = Variable(self.runtime.varRowID, name, vartype, value, color, description)
        self.runtime.listVariables.append(var)                
        self.refreshVariablePanel()
        self.statusBar().showMessage('Variable "' + name + '" created.')
    
    def AddVariableToPanel(self, variable):                
        if (self.runtime.varRowID == 1):
            # add the name header
            self.ui.labVarHeaderName = QLabel()
            sizePolicy = QSizePolicy(QSizePolicy.Preferred, QSizePolicy.Fixed)
            sizePolicy.setHorizontalStretch(0)
            sizePolicy.setVerticalStretch(0)
            sizePolicy.setHeightForWidth(self.ui.labVarHeaderName.sizePolicy().hasHeightForWidth())
            self.ui.labVarHeaderName.setSizePolicy(sizePolicy)
            self.ui.labVarHeaderName.setMinimumSize(QtCore.QSize(0, 25))
            self.ui.labVarHeaderName.setMaximumSize(QtCore.QSize(16777215, 25))
            font = QtGui.QFont()
            font.setBold(True)
            font.setWeight(75)
            font.setPointSize(Options.Global_Font_Size)
            self.ui.labVarHeaderName.setFont(font)
            self.ui.labVarHeaderName.setAlignment(QtCore.Qt.AlignCenter)
            self.ui.labVarHeaderName.setText("Name")
            self.ui.labVarHeaderName.setObjectName("labVarHeaderName")
            self.ui.gridLayout_4.addWidget(self.ui.labVarHeaderName, 0, 4, Qt.AlignTop)
            # add the value header
            self.ui.labVarHeaderValue = QLabel()
            sizePolicy = QSizePolicy(QSizePolicy.Preferred, QSizePolicy.Fixed)
            sizePolicy.setHorizontalStretch(1)
            sizePolicy.setVerticalStretch(0)
            sizePolicy.setHeightForWidth(self.ui.labVarHeaderValue.sizePolicy().hasHeightForWidth())
            self.ui.labVarHeaderValue.setSizePolicy(sizePolicy)
            self.ui.labVarHeaderValue.setMinimumSize(QtCore.QSize(0, 25))
            self.ui.labVarHeaderValue.setMaximumSize(QtCore.QSize(16777215, 25))
            font = QtGui.QFont()            
            font.setBold(True)
            font.setWeight(75)
            font.setPointSize(Options.Global_Font_Size)
            self.ui.labVarHeaderValue.setFont(font)
            self.ui.labVarHeaderValue.setAlignment(QtCore.Qt.AlignCenter)
            self.ui.labVarHeaderValue.setText("Value")
            self.ui.labVarHeaderValue.setObjectName("labVarHeaderValue")
            self.ui.gridLayout_4.addWidget(self.ui.labVarHeaderValue, 0, 5, Qt.AlignTop)
                       
        # create the Drag and Drop button        
        self.ui.btnVarDragAndDrop = DragAndDropButton()
        sizePolicy = QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)        
        self.ui.gridLayout_4.addWidget(self.ui.btnVarDragAndDrop, self.runtime.varRowID, 0, Qt.AlignTop)
        
        # create the delete button
        self.ui.btnVarDelete = QPushButton()
        sizePolicy = QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.ui.btnVarDelete.sizePolicy().hasHeightForWidth())
        self.ui.btnVarDelete.setSizePolicy(sizePolicy)
        self.ui.btnVarDelete.setMinimumSize(QtCore.QSize(35, 35))
        self.ui.btnVarDelete.setMaximumSize(QtCore.QSize(35, 35))
        self.ui.btnVarDelete.setText("")
        self.ui.btnVarDelete.setToolTip("Delete this variable")
        icon3 = QtGui.QIcon()
        icon3.addPixmap(QtGui.QPixmap(":/trash.ico"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.ui.btnVarDelete.setIcon(icon3)
        self.ui.btnVarDelete.setIconSize(QtCore.QSize(20, 20))
        self.ui.btnVarDelete.clicked.connect(self.deleteVariable)
        self.ui.gridLayout_4.addWidget(self.ui.btnVarDelete, self.runtime.varRowID, 1, Qt.AlignTop)
        
        # create the edit button
        self.ui.btnVarEdit = QPushButton()
        sizePolicy = QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.ui.btnVarEdit.sizePolicy().hasHeightForWidth())
        self.ui.btnVarEdit.setSizePolicy(sizePolicy)
        self.ui.btnVarEdit.setMinimumSize(QtCore.QSize(35, 35))
        self.ui.btnVarEdit.setMaximumSize(QtCore.QSize(35, 35))
        self.ui.btnVarEdit.setText("")
        self.ui.btnVarEdit.setToolTip("Edit this variable")
        icon2 = QtGui.QIcon()
        icon2.addPixmap(QtGui.QPixmap(":/pencil2.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.ui.btnVarEdit.setIcon(icon2)
        self.ui.btnVarEdit.setIconSize(QtCore.QSize(20, 20))
        self.ui.btnVarEdit.clicked.connect(self.updateVariable)
        self.ui.gridLayout_4.addWidget(self.ui.btnVarEdit, self.runtime.varRowID, 2, Qt.AlignTop)
        
        # create the tooltip button
        self.ui.btnVarDescription = QPushButton()
        sizePolicy = QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.ui.btnVarDescription.sizePolicy().hasHeightForWidth())
        self.ui.btnVarDescription.setSizePolicy(sizePolicy)
        self.ui.btnVarDescription.setMinimumSize(QtCore.QSize(35, 35))
        self.ui.btnVarDescription.setMaximumSize(QtCore.QSize(35, 35))
        self.ui.btnVarDescription.setText("")
        self.ui.btnVarDescription.setToolTip(str(variable.description))
        icon1 = QtGui.QIcon()
        icon1.addPixmap(QtGui.QPixmap(":/tooltip.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.ui.btnVarDescription.setIcon(icon1)
        self.ui.btnVarDescription.setIconSize(QtCore.QSize(20, 20))
        self.ui.btnVarDescription.setCheckable(False)
        self.ui.btnVarDescription.setFlat(True)
        self.ui.gridLayout_4.addWidget(self.ui.btnVarDescription, self.runtime.varRowID, 3, Qt.AlignTop)        
        
        # create the name label
        self.ui.labVarName = QLabel()
        sizePolicy = QSizePolicy(QSizePolicy.Preferred, QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.ui.labVarName.sizePolicy().hasHeightForWidth())
        self.ui.labVarName.setMinimumSize(QtCore.QSize(0, 35))
        self.ui.labVarName.setMaximumSize(QtCore.QSize(16777215, 35))
        self.ui.labVarName.setSizePolicy(sizePolicy)
        self.ui.labVarName.setText(str(variable.name))        
        self.ui.labVarName.setStyleSheet("color: " + variable.color)            
        self.ui.labVarName.setAlignment(QtCore.Qt.AlignCenter)
        font = self.ui.labVarName.font()
        font.setPointSize(Options.Global_Font_Size)        
        self.ui.labVarName.setFont(font)
        self.ui.gridLayout_4.addWidget(self.ui.labVarName, self.runtime.varRowID, 4, Qt.AlignTop)
        
        # create the value lineEdit
        if variable.type == "String" or variable.type == "Password":
            self.ui.txtVarValue = QLineEdit()
            sizePolicy = QSizePolicy(QSizePolicy.Preferred, QSizePolicy.Fixed)
            sizePolicy.setHorizontalStretch(1)        
            sizePolicy.setVerticalStretch(0)
            sizePolicy.setHeightForWidth(self.ui.txtVarValue.sizePolicy().hasHeightForWidth())        
            self.ui.txtVarValue.setSizePolicy(sizePolicy)
            self.ui.txtVarValue.setMinimumSize(QtCore.QSize(0, 35))
            self.ui.txtVarValue.setMaximumSize(QtCore.QSize(16777215, 35))
            self.ui.txtVarValue.setText(str(variable.value))        
            self.ui.txtVarValue.setReadOnly(True)
            font = self.ui.txtVarValue.font()
            font.setPointSize(Options.Global_Font_Size)
            self.ui.txtVarValue.setFont(font)
            self.ui.gridLayout_4.addWidget(self.ui.txtVarValue, self.runtime.varRowID, 5, Qt.AlignTop)
            if variable.type == "Password":  self.ui.txtVarValue.setEchoMode(QLineEdit.Password)
        elif variable.type == "List":
            self.ui.txtVarValue = QComboBox()            
            sizePolicy = QSizePolicy(QSizePolicy.Preferred, QSizePolicy.Fixed)
            sizePolicy.setHorizontalStretch(1)        
            sizePolicy.setVerticalStretch(0)
            sizePolicy.setHeightForWidth(self.ui.txtVarValue.sizePolicy().hasHeightForWidth())        
            self.ui.txtVarValue.setSizePolicy(sizePolicy)
            self.ui.txtVarValue.setMinimumSize(QtCore.QSize(0, 35))
            self.ui.txtVarValue.setMaximumSize(QtCore.QSize(16777215, 35))
            listvalue = str(variable.value).split(" ; ")
            itemIndex = 0
            for value in listvalue: 
                self.ui.txtVarValue.addItem(value)
                if variable.selectedValue == value:
                    self.ui.txtVarValue.setCurrentIndex(itemIndex)
                itemIndex = itemIndex + 1                              
            #self.ui.txtVarValue.setEditable(True)
            #self.ui.txtVarValue.lineEdit().setAlignment(QtCore.Qt.AlignRight)
            #self.ui.txtVarValue.setEditable(False)
            font = self.ui.txtVarValue.font()
            font.setPointSize(Options.Global_Font_Size)
            self.ui.txtVarValue.setFont(font)
            
            self.ui.txtVarValue.currentIndexChanged.connect(self.updateVariableListSelectedValue)
            self.ui.gridLayout_4.addWidget(self.ui.txtVarValue, self.runtime.varRowID, 5, Qt.AlignTop)            
        
        self.ui.btnVarDragAndDrop.setObjectName("btnVarDragAndDrop_" + str(self.runtime.varRowID))
        self.ui.btnVarDelete.setObjectName("btnVarDelete_" + str(self.runtime.varRowID))
        self.ui.btnVarEdit.setObjectName("btnVarEdit_" + str(self.runtime.varRowID))
        self.ui.btnVarDescription.setObjectName("btnVarDescription_" + str(self.runtime.varRowID))        
        self.ui.labVarName.setObjectName("labVarName_" + str(self.runtime.varRowID))
        self.ui.txtVarValue.setObjectName("txtVarValue_" + str(self.runtime.varRowID))
    
        self.runtime.varRowID += 1
        
    def addItemToListWidget(self, listwidget, item):        
        if listwidget.objectName() == "lvActionCommands":
            # Display the line number
            listwidget.addItem(str(len(listwidget) + 1) + ". " + item.name)                    
        else:
            listwidget.addItem(item.name)        
        line = listwidget.item(len(listwidget) - 1)
        line.setFlags(line.flags() or Qt.ItemIsEditable)
        foreground, background = item.display()                        
        line.setForeground(foreground)            
        line.setBackground(background)        
    
    def newInterface(self):
        # Delete all variable lines 
        for row in range(self.ui.gridLayout_4.rowCount()):
            for column in range(self.ui.gridLayout_4.columnCount()):            
                layout = self.ui.gridLayout_4.itemAtPosition(row, column)
                if layout is not None:                        
                    if layout.widget() is not None: layout.widget().deleteLater()
                    self.ui.gridLayout_4.removeItem(layout)
        # Delete all commands
        self.ui.lvCommands.clear()
        # Delete all action lines 
        for row in range(self.ui.gridLayout_6.rowCount()):
            for column in range(self.ui.gridLayout_6.columnCount()):            
                layout = self.ui.gridLayout_6.itemAtPosition(row, column)
                if layout is not None:                        
                    if layout.widget() is not None: layout.widget().deleteLater()
                    self.ui.gridLayout_6.removeItem(layout)
        # Delete all saved data
        self.runtime.listVariables = []
        self.runtime.listCommands = []
        self.runtime.listActions = []
        self.runtime.varRowID = 1
        self.runtime.actionRowID = 1
        
        self.setWindowTitle('QuickTrace: new_interface*')
        self.currentInterface = ""
        
        self.ui.cbCommandType.setCurrentIndex(0) # 0 = Shell
        
        self.ui.cbCommandOperation.setEnabled(False)
        self.ui.labCommandOperation.setEnabled(False)
        self.ui.txtCommandName.setText("")
        self.ui.txtCommandDescription.setText("")
        self.ui.txtCommandCode.setPlainText("")        
                        
        self.statusBar().showMessage('New interface.')
            
    def loadInterface(self):                
        self.statusBar().showMessage('Loading Interface...')
        filename = self.openInterfaceFile()                        
        if (filename != ""):
            # Delete all variable lines 
            for row in range(self.ui.gridLayout_4.rowCount()):
                for column in range(self.ui.gridLayout_4.columnCount()):            
                    layout = self.ui.gridLayout_4.itemAtPosition(row, column)
                    if layout is not None:                        
                        if layout.widget() is not None: layout.widget().deleteLater()
                        self.ui.gridLayout_4.removeItem(layout)
            # Delete all commands
            self.ui.lvCommands.clear()
            # Delete all action lines 
            for row in range(self.ui.gridLayout_6.rowCount()):
                for column in range(self.ui.gridLayout_6.columnCount()):            
                    layout = self.ui.gridLayout_6.itemAtPosition(row, column)
                    if layout is not None:                        
                        if layout.widget() is not None: layout.widget().deleteLater()
                        self.ui.gridLayout_6.removeItem(layout)
            # Delete all saved data
            self.runtime.listVariables = []
            self.runtime.listCommands = []
            self.runtime.listActions = []
            self.runtime.varRowID = 1
            self.runtime.actionRowID = 1
            # parse the XML file
            tree = ET.parse(filename)
            root = tree.getroot() 
            for options in root.findall(XMLTagDictionary.KwOption): #There should be only one
                Options.R_Location = options.get(XMLTagDictionary.KwOptionRLocation)                 
                self.ui.txtOptionRLocation.setText(str(Options.R_Location))
                Options.Python_Location = options.get(XMLTagDictionary.KwOptionPythonLocation)                
                self.ui.txtOptionPythonLocation.setText(str(Options.Python_Location))                                                    
            for variable in root.findall(XMLTagDictionary.KwVariable):                
                varName = variable.get(XMLTagDictionary.KwVarName)
                varType = variable.get(XMLTagDictionary.KwVarType)
                varValue = variable.get(XMLTagDictionary.KwVarValue)
                varSelectedValue = variable.get(XMLTagDictionary.KwVarSelectedValue)
                varColor = variable.get(XMLTagDictionary.KwVarColor)                
                varDescription = variable.get(XMLTagDictionary.KwVarDescription)            
                var = Variable(self.runtime.varRowID, varName, varType, varValue, varColor, varDescription)
                var.selectedValue = varSelectedValue                
                self.runtime.listVariables.append(var)  
                self.AddVariableToPanel(var)                                     
            for command in root.findall(XMLTagDictionary.KwCommand):
                cmdName = command.get(XMLTagDictionary.KwCmdName)
                cmdType = command.get(XMLTagDictionary.KwCmdType)                                      
                cmdOperation = command.get(XMLTagDictionary.KwCmdOperation)
                cmdDescription = command.get(XMLTagDictionary.KwCmdDescription)
                cmdStopOnFailure = command.get(XMLTagDictionary.KwCmdStopOnFailure)
                bool_StopOnFailure = (cmdStopOnFailure == "True")
                cmdCode = command.text
                if not cmdCode: cmdCode = ""                
                if cmdType == "SSH":
                    cmd = CommandSSH(cmdName, cmdOperation, cmdDescription, cmdCode, bool_StopOnFailure)
                elif cmdType == "SFTP":
                    cmd = CommandSFTP(cmdName, cmdOperation, cmdDescription, cmdCode, bool_StopOnFailure)
                elif cmdType == "R":
                    cmd = CommandR(cmdName, cmdOperation, cmdDescription, cmdCode, bool_StopOnFailure)
                elif cmdType == "Shell":
                    cmd = CommandShell(cmdName, cmdOperation, cmdDescription, cmdCode, bool_StopOnFailure)
                elif cmdType == "Python":
                    cmd = CommandPython(cmdName, cmdOperation, cmdDescription, cmdCode, bool_StopOnFailure)
                elif cmdType == "QuickTrace":
                    cmd = CommandQuickTrace(cmdName, cmdOperation, cmdDescription, cmdCode, bool_StopOnFailure)                
                self.runtime.listCommands.append(cmd)
                self.addItemToListWidget(self.ui.lvCommands, cmd)
            for action in root.findall(XMLTagDictionary.KwAction):
                acName = action.get(XMLTagDictionary.KwAcName)
                acColor = action.get(XMLTagDictionary.KwAcColor)
                acCommandlist = str(action.text).split(", ")
                ac = Action(self.runtime.actionRowID, acName, acCommandlist, acColor)
                self.runtime.listActions.append(ac)
                self.AddActionToPanel(ac)                        
            self.setWindowTitle('QuickTrace: ' + filename)            
            self.currentInterface = filename
            self.ui.mainTabs.setCurrentIndex(0)
            self.statusBar().showMessage('Interface loaded.')
            
    def saveInterface(self, filename):        
        if filename == "": filename = self.saveInterfaceFile("~/", "Save as")
        else: filename = self.currentInterface
        if (filename != ""):            
            root = ET.Element(XMLTagDictionary.KwToolchain, {})
            options = ET.Element(XMLTagDictionary.KwOption)
            options.set(XMLTagDictionary.KwOptionRLocation, str(Options.R_Location))
            options.set(XMLTagDictionary.KwOptionPythonLocation, str(Options.Python_Location))
            root.append(options)            
            for variable in self.runtime.listVariables:            
                var = ET.Element(XMLTagDictionary.KwVariable)
                var.set(XMLTagDictionary.KwVarName, str(variable.name))
                var.set(XMLTagDictionary.KwVarType, str(variable.type))
                var.set(XMLTagDictionary.KwVarValue, str(variable.value))
                if str(variable.selectedValue) != "": var.set(XMLTagDictionary.KwVarSelectedValue, str(variable.selectedValue))
                else: var.set(XMLTagDictionary.KwVarSelectedValue, str(variable.value))
                var.set(XMLTagDictionary.KwVarColor, str(variable.color))
                var.set(XMLTagDictionary.KwVarDescription, str(variable.description))    
                root.append(var)
            for command in self.runtime.listCommands:
                cmd = ET.Element(XMLTagDictionary.KwCommand)
                cmd.set(XMLTagDictionary.KwCmdName, str(command.name))
                if isinstance(command, CommandSSH) is True: cmd.set(XMLTagDictionary.KwCmdType, "SSH")
                elif isinstance(command, CommandSFTP) is True: cmd.set(XMLTagDictionary.KwCmdType, "SFTP")    
                elif isinstance(command, CommandR) is True: cmd.set(XMLTagDictionary.KwCmdType, "R")
                elif isinstance(command, CommandShell) is True: cmd.set(XMLTagDictionary.KwCmdType, "Shell")
                elif isinstance(command, CommandPython) is True: cmd.set(XMLTagDictionary.KwCmdType, "Python")
                elif isinstance(command, CommandQuickTrace) is True: cmd.set(XMLTagDictionary.KwCmdType, "QuickTrace")
                cmd.set(XMLTagDictionary.KwCmdOperation, str(command.operation))                
                cmd.set(XMLTagDictionary.KwCmdDescription, str(command.description))
                cmd.set(XMLTagDictionary.KwCmdStopOnFailure, str(command.stoponfailure))
                cmd.text = str(command.code)                
                root.append(cmd)
            for action in self.runtime.listActions:
                ac = ET.Element(XMLTagDictionary.KwAction)
                if (str(action.name) != ""): ac.set(XMLTagDictionary.KwAcName, str(action.name))
                if (str(action.commandlist) != ""):                     
                    ac.text = str(action.commandlist).strip('[]')                    
                    ac.text = ac.text.replace('\'', '')
                ac.set(XMLTagDictionary.KwAcColor, str(action.color))                    
                root.append(ac)
            tree = ET.ElementTree(root)
            tree.write(filename, xml_declaration=True, method="xml")            
            self.setWindowTitle('QuickTrace: ' + filename)
            self.currentInterface = filename
            self.statusBar().showMessage('Interface saved.')
            
    def executeAction(self):
        self.ui.mainTabs.setCurrentIndex(3)   
        qApp.processEvents()  # Used to refresh the GUI               
        self.ui.txtActionTerminal.setText("")
        self.ui.labActionProgress.setText("")        
        # Get the action that must be executed
        indexItem = self.ui.gridLayout_6.indexOf(self.sender())
        (row, _, _, _) = self.ui.gridLayout_6.getItemPosition(indexItem)                
        actionToBeExecuted = ""
        for action in self.runtime.listActions:
            if (action.rowID == row):                
                actionToBeExecuted = action
                break
        if actionToBeExecuted == "": return False
        self.ui.barActionProgress.setMinimum(0)
        self.ui.barActionProgress.setValue(0)
        self.ui.barActionProgress.setMaximum(actionToBeExecuted.numberOfCommands(self.runtime) - 1)        
        self.statusBar().showMessage('Action "' + actionToBeExecuted.name + '" is executing...')
        actionToBeExecuted.execute(self.runtime, self.executeCommand)        
        self.statusBar().showMessage('Action "' + actionToBeExecuted.name + '" executed.') 
        self.ui.labActionProgress.setText("Done!")   

        
    def executeCommand(self, command):        
        ExecutionSuccess = True
        self.ui.labActionProgress.setText("Running: " + command.name)        
        self.PrintHeader(" Execute the command '" + command.name + "'")            
        self.statusBar().showMessage('Command "' + command.name + '" is executing...')        
        # Replace all the variables    for their values        
        command.decode(self.runtime.listVariables)
        #if not isinstance(command, CommandR):                
        self.printOnTerminal("|> Code: ", "comment")
        self.printOnTerminal(command.displayCode(), "command")            
        if command.description != "":
            self.printOnTerminal("|> Description: ", "comment")
            self.printOnTerminal(command.description, "comment")    
        errormsg, responsemsg = command.execute(self.runtime)  
        command.resetCode()             
        if responsemsg != "": self.printOnTerminal("|> Response: " + responsemsg, "response")
        if errormsg != "": 
            self.printOnTerminal(errormsg, "alert")    
            ExecutionSuccess = False
        else: self.printOnTerminal("|> Command '" + command.name + "' completed.", "success")            
        self.statusBar().showMessage('Command "' + command.name + '" executed.')
        self.ui.barActionProgress.setValue(self.ui.barActionProgress.value() + 1)
        qApp.processEvents()  # Used to refresh the GUI
        return ExecutionSuccess
        
    def PrintHeader(self, text):
        self.printOnTerminal(" ", "normal")
        self.printOnTerminal("------------------------------------------------", "normal")
        self.printOnTerminal("| " + text, "normal")
        self.printOnTerminal("------------------------------------------------", "normal")
        
    def printOnTerminal(self, msg, msgtype):
        if (msgtype == "alert"):
            self.ui.txtActionTerminal.setTextColor(QColor("blue"))
        elif (msgtype == "success"):
            self.ui.txtActionTerminal.setTextColor(QColor(0,111,0))            
        elif (msgtype == "command"):
            self.ui.txtActionTerminal.setTextColor(QColor(164,90,42))            
        elif (msgtype == "response"):
            self.ui.txtActionTerminal.setTextColor(QColor("black"))
        elif (msgtype == "comment"):
            self.ui.txtActionTerminal.setTextColor(QColor(95,37,159))
        else:
            self.ui.txtActionTerminal.setTextColor(QColor("black"))    
        if (msg != ""): self.ui.txtActionTerminal.append(str(msg))
            
        
