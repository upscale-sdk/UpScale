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



import paramiko # for SSH connection
import re

from PyQt4 import QtGui
from Command import *
	
class CommandSSH(Command):
	def __init__(self, name, operation, description = "", code = "", stoponfailure = True):
		Command.__init__(self, name, description, code, stoponfailure)
		self.operation = operation		
		if operation == "Open": 
			self.syntax = re.compile("\A.+ \; .+ \; .+ \; (.+)\Z")
			self.syntaxErrormsg = "Syntax is: host_IP ; port ; username ; password"
		elif operation == "Close": 
			self.syntax = re.compile("\A\Z")
			self.syntaxErrormsg = "A SSH close command must have an empty code."
			
	@staticmethod
	def display():	
		foreground = QtGui.QBrush(QtGui.QColor(16, 78, 139))
		background = QtGui.QBrush(QtGui.QColor(198, 226, 255))	
		return foreground, background
	
	def displayCode(self):
		code = ""
		if self.operation == "Open":
			(errormsg, params) = self.getParameters()			
			if (errormsg == ""): code = "Connect to '" + params[0] + " on port " + params[1] + ", user = " + params[2]
		elif self.operation == "Remote Command[s]":			
			code = self.code		
		return code
		
	def execute(self, runtime):
		errormsg = ""
		responsemsg = ""
		(errormsg, params) = self.getParameters()
		if errormsg == "": 
			if self.operation == "Open":				
				errormsg = runtime.SSHClient.connect(params)
			elif self.operation == "Close":
				errormsg = runtime.SSHClient.close()
			elif self.operation == "Remote Command[s]":
				if runtime.SSHClient.isConnected():
					try:						
						(stdin, stdout, stderr) = runtime.SSHClient.exec_command(self.code)			
						error = stderr.read().splitlines()	
						outmsg = stdout.read().splitlines()
						if (len(outmsg) > 0): 
							for line in outmsg:
								str_line = str(line)
								if str_line[:2] == "b'": 
									str_line = str_line[2:]
									str_line = str_line[:len(str_line) - 1]
								responsemsg = responsemsg + "\n" + str_line #save the response only if different from that crap b''
						if (len(error) > 0): 
							for line in error:
								str_line = str(line)
								if str_line[:2] == "b'": 
									str_line = str_line[2:]
									str_line = str_line[:len(str_line) - 1]
								errormsg = errormsg + "\n" + str_line
					except paramiko.SSHException: errormsg = "SSHException: request was rejected or the channel was closed"
				else: errormsg = "You are not connected. Use the SSH command 'Open' to connect to the platform."
		return errormsg, responsemsg
			
			
	def getParameters(self):
		params = []
		errormsg = ""
		
		# If the special command is a SSH open
		if self.operation == "Open":
			params = self.code.split(" ; ")
			if len(params) != 4:
				errormsg = "For the command 'SSH open', the code must be a single line using the syntax: host ; port ; username ; password"					
			
		# If the special command is a SSH close
		elif self.operation == "Close":
			params = []
			if self.code != "": 
				errormsg = "For the command 'SSH close', the code must be empty"
		
		elif self.operation == "Remote Command[s]":
			params = []
			
		return errormsg, params
