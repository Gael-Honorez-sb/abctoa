
# -*- coding: utf-8 -*-
# Alembic Holder
# Copyright (c) 2014, Gaël Honorez, All rights reserved.
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3.0 of the License, or (at your option) any later version.
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
# You should have received a copy of the GNU Lesser General Public
# License along with this library.
# Form implementation generated from reading ui file 'UI_ABCHierarchy.ui'
#
# Created: Thu Sep 11 16:49:59 2014
#      by: pyside-uic 0.2.15 running on PySide 1.2.1
#
# WARNING! All changes made in this file will be lost!

from PySide import QtCore, QtGui

class Ui_NAM(object):
    def setupUi(self, NAM):
        NAM.setObjectName("NAM")
        NAM.resize(1600, 661)
        NAM.setMinimumSize(QtCore.QSize(1600, 0))
        NAM.setBaseSize(QtCore.QSize(1280, 0))
        self.centralwidget = QtGui.QWidget(NAM)
        self.centralwidget.setObjectName("centralwidget")
        self.gridLayout_3 = QtGui.QGridLayout(self.centralwidget)
        self.gridLayout_3.setObjectName("gridLayout_3")
        self.splitter_2 = QtGui.QSplitter(self.centralwidget)
        self.splitter_2.setMinimumSize(QtCore.QSize(0, 0))
        self.splitter_2.setOrientation(QtCore.Qt.Horizontal)
        self.splitter_2.setObjectName("splitter_2")
        self.layoutWidget = QtGui.QWidget(self.splitter_2)
        self.layoutWidget.setObjectName("layoutWidget")
        self.gridLayout = QtGui.QGridLayout(self.layoutWidget)
        self.gridLayout.setContentsMargins(0, 0, 0, 0)
        self.gridLayout.setObjectName("gridLayout")
        self.overrideShaders = QtGui.QCheckBox(self.layoutWidget)
        self.overrideShaders.setObjectName("overrideShaders")
        self.gridLayout.addWidget(self.overrideShaders, 0, 0, 1, 1)
        self.renderLayer = QtGui.QComboBox(self.layoutWidget)
        self.renderLayer.setMinimumSize(QtCore.QSize(150, 0))
        self.renderLayer.setObjectName("renderLayer")
        self.gridLayout.addWidget(self.renderLayer, 0, 4, 1, 1)
        self.splitter = QtGui.QSplitter(self.layoutWidget)
        self.splitter.setOrientation(QtCore.Qt.Vertical)
        self.splitter.setObjectName("splitter")
        self.hierarchyWidget = QtGui.QTreeWidget(self.splitter)
        self.hierarchyWidget.setMouseTracking(True)
        self.hierarchyWidget.setDragDropMode(QtGui.QAbstractItemView.DropOnly)
        self.hierarchyWidget.setAlternatingRowColors(True)
        self.hierarchyWidget.setSelectionMode(QtGui.QAbstractItemView.ExtendedSelection)
        self.hierarchyWidget.setRootIsDecorated(True)
        self.hierarchyWidget.setUniformRowHeights(False)
        self.hierarchyWidget.setColumnCount(3)
        self.hierarchyWidget.setObjectName("hierarchyWidget")
        self.hierarchyWidget.header().setVisible(True)
        self.tagGroup = QtGui.QGroupBox(self.splitter)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Preferred, QtGui.QSizePolicy.Minimum)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.tagGroup.sizePolicy().hasHeightForWidth())
        self.tagGroup.setSizePolicy(sizePolicy)
        self.tagGroup.setObjectName("tagGroup")
        self.gridLayout_2 = QtGui.QGridLayout(self.tagGroup)
        self.gridLayout_2.setContentsMargins(0, 0, 0, 0)
        self.gridLayout_2.setObjectName("gridLayout_2")
        self.gridLayout.addWidget(self.splitter, 3, 0, 1, 5)
        self.overrideDisps = QtGui.QCheckBox(self.layoutWidget)
        self.overrideDisps.setObjectName("overrideDisps")
        self.gridLayout.addWidget(self.overrideDisps, 0, 1, 1, 1)
        self.overrideProps = QtGui.QCheckBox(self.layoutWidget)
        self.overrideProps.setObjectName("overrideProps")
        self.gridLayout.addWidget(self.overrideProps, 0, 2, 1, 1)
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.gridLayout.addItem(spacerItem, 0, 3, 1, 1)
        self.shadersList = QtGui.QListWidget(self.splitter_2)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.shadersList.sizePolicy().hasHeightForWidth())
        self.shadersList.setSizePolicy(sizePolicy)
        self.shadersList.setMinimumSize(QtCore.QSize(0, 0))
        self.shadersList.setMaximumSize(QtCore.QSize(300, 16777215))
        self.shadersList.setBaseSize(QtCore.QSize(0, 0))
        self.shadersList.setDragDropMode(QtGui.QAbstractItemView.DragOnly)
        self.shadersList.setAlternatingRowColors(True)
        self.shadersList.setIconSize(QtCore.QSize(25, 25))
        self.shadersList.setSpacing(5)
        self.shadersList.setObjectName("shadersList")
        self.gridLayout_3.addWidget(self.splitter_2, 0, 0, 1, 1)
        NAM.setCentralWidget(self.centralwidget)
        self.menubar = QtGui.QMenuBar(NAM)
        self.menubar.setGeometry(QtCore.QRect(0, 0, 1600, 21))
        self.menubar.setObjectName("menubar")
        NAM.setMenuBar(self.menubar)
        self.statusbar = QtGui.QStatusBar(NAM)
        self.statusbar.setObjectName("statusbar")
        NAM.setStatusBar(self.statusbar)

        self.retranslateUi(NAM)
        QtCore.QMetaObject.connectSlotsByName(NAM)

    def retranslateUi(self, NAM):
        NAM.setWindowTitle(QtGui.QApplication.translate("NAM", "Nozon Alembic Cache Manager", None, QtGui.QApplication.UnicodeUTF8))
        self.overrideShaders.setText(QtGui.QApplication.translate("NAM", "Override All Shaders", None, QtGui.QApplication.UnicodeUTF8))
        self.hierarchyWidget.setSortingEnabled(False)
        self.hierarchyWidget.headerItem().setText(0, QtGui.QApplication.translate("NAM", "ABC Hierarchy", None, QtGui.QApplication.UnicodeUTF8))
        self.hierarchyWidget.headerItem().setText(1, QtGui.QApplication.translate("NAM", "shaders", None, QtGui.QApplication.UnicodeUTF8))
        self.hierarchyWidget.headerItem().setText(2, QtGui.QApplication.translate("NAM", "displacement", None, QtGui.QApplication.UnicodeUTF8))
        self.tagGroup.setTitle(QtGui.QApplication.translate("NAM", "Tags", None, QtGui.QApplication.UnicodeUTF8))
        self.overrideDisps.setText(QtGui.QApplication.translate("NAM", "Override all displacement shaders", None, QtGui.QApplication.UnicodeUTF8))
        self.overrideProps.setText(QtGui.QApplication.translate("NAM", "Override all object properties", None, QtGui.QApplication.UnicodeUTF8))
        self.shadersList.setSortingEnabled(True)

