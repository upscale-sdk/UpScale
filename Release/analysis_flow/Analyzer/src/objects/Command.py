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


from Variable import *
from PyQt4 import QtGui
import re

class Command:
	def __init__(self, name, description = "", code = "", stoponfailure = True):
		self.name = name
		self.description = description
		self.stoponfailure = stoponfailure
		self.code = str(code)
		self.initialcode = str(code)
		self.syntax = re.compile(".")
		self.syntaxErrormsg = ""

	@staticmethod
	def display():
		foreground = QtGui.QBrush(QtGui.QColor(0, 0, 0))
		background = QtGui.QBrush(QtGui.QColor(200, 200, 200))	
		return foreground, background
	
	def decode(self, listVariables):		
		replacementOccured = True				
		if self.code != "" and self.code is not None:
			while '@{' in self.code and replacementOccured is True:
				replacementOccured = False
				code = str(self.code)				
				for variable in listVariables: self.code = str(self.code.replace("@{" + variable.name + "}", variable.selectedValue))				
				if (code != self.code): replacementOccured = True				
		return replacementOccured # replacementOccured is False if there is an occurence of "@{" in the code but no corresponding variable => It is likely that a variable is misspelled in the code or a new variable must be added	
			
	def resetCode(self):		
		self.code = str(self.initialcode)	
			