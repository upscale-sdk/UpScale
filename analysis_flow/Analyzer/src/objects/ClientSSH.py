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
import socket

class ClientSSH:	
	def __init__(self):
		self.client = paramiko.SSHClient()
	
	def isConnected(self):
		transport = self.client.get_transport() if self.client else None
		return transport and transport.is_active()
		
	def connect(self, params):		
		if self.isConnected(): errormsg = "You are already connected. Use the SSH command 'Close' before opening a new connection."
		else: 				
			errormsg = ""
			host = params[0]
			port = params[1]
			username = params[2]
			password = params[3]
			try:			
				self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy())			
				self.client.connect(host, int(port), username, password)
			except paramiko.AuthenticationException: errormsg = "|> Failed to connect: authentication failed"			
			except paramiko.BadHostKeyException: errormsg = "|> Failed to connect: the server's host key could not be verified"			
			except paramiko.SSHException: errormsg = "|> Failed to connect: there was any other error connecting or establishing an SSH session"			
			except socket.error: errormsg = "|> Failed to connect: a socket error occurred while connecting. Try another port."		
		return errormsg
		
	def close(self):	
		errormsg = ""
		if (self.client != ""):
			self.client.close()			
		return errormsg
		
	def open_sftp(self):
		return self.client.open_sftp()
			
	
	def exec_command(self, code):
		return self.client.exec_command(code) 		