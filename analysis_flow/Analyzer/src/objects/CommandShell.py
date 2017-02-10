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

import os
import platform

from PyQt4 import QtGui
from Command import *

import subprocess

class CommandShell(Command):
	def __init__(self, name, operation, description = "", code = "", stoponfailure = True):
		Command.__init__(self, name, description, code, stoponfailure)
		self.operation = operation

	@staticmethod
	def display():
		foreground = QtGui.QBrush(QtGui.QColor(139, 105, 20))
		background = QtGui.QBrush(QtGui.QColor(255, 236, 139))	
		return foreground, background	
	
	def displayCode(self):
		return self.code
		
	def execute(self, runtime):
		errormsg = ""
		responsemsg = ""		
		pf = platform.system()
		filename = ""
		if pf == 'Windows':
			filename = 'script.bat'
			f = open( filename, 'w' )
			f.write(self.code)
			f.close()
		elif pf == 'Darwin':
			filename = './script.sh'
			f = open( filename, 'w' )
			f.write(self.code)
			f.close()			
			subprocess.call(['chmod', '0777', filename])	
		p = subprocess.Popen(filename, shell=True, stdout = subprocess.PIPE, stderr = subprocess.PIPE)		
		(stdout, stderr) = p.communicate()				
		#retcode = p.returncode 		
		if stdout is not None: responsemsg = stdout.decode()
		if stderr is not None: errormsg = stderr.decode()
		os.remove(filename)
		return errormsg, responsemsg
		
	