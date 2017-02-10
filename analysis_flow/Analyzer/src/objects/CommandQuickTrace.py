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

from Command import *

class CommandQuickTrace(Command):
	def __init__(self, name, operation, description = "", code = "", stoponfailure = True):
		Command.__init__(self, name, description, code, stoponfailure)
		self.operation = operation
		self.condSatisfied = False
		self.nextCommand = 0
		if operation == "Set Variable[s]": 
			self.syntax = re.compile("\A(^(.+) \; (.+))(\n^(.+) \; (.+))*\Z", re.MULTILINE)			
			self.syntaxErrormsg = "Syntax is: Variable name ; value (at least one line)"
		if operation == "Increment Variable": 
			#NEED TO BE REDEFINED!!!
			self.syntax = re.compile("\A(^(.+) \; (.+))(\n^(.+) \; (.+))*\Z", re.MULTILINE)			
			self.syntaxErrormsg = "Syntax is: VariableName ; OperationType ; ValueToCompareWith ; CommandNumber (one line)"
		if operation == "If xxx then GoTo": 
			#NEED TO BE REDEFINED!!!
			self.syntax = re.compile("\A(^(.+) \; (.+))(\n^(.+) \; (.+))*\Z", re.MULTILINE)			
			self.syntaxErrormsg = "Syntax is: VariableName ; OperationType ; ValueToCompareWith ; CommandNumber (one line)"
			
	@staticmethod
	def display():
		foreground = QtGui.QBrush(QtGui.QColor(41, 36, 33))
		background = QtGui.QBrush(QtGui.QColor(255, 165, 79))	
		return foreground, background	
	
	def displayCode(self):
		errormsg = ""
		responsemsg = ""				
		(errormsg, params) = self.getParameters()
		if errormsg == "": 
			if self.operation == "Set Variable[s]":
				responsemsg = "Set variable '" + params[0] + "' to '" + params[1] + "'." 
			if self.operation == "Increment Variable":
				responsemsg = "Increment variable '" + params[0] + "' by '" + params[1] + "'."				
			if self.operation == "If xxx then GoTo":
				responsemsg = "If '" + params[0] + "' " + params[1] + " " + params[2] + " then go to line " + params[3] + "."
		return responsemsg
	
	def exec_ifgoto(self, params, runtime):
		print("Execute: If-Go-To")
		errormsg = ""							
		varName = params[0]
		opType = params[1]
		compValue = int(params[2])
		self.nextCommand = 0
		self.condSatisfied = False
		varFound = False
		for variable in runtime.listVariables:				
			if variable.name == varName:
				varFound = True
				varValue = int(variable.selectedValue)
				if opType == "<": self.condSatisfied = (varValue < compValue)
				if opType == "<=": self.condSatisfied = (varValue <= compValue)
				if opType == "==": self.condSatisfied = (varValue == compValue)
				if opType == ">": self.condSatisfied = (varValue > compValue)
				if opType == ">=": self.condSatisfied = (varValue >= compValue)	 	
				print(str(varValue) + " " + opType + " " + str(compValue) + " is " + str(self.condSatisfied))	
				if self.condSatisfied is True: self.nextCommand = params[3]
				break	
		if varFound is False: # one of the variables does not exist and all the values need to be restored			
			errormsg = "|> The variable '" + varName + "' does not exist (all variables restored)"		
		return errormsg
	
	def exec_varinc(self, params, runtime):
		print("Execute: Var-inc")
		errormsg = ""		
		varName = params[0]
		increment = params[1]		
		varFound = False
		for variable in runtime.listVariables:				
			if variable.name == varName:
				varFound = True
				runtime.varRedefinedNames.append(varName)
				runtime.varInitialValues.append(variable.value)
				variable.selectedValue = str(int(variable.selectedValue) + int(increment))
				print(varName + " <- " + variable.selectedValue)					
				break			
		if varFound is False: # one of the variables does not exist and all the values need to be restored
			runtime.restoreVariables()
			errormsg = "|> The variable '" + varName + "' does not exist (all variables restored)"
		return errormsg
	
	def exec_varset(self, params, runtime):
		print("Execute: Var-set")
		errormsg = ""		
		for index in range(0, len(params), 2):			
			varName = params[index]
			newValue = params[index + 1]		
			varFound = False
			for variable in runtime.listVariables:				
				if variable.name == varName:
					varFound = True
					print(varName + " <- " + newValue)					
					runtime.varRedefinedNames.append(varName)
					runtime.varInitialValues.append(variable.value)
					variable.selectedValue = newValue					
					break			
			if varFound is False: # one of the variables does not exist and all the values need to be restored
				runtime.restoreVariables()
				errormsg = "|> The variable '" + varName + "' does not exist (all variables restored)"
				break
		return errormsg
	
	def getParameters(self):
		params = []
		errormsg = ""
		
		# if the command is a var set
		if self.operation == "Set Variable[s]":
			lines = self.code.split("\n")						
			if len(lines) == 0: 
				errormsg = "For the command 'Set Variable[s]', you must specified which variables you want to update, using the following syntax at each line: VariableName ; newValue"
				return errormsg, params
			for line in lines:				
				parline = line.split(" ; ")
				if len(parline) != 2: 
					errormsg = "For the command 'Set Variable[s]', the syntax of each line must be: VariableName ; newValue"
					return errormsg, params
				params.append(parline[0])
				params.append(parline[1])
				
		# if the command is a var inc
		if self.operation == "Increment Variable":
			line = self.code
			parline = line.split(" ; ")
			if len(parline) != 2: 
				errormsg = "For the command 'Increment Variable', the syntax of each line must be: VariableName ; Increment"
				return errormsg, params
			params.append(parline[0])
			params.append(parline[1])
				
		# if the command is a if-to-go
		if self.operation == "If xxx then GoTo":
			line = self.code
			parline = line.split(" ; ")
			if len(parline) != 4: 
				errormsg = "For the command 'If xxx then GoTo', the syntax of each line must be: VariableName ; OperationType ; ValueToCompareWith ; CommandNumber"
				return errormsg, params
			params.append(parline[0])
			params.append(parline[1])
			params.append(parline[2])
			params.append(parline[3])				
		
		return errormsg, params
				
	def execute(self, runtime):
		errormsg = ""
		responsemsg = ""				
		(errormsg, params) = self.getParameters()
		if errormsg == "": 
			if self.operation == "Set Variable[s]":
				errormsg = self.exec_varset(params, runtime)
			if self.operation == "Increment Variable":
				errormsg = self.exec_varinc(params, runtime)				
			if self.operation == "If xxx then GoTo":
				errormsg = self.exec_ifgoto(params, runtime)
		return errormsg, responsemsg
		
				
				
			
