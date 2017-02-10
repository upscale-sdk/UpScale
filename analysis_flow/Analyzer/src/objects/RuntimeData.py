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

from ClientSSH import *
from ClientSFTP import *

class RuntimeData:	
	def __init__(self):		
		self.SSHClient = ClientSSH()		
		self.SFTPClient = ClientSFTP()
		self.listVariables = []
		self.listCommands = []
		self.listActions = []
		self.varRowID = 1
		self.actionRowID = 1		
		self.selectedCommand = 0
		self.updateCommandActive = True		
		self.varRedefinedNames = []
		self.varInitialValues = []
	
	def restoreVariables(self):	
		print("Restore variables")
		for index in range(len(self.varRedefinedNames)-1, -1, -1):			
			varName = self.varRedefinedNames[index]
			for variable in self.listVariables:
				if (variable.name == varName):
					print(varName + " <- " + self.varInitialValues[index])
					variable.value =  self.varInitialValues[index]
					break		
		self.varRedefinedNames = []
		self.varInitialValues = []
		
	def deleteVariable(self, varName):
		ret = False
		for variable in self.listVariables:
			if (variable.name == varName):
				self.listVariables.remove(variable)
				ret = True
				break
		return ret
	
	def swapVariablesPosition(self, varname_src, varname_dest):		
		index = 0
		fromRow = 0
		toRow = 0
		varToBeMoved=None
		for var in self.listVariables:
			if var.name == varname_src: 
				fromRow = index
				varToBeMoved = var
			if var.name == varname_dest: 
				toRow = index
			index += 1
		if fromRow < toRow: toRow = toRow + 1
		else: fromRow = fromRow + 1			
		# Move the variable in the model (the variable list)              
		self.listVariables.insert(toRow, varToBeMoved)
		self.listVariables.pop(fromRow)
	
	def swapActionsPosition(self, acname_src, acname_dest):		
		index = 0
		fromRow = 0
		toRow = 0
		acToBeMoved=None	
		for action in self.listActions:			
			if action.name == acname_src: 
				fromRow = index
				acToBeMoved = action
			if action.name == acname_dest: 
				toRow = index
			index += 1
		if fromRow < toRow: toRow = toRow + 1
		else: fromRow = fromRow + 1			
		# Move the action in the model (the action list)              
		self.listActions.insert(toRow, acToBeMoved)
		self.listActions.pop(fromRow)		
		
	def deleteCommand(self, cmdName):
		ret = False
		for command in self.listCommands:
			if (command.name == cmdName):
				self.listCommands.remove(command)
				ret = True
				break
		return ret
		
	def deleteAction(self, actionName):
		ret = False
		for action in self.listActions:
			if (action.name == actionName):
				ret = self.listActions.remove(action)				
				break
		return ret
	