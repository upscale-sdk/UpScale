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

from PyQt4 import QtGui

from CommandQuickTrace import *

from Options import *

class Action:	
	def __init__(self, rowID, name, commandlist = "", color="#000000", description = ""):
		self.rowID = rowID
		self.name = name
		self.commandlist = commandlist
		self.color = color
		self.description = description	
		self.showActionDetails = False
		
	def display(self):
		foreground = QtGui.QBrush(QtGui.QColor(255, 255, 255))
		background = QtGui.QBrush(QtGui.QColor(37, 108, 37))	
		return foreground, background
	
	def numberOfCommands(self, runtime):
		# unroll all the actions within the action to be executed		
		tempList = self.commandlist		
		listcommands = []
		containAction = True
		while containAction is True:
			containAction = False
			for item in tempList:
				itemFoundAsCommand = False
				for command in runtime.listCommands:
					if command.name == item:
						itemFoundAsCommand = True
						listcommands.append(command.name)
						break	
				if itemFoundAsCommand is False:
					containAction = True
					for action in runtime.listActions:
						if action.name == item:
							for cmdName in action.commandlist:
								listcommands.append(cmdName)
							break
			tempList = listcommands
			listcommands = []			
		listcommands = tempList
		return len(listcommands)
	
	def execute(self, runtime, executeCommandFunction):								
		# unroll all the actions within the action to be executed		
		tempList = self.commandlist		
		listcommands = []
		containAction = True
		while containAction is True:
			containAction = False
			for item in tempList:
				itemFoundAsCommand = False
				for command in runtime.listCommands:
					if command.name == item:
						itemFoundAsCommand = True
						listcommands.append(command.name)
						break	
				if itemFoundAsCommand is False:
					containAction = True
					for action in runtime.listActions:
						if action.name == item:
							for cmdName in action.commandlist:
								listcommands.append(cmdName)
							break
			tempList = listcommands
			listcommands = []			
		listcommands = tempList
		
		# listcommands now contains the name of all commands to be executed			
		# Execute every command in listcommands
		
		result = True	
		cmdIndex=0
		while cmdIndex < len(listcommands):
			cmdName = listcommands[cmdIndex]
			for command in runtime.listCommands:
				if command.name == cmdName:					
					result = executeCommandFunction(command)
					if command.stoponfailure is True and result is False: 
						runtime.SFTPClient.close()
						runtime.SSHClient.close()
					else:
						if (isinstance(command, CommandQuickTrace) is True) and (command.operation == "If xxx then GoTo") and (command.condSatisfied is True):
							# Must be decreased by two because it starts at index 0 (instead of 1) and there is the +1 at the end of the loop, 5 lines below  
							cmdIndex = int(command.nextCommand) - 2						
					break	
			if command.stoponfailure is True and result is False: break		  	
			cmdIndex += 1
		
		#print(listcommands)
		if len(runtime.varRedefinedNames) > 0: runtime.restoreVariables()		
		return result
		