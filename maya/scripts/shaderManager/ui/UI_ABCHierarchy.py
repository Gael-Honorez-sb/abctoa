# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'UI_ABCHierarchy.ui'
#
# Created: Thu Mar 12 11:02:30 2015
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
        self.gridLayout_2 = QtGui.QGridLayout(self.centralwidget)
        self.gridLayout_2.setObjectName("gridLayout_2")
        self.gridLayout = QtGui.QGridLayout()
        self.gridLayout.setObjectName("gridLayout")
        self.overrideShaders = QtGui.QCheckBox(self.centralwidget)
        self.overrideShaders.setObjectName("overrideShaders")
        self.gridLayout.addWidget(self.overrideShaders, 0, 0, 1, 1)
        self.overrideDisps = QtGui.QCheckBox(self.centralwidget)
        self.overrideDisps.setObjectName("overrideDisps")
        self.gridLayout.addWidget(self.overrideDisps, 0, 1, 1, 1)
        self.overrideProps = QtGui.QCheckBox(self.centralwidget)
        self.overrideProps.setObjectName("overrideProps")
        self.gridLayout.addWidget(self.overrideProps, 0, 2, 1, 1)
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.gridLayout.addItem(spacerItem, 0, 3, 1, 1)
        self.renderLayer = QtGui.QComboBox(self.centralwidget)
        self.renderLayer.setMinimumSize(QtCore.QSize(150, 0))
        self.renderLayer.setObjectName("renderLayer")
        self.gridLayout.addWidget(self.renderLayer, 0, 5, 1, 1)
        self.splitter = QtGui.QSplitter(self.centralwidget)
        self.splitter.setOrientation(QtCore.Qt.Vertical)
        self.splitter.setObjectName("splitter")
        self.hierarchyWidget = QtGui.QTreeWidget(self.splitter)
        self.hierarchyWidget.setMouseTracking(True)
        self.hierarchyWidget.setStyleSheet("QTreeWidget\n"
"\n"
"{\n"
"\n"
"border-style:solid;\n"
"    \n"
"border-width:1px;\n"
"    \n"
"border-color:#353535;\n"
"    \n"
"color:silver;\n"
"    \n"
"padding:5px;\n"
"    \n"
"border-top-right-radius : 5px;\n"
"    \n"
"border-top-left-radius : 5px;   \n"
"    \n"
"border-bottom-left-radius : 5px;\n"
"    \n"
"border-bottom-right-radius : 5px;   \n"
"}\n"
"\n"
"\n"
"\n"
"QTreeWidget::item:hover\n"
"\n"
"{\n"
"\n"
"border: none;\n"
"    \n"
"background: #000000;\n"
"    \n"
"border-radius: 3px;\n"
"}\n"
"\n"
"QTreeWidget::item:previously-selected\n"
"\n"
"\n"
"\n"
"{\n"
"\n"
"border: none;\n"
"}\n"
"\n"
"\n"
"\n"
"QTreeWidget::item:selected, QTreeWidget::item:previously-selected\n"
"\n"
"{\n"
"\n"
"border: none;\n"
"}")
        self.hierarchyWidget.setDragDropMode(QtGui.QAbstractItemView.DropOnly)
        self.hierarchyWidget.setAlternatingRowColors(True)
        self.hierarchyWidget.setSelectionMode(QtGui.QAbstractItemView.ExtendedSelection)
        self.hierarchyWidget.setRootIsDecorated(True)
        self.hierarchyWidget.setUniformRowHeights(False)
        self.hierarchyWidget.setColumnCount(3)
        self.hierarchyWidget.setObjectName("hierarchyWidget")
        self.hierarchyWidget.header().setVisible(True)
        self.gridLayout.addWidget(self.splitter, 3, 0, 1, 6)
        self.isolateCheckbox = QtGui.QCheckBox(self.centralwidget)
        self.isolateCheckbox.setObjectName("isolateCheckbox")
        self.gridLayout.addWidget(self.isolateCheckbox, 0, 4, 1, 1)
        self.gridLayout_2.addLayout(self.gridLayout, 0, 0, 1, 1)
        self.splitter_2 = QtGui.QSplitter(self.centralwidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Preferred, QtGui.QSizePolicy.Minimum)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.splitter_2.sizePolicy().hasHeightForWidth())
        self.splitter_2.setSizePolicy(sizePolicy)
        self.splitter_2.setOrientation(QtCore.Qt.Vertical)
        self.splitter_2.setChildrenCollapsible(True)
        self.splitter_2.setObjectName("splitter_2")
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
        self.shadersList.setSpacing(2)
        self.shadersList.setObjectName("shadersList")
        self.displacementList = QtGui.QListWidget(self.splitter_2)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Minimum)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.displacementList.sizePolicy().hasHeightForWidth())
        self.displacementList.setSizePolicy(sizePolicy)
        self.displacementList.setMaximumSize(QtCore.QSize(300, 16777215))
        self.displacementList.setSpacing(2)
        self.displacementList.setObjectName("displacementList")
        self.gridLayout_2.addWidget(self.splitter_2, 0, 1, 1, 1)
        self.wildCardButton = QtGui.QPushButton(self.centralwidget)
        self.wildCardButton.setMaximumSize(QtCore.QSize(200, 16777215))
        self.wildCardButton.setObjectName("wildCardButton")
        self.gridLayout_2.addWidget(self.wildCardButton, 1, 0, 1, 1)
        self.refreshShadersBtn = QtGui.QPushButton(self.centralwidget)
        self.refreshShadersBtn.setObjectName("refreshShadersBtn")
        self.gridLayout_2.addWidget(self.refreshShadersBtn, 1, 1, 1, 1)
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
        self.overrideDisps.setText(QtGui.QApplication.translate("NAM", "Override all displacement shaders", None, QtGui.QApplication.UnicodeUTF8))
        self.overrideProps.setText(QtGui.QApplication.translate("NAM", "Override all object properties", None, QtGui.QApplication.UnicodeUTF8))
        self.hierarchyWidget.setSortingEnabled(False)
        self.hierarchyWidget.headerItem().setText(0, QtGui.QApplication.translate("NAM", "ABC Hierarchy", None, QtGui.QApplication.UnicodeUTF8))
        self.hierarchyWidget.headerItem().setText(1, QtGui.QApplication.translate("NAM", "shaders", None, QtGui.QApplication.UnicodeUTF8))
        self.hierarchyWidget.headerItem().setText(2, QtGui.QApplication.translate("NAM", "displacement", None, QtGui.QApplication.UnicodeUTF8))
        self.isolateCheckbox.setText(QtGui.QApplication.translate("NAM", "Isolate Selected", None, QtGui.QApplication.UnicodeUTF8))
        self.shadersList.setSortingEnabled(True)
        self.displacementList.setSortingEnabled(True)
        self.wildCardButton.setText(QtGui.QApplication.translate("NAM", "Add WilCard Assignation", None, QtGui.QApplication.UnicodeUTF8))
        self.refreshShadersBtn.setText(QtGui.QApplication.translate("NAM", "Refresh Shaders", None, QtGui.QApplication.UnicodeUTF8))

