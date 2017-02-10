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

from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *

import images_rc

class DragAndDropButton(QPushButton):
	def __init__(self):
		QPushButton.__init__(self)
		sizePolicy = QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
		sizePolicy.setHorizontalStretch(0)
		sizePolicy.setVerticalStretch(0)		
		sizePolicy.setHeightForWidth(self.sizePolicy().hasHeightForWidth())
		self.setSizePolicy(sizePolicy)
		self.setMinimumSize(QtCore.QSize(35, 35))
		self.setMaximumSize(QtCore.QSize(35, 35))
		self.setText("")
		self.setToolTip("Drag and drop")
		icon = QtGui.QIcon()
		icon.addPixmap(QtGui.QPixmap(":/draganddrop.ico"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
		self.setIcon(icon)
		self.setIconSize(QtCore.QSize(20, 20))
		self.setToolTip("Click here and drag this variable to another line")
		
	def mouseMoveEvent(self, e):		
		mimeData = QMimeData()
		drag = QDrag(self)
		drag.setMimeData(mimeData)
		drag.setHotSpot(e.pos() - self.rect().topLeft())
		dropAction = drag.exec_(Qt.MoveAction)
	
