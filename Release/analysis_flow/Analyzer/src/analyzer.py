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


#from __future__ import print_function

# Hidden import

#import PyQt4, PySide, unicodedata, _sre, _warnings, types, strop

#from PyQt4.QtCore import QCoreApplication
#from PySide import QCoreApplication
#from unicodedata import ucd_3_2_0
#from _sre import MAXREPEAT
#from _warnings import warn_explicit, filters, once_registry, default_action, warn
#from types import SimpleNamespace
#from strop import whitespace, maketrans, lowercase, uppercase

import crypto

from Options import *
import platform

if platform.system() == 'Darwin':	
	#os.environ['R_HOME']='/opt/local/Library/Frameworks/R.framework/Versions/3.2/Resources'	
	#os.environ["R_USER"]='/Users/'	
	Options.setFontSize(13)	
elif platform.system() == 'Windows':
	Options.setFontSize(10)

import sys
#import win32api
	
from PyQt4.QtGui import QApplication
from PyQt4.QtGui import QSplashScreen

from Main import mainWindow

if __name__ == '__main__':	
	app = QApplication(sys.argv)
	app.addLibraryPath("./")
	mainwin = mainWindow()	
	
	# Trick to enable the Toolbar 
	dummy = QSplashScreen()
	dummy.show()
	dummy.finish(mainwin)
	
	mainwin.show()
	mainwin.raise_()
	sys.exit(app.exec_())
	