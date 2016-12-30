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

import re

class CommandSFTP(Command):
	def __init__(self, name, operation, description = "", code = "", stoponfailure = True):
		Command.__init__(self, name, description, code, stoponfailure)
		self.operation = operation
		if operation == "Open": 
			self.syntax = re.compile("\A\Z")
			self.syntaxErrormsg = "A SFTP open command must have an empty code."
		elif operation == "Close": 
			self.syntax = re.compile("\A\Z")
			self.syntaxErrormsg = "A SFTP close command must have an empty code."
		elif operation == "Get": 
			#self.syntax = re.compile("^.+ \; [^\r\n]+(\n.+ \; [^\r\n]+)*$")
			self.syntax = re.compile("\A(^(.+) \; (.+))(\n^(.+) \; (.+))*\Z", re.MULTILINE)			
			self.syntaxErrormsg = "Syntax is: RemotePath ; LocalPath (at least one line)"
		elif operation == "Put": 
			self.syntax = re.compile("\A(^(.+) \; (.+))(\n^(.+) \; (.+))*\Z", re.MULTILINE)
			self.syntaxErrormsg = "Syntax is: LocalPath ; RemotePath (at least one line)"

	@staticmethod
	def display():
		foreground = QtGui.QBrush(QtGui.QColor(0, 100, 0))
		background = QtGui.QBrush(QtGui.QColor(193, 255, 193))	
		return foreground, background	
	
	def displayCode(self):
		code = ""
		if self.operation == "Open":			
			code = "Connecting by SFTP by using the SSH connection already open."
		elif self.operation == "Get":
			(errormsg, params) = self.getParameters()
			if (errormsg == ""):
				for index in range(0, len(params), 2):
					remotepath = params[index]			
					localpath = params[index + 1]
					code = code + "Transfer of '" + remotepath + "' to '" + localpath + "' \n"
		elif self.operation == "Put":
			(errormsg, params) = self.getParameters()
			if (errormsg == ""):
				for index in range(0, len(params), 2):
					localpath = params[index]			
					remotepath = params[index + 1]
					code = code + "Transfer of '" + localpath + "' to '" + remotepath + "' \n"
		return code
		
	def execute(self, runtime):
		errormsg = ""
		responsemsg = ""
		(errormsg, params) = self.getParameters()
		if errormsg == "": 
			if self.operation == "Open":				
				errormsg = runtime.SFTPClient.connect(params, runtime.SSHClient)
			elif self.operation == "Close":
				errormsg = runtime.SFTPClient.close(params, runtime.SSHClient)
			elif self.operation == "Get":
				errormsg = runtime.SFTPClient.get(params, runtime.SSHClient)
			elif self.operation == "Put":
				errormsg = runtime.SFTPClient.put(params, runtime.SSHClient)
			elif self.operation == "Chmod":
				errormsg = runtime.SFTPClient.chmod(params, runtime.SSHClient)
		return errormsg, responsemsg
		
	def getParameters(self):
		params = []
		errormsg = ""
		
		# if the command is a SFTP Open
		if self.operation == "Open":
			params = []
			if self.code != "": 
				errormsg = "For the command 'SSH close', the code must be empty"
		
		# If the command is a SFTP Close
		elif self.operation == "Close":
			params = []
			if self.code != "": 
				errormsg = "For the command 'SSH close', the code must be empty"
				
		# if the command is a SFTP Get
		elif self.operation == "Get":
			lines = self.code.split("\n")
			if len(lines) == 0: 
				errormsg = "For the SFTP command 'Get', You must specified which files you want to download, using the following syntax at each line: RemotePath ; LocalPath"
				return errormsg, params
			for line in lines:				
				parline = line.split(" ; ")
				if len(parline) != 2: 
					errormsg = "For the SFTP command 'Get', the syntax for all the lines (except the first one) of the code is: RemotePath ; LocalPath"
					return errormsg, params
				params.append(parline[0])
				params.append(parline[1])
			
		# if the command is a SFTP Put
		elif self.operation == "Put":
			lines = self.code.split("\n")
			if len(lines) == 0: 
				errormsg = "For the SFTP command 'Put', You must specified which files you want to download, using the following syntax at each line: LocalPath ; RemotePath"
				return errormsg, params
			for line in lines:				
				parline = line.split(" ; ")
				if len(parline) != 2: 
					errormsg = "For the SFTP command 'Put', the syntax for all the lines (except the first one) of the code is: LocalPath ; RemotePath"
					return errormsg, params
				params.append(parline[0])
				params.append(parline[1])			
						
		# if the command is a SFTP CHMOD
		elif self.operation == "Chmod":
			lines = self.code.split("\n")
			if len(lines) == 0: 
				errormsg = "For the SFTP command 'Chmod', You must specified which files you want to download, using the following syntax at each line: RemotePath ; Permissions"
				return errormsg, params
			line_index = 1
			for line in lines:				
				parline = line.split(" ; ")
				if line_index == 1:
					if len(parline) != 4: 
						errormsg = "For the SFTP command 'Chmod', the syntax for the first line of the code is: host ; port ; username ; password"
						return errormsg, params
					params.append(parline[0])
					params.append(parline[1])
					params.append(parline[2])
					params.append(parline[3])
				else:
					if len(parline) != 2: 
						errormsg = "For the SFTP command 'Chmod', the syntax for all the lines (except the first one) of the code is: RemotePath ; Permissions"
						return errormsg, params
					params.append(parline[0])
					params.append(parline[1])
				line_index += 1
				
		return errormsg, params	
		
	
