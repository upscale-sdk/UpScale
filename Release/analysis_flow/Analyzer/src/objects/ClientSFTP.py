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

import ntpath # to extract the basename from a path
import paramiko # for SSH connection
import socket

from ClientSSH import *
from XMLtags import *

import os.path

class ClientSFTP:	
	def __init__(self):
		self.client = ""
	
	def isConnected(self):
		return (self.client != "")
		
	def connect(self, params, ssh_client):		
		if self.isConnected(): errormsg = "You are already connected. Use the SFTP command 'Close' before opening a new connection."
		elif ssh_client.isConnected() is False: errormsg = "You must be connected through SSH before opening a SFTP connection. Use the SSH command 'Open' before opening a new SFTP connection."
		else: 		
			self.client = ssh_client.open_sftp()
			errormsg = ""			
		return errormsg
		
	def close(self, params=[], ssh_client=ClientSSH()):
		errormsg = ""
		if (self.client != ""):
			self.client.close()
			self.client = ""
		return errormsg
		
	def get(self, params, ssh_client):	
		errormsg = ""
		if ssh_client.isConnected() is False:			
			errormsg = "You must be connected through SSH before using this SFTP command. Use the SSH command 'Open' to open an SSH connection. Then, use a SFTP command 'Open' to open the SFTP connection. Finally, use the SFTP command 'Get'."
		elif self.isConnected() is False:
			errormsg = "You must be connected through SFTP before using this SFTP command. Use the SFTP command 'Open' to open an SFTP connection."
		else: 					
			for index in range(0, len(params), 2):
				remotepath = params[index]			
				localpath = params[index + 1]
				self.client.chdir("/")
				# Check if the remote directory exists
				try:	
					dirname = ntpath.dirname(remotepath)					
					self.client.chdir(dirname)
				except paramiko.sftp.SFTPError:
					errormsg = "'" + dirname + "' is not a directory. Failed to download the file '" + ntpath.basename(remotepath) + "' from the server."
					continue				
				except OSError:					
					errormsg = "The remote directory '" + dirname + "' does not exist on the server. Failed to download the file '" + ntpath.basename(remotepath) + "' from the server."
					continue
				self.client.chdir("/")
				# Check if the remote path is a directory or a file
				try: 
					lstatout=str(self.client.lstat(remotepath)).split()[0]
					if 'd' in lstatout: remotePathIsDirectory = True
					else: remotePathIsDirectory = False											
				except OSError: remotePathIsDirectory = False				
				if remotePathIsDirectory is True:
					# Check if the local path is a directory					
					localpathIsDirectory = os.path.isdir(localpath)						
					if localpathIsDirectory is False:
						errormsg = "The remote path is a directory but the local path is not. Both paths should target a file or a directory. "
						continue					
					# get all the files						
					for file in self.client.listdir(remotepath):
						lstatout=str(self.client.lstat(remotepath + "/" + file)).split()[0]
						if 'd' not in lstatout: 
							remotefile = remotepath + '/' + file
							localfile =  localpath + '/' + file
							self.client.get(remotefile, localfile)					
				else:
					# The remote path is not a directory
					# Check if the local path is a directory
					localpathIsDirectory = os.path.isdir(localpath)						
					if localpathIsDirectory is True:
						errormsg = "The remote path is not a directory but the local path is. Both paths should target a file or a directory. "
						continue
					# Check if the file exists in that remote directory
					basename = ntpath.basename(remotepath)
					listfiles = self.client.listdir(dirname)
					if basename not in listfiles: 
						errormsg = "The file '" + basename + "' does not exist in the remote directory '" + dirname + "'. Failed to download the file '" + basename + "' from the server."
						continue											
					self.client.get(remotepath, localpath)			
		return errormsg
		
	def put(self, params, ssh_client=ClientSSH()):
		errormsg = ""
		if ssh_client.isConnected() is False:			
			errormsg = "You must be connected through SSH before using this SFTP command. Use the SSH command 'Open' to open an SSH connection. Then, use a SFTP command 'Open' to open the SFTP connection. Finally, use the SFTP command 'Get'."
		elif self.isConnected() is False:
			errormsg = "You must be connected through SFTP before using this SFTP command. Use the SFTP command 'Open' to open an SFTP connection."
		else: 					
			for index in range(0, len(params), 2):
				localpath = params[index]			
				remotepath = params[index + 1]
				self.client.chdir("/")
				# Check if the remote directory exists
				try:
					dirname = ntpath.dirname(remotepath)					
					self.client.chdir(dirname)
				except paramiko.sftp.SFTPError:
					errormsg = "'" + dirname + "' is not a directory."
					continue				
				except IOError:					
					errormsg = "The remote directory '" + dirname + "' does not exist on the server."
					continue
				self.client.chdir("/")
				# Check if the remote path is a directory or a file
				try: 
					lstatout=str(self.client.lstat(remotepath)).split()[0]
					if 'd' in lstatout: remotePathIsDirectory = True
					else: remotePathIsDirectory = False											
				except OSError: remotePathIsDirectory = False	
				except IOError: remotePathIsDirectory = False # The remote file does not exist					
				if remotePathIsDirectory is True:
					# Check if the local path is a directory					
					localpathIsDirectory = os.path.isdir(localpath)						
					if localpathIsDirectory is False:						
						errormsg = "The remote path is a directory but the local path is not. Both paths should target a file or a directory. "
						continue							
					# put all the files
					for file in os.listdir(localpath):
						if os.path.isfile(localpath + "/" + file) is True:
							remotefile = remotepath + '/' + file
							localfile =  localpath + '/' + file							
							self.client.put(localfile, remotefile)
				else:
					# The remote path is not a directory
					# Check if the local path is a directory
					localpathIsDirectory = os.path.isdir(localpath)						
					if localpathIsDirectory is True:
						errormsg = "The remote path is not a directory but the local path is. Both paths should target a file or a directory. "
						continue
					# Check if the file exists in that local directory
					basename = ntpath.basename(localpath)	
					dirname = ntpath.dirname(localpath)
					if basename not in os.listdir(dirname): 
						errormsg = "The file '" + basename + "' does not exist in the local directory '" + dirname + "'. Failed to upload the file '" + basename + "' to the server."
						continue
					self.client.put(localpath, remotepath)										
		return errormsg
		
	def chmod(self, params, ssh_client=""):
		errormsg = ""
		if ssh_client.isConnected() is False:			
			errormsg = "You must be connected through SSH before using this SFTP command. Use the SSH command 'Open' to open an SSH connection. Then, use a SFTP command 'Open' to open the SFTP connection. Finally, use the SFTP command 'Get'."
		elif self.isConnected() is False:
			errormsg = "You must be connected through SFTP before using this SFTP command. Use the SFTP command 'Open' to open an SFTP connection."
		else: 					
			for index in range(0, len(params), 2):
				remotepath = params[index]
				mode = params[index + 1]				
				# Check if the directory exists				
				try:	
					dirname = ntpath.dirname(remotepath)			
					self.client.chdir(dirname)			
				except OSError:					
					errormsg = errormsg + "|> The remote directory '" + dirname + "' does not exist on the server.\n"
					errormsg = errormsg + "|> Failed to modify the permissions for the file '" + ntpath.basename(remotepath) + "'.\n"
					continue
				# Check if the file exists in that directory
				basename = ntpath.basename(remotepath)
				listfiles = self.client.nlst(dirname)
				if basename not in listfiles: 
					errormsg = errormsg + "|> The file '" + basename + "' does not exist in the directory '" + dirname + "'\n"
					errormsg = errormsg + "|> Failed to modify the permissions for the file '" + basename + "'.\n"
					continue											
				self.client.chmod(remotepath, int(mode))					
		return errormsg	
		