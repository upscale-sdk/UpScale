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

from ftplib import FTP # for FTP connection

class ClientFTP:
	def __init__(self):
		self.Client = ""
	
	def get(self, command, printOnTerminal):	
		errormsg = ""
		(errormsg, params) = command.getParameters()
		if errormsg != "": return errormsg
		print(params[0])
		print(params[1])
		print(params[2])
		self.Client = FTP(params[0])     # connect to host, default port		
		self.Client.login(params[1], params[2]) # connect with username and password
		
		for index in range(3, len(params), 2):
			remotepath = params[3]			
			localpath = params[4]
			# Check if the directory exists
			try:	
				dirname = ntpath.dirname(remotepath)			
				self.Client.voidcmd("dir " + dirname)
			except ftplib.error_reply:				
				printOnTerminal("|> The remote directory '" + dirname + "' does not exist on the server.", "alert")
				continue			
			# Check if the file exists in that directory
			basename = ntpath.basename(remotepath)
			listfiles = self.Client.nlst(dirname)
			if basename not in listfiles: 
				printOnTerminal("|> The file '" + basename + "' does not exist in the directory '" + dirname + "'", "alert")
				continue											
			localfile = open(localpath, 'wb')
			self.Client.retrbinary('RETR ' + remotepath, localfile.write)
		try:
			self.Client.quit()
		except ftplib.all_errors:
			self.Client.close()
		self.Client = ""
		return errormsg