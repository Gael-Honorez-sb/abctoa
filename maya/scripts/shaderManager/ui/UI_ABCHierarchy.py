# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 's:\projects\Gael_Testing\config\modules_2015\abcToA-2.0.0\scripts\shaderManager\ui\UI_ABCHierarchy.ui'
#
# Created: Fri Aug 19 15:16:53 2016
#      by: pyside-uic 0.2.15 running on Qt 1.2.4
#
# WARNING! All changes made in this file will be lost!

from Qt import QtCore, QtGui, QtWidgets

class Ui_NAM(object):
    def setupUi(self, NAM):
        NAM.setObjectName("NAM")
        NAM.resize(1600, 730)
        NAM.setMinimumSize(QtCore.QSize(1600, 0))
        NAM.setBaseSize(QtCore.QSize(1280, 0))
        self.centralwidget = QtWidgets.QWidget(NAM)
        self.centralwidget.setObjectName("centralwidget")
        self.gridLayout_2 = QtWidgets.QGridLayout(self.centralwidget)
        self.gridLayout_2.setObjectName("gridLayout_2")
        self.gridLayout = QtWidgets.QGridLayout()
        self.gridLayout.setObjectName("gridLayout")
        self.overrideShaders = QtWidgets.QCheckBox(self.centralwidget)
        self.overrideShaders.setObjectName("overrideShaders")
        self.gridLayout.addWidget(self.overrideShaders, 0, 0, 1, 1)
        self.overrideDisps = QtWidgets.QCheckBox(self.centralwidget)
        self.overrideDisps.setObjectName("overrideDisps")
        self.gridLayout.addWidget(self.overrideDisps, 0, 1, 1, 1)
        self.overrideProps = QtWidgets.QCheckBox(self.centralwidget)
        self.overrideProps.setObjectName("overrideProps")
        self.gridLayout.addWidget(self.overrideProps, 0, 2, 1, 1)
        spacerItem = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.gridLayout.addItem(spacerItem, 0, 3, 1, 1)
        self.renderLayer = QtWidgets.QComboBox(self.centralwidget)
        self.renderLayer.setMinimumSize(QtCore.QSize(150, 0))
        self.renderLayer.setObjectName("renderLayer")
        self.gridLayout.addWidget(self.renderLayer, 0, 5, 1, 1)
        self.splitter = QtWidgets.QSplitter(self.centralwidget)
        self.splitter.setOrientation(QtCore.Qt.Vertical)
        self.splitter.setObjectName("splitter")
        self.hierarchyWidget = QtWidgets.QTreeWidget(self.splitter)
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
        self.hierarchyWidget.setDragDropMode(QtWidgets.QAbstractItemView.DropOnly)
        self.hierarchyWidget.setAlternatingRowColors(True)
        self.hierarchyWidget.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self.hierarchyWidget.setRootIsDecorated(True)
        self.hierarchyWidget.setUniformRowHeights(False)
        self.hierarchyWidget.setColumnCount(3)
        self.hierarchyWidget.setObjectName("hierarchyWidget")
        self.hierarchyWidget.header().setVisible(True)
        self.gridLayout.addWidget(self.splitter, 3, 0, 1, 6)
        self.isolateCheckbox = QtWidgets.QCheckBox(self.centralwidget)
        self.isolateCheckbox.setObjectName("isolateCheckbox")
        self.gridLayout.addWidget(self.isolateCheckbox, 0, 4, 1, 1)
        self.gridLayout_2.addLayout(self.gridLayout, 0, 0, 1, 3)
        self.splitter_2 = QtWidgets.QSplitter(self.centralwidget)
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.Minimum)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.splitter_2.sizePolicy().hasHeightForWidth())
        self.splitter_2.setSizePolicy(sizePolicy)
        self.splitter_2.setOrientation(QtCore.Qt.Vertical)
        self.splitter_2.setChildrenCollapsible(True)
        self.splitter_2.setObjectName("splitter_2")
        self.filterShaderLineEdit = QtWidgets.QLineEdit(self.splitter_2)
        self.filterShaderLineEdit.setObjectName("filterShaderLineEdit")
        self.shadersList = QtWidgets.QListWidget(self.splitter_2)
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.shadersList.sizePolicy().hasHeightForWidth())
        self.shadersList.setSizePolicy(sizePolicy)
        self.shadersList.setMinimumSize(QtCore.QSize(0, 0))
        self.shadersList.setMaximumSize(QtCore.QSize(300, 16777215))
        self.shadersList.setBaseSize(QtCore.QSize(0, 0))
        self.shadersList.setDragDropMode(QtWidgets.QAbstractItemView.DragOnly)
        self.shadersList.setAlternatingRowColors(True)
        self.shadersList.setSpacing(2)
        self.shadersList.setObjectName("shadersList")
        self.displacementList = QtWidgets.QListWidget(self.splitter_2)
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Minimum)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.displacementList.sizePolicy().hasHeightForWidth())
        self.displacementList.setSizePolicy(sizePolicy)
        self.displacementList.setMaximumSize(QtCore.QSize(300, 16777215))
        self.displacementList.setSpacing(2)
        self.displacementList.setObjectName("displacementList")
        self.gridLayout_2.addWidget(self.splitter_2, 0, 3, 1, 1)
        self.wildCardButton = QtWidgets.QPushButton(self.centralwidget)
        self.wildCardButton.setMaximumSize(QtCore.QSize(200, 16777215))
        self.wildCardButton.setObjectName("wildCardButton")
        self.gridLayout_2.addWidget(self.wildCardButton, 1, 0, 1, 1)
        self.autoAssignButton = QtWidgets.QPushButton(self.centralwidget)
        self.autoAssignButton.setObjectName("autoAssignButton")
        self.gridLayout_2.addWidget(self.autoAssignButton, 1, 1, 1, 1)
        spacerItem1 = QtWidgets.QSpacerItem(983, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.gridLayout_2.addItem(spacerItem1, 1, 2, 1, 1)
        self.refreshManagerBtn = QtWidgets.QPushButton(self.centralwidget)
        self.refreshManagerBtn.setObjectName("refreshManagerBtn")
        self.gridLayout_2.addWidget(self.refreshManagerBtn, 1, 3, 1, 1)
        NAM.setCentralWidget(self.centralwidget)
        self.menubar = QtWidgets.QMenuBar(NAM)
        self.menubar.setGeometry(QtCore.QRect(0, 0, 1600, 21))
        self.menubar.setObjectName("menubar")
        NAM.setMenuBar(self.menubar)
        self.statusbar = QtWidgets.QStatusBar(NAM)
        self.statusbar.setObjectName("statusbar")
        NAM.setStatusBar(self.statusbar)

        self.retranslateUi(NAM)
        QtCore.QMetaObject.connectSlotsByName(NAM)

    def retranslateUi(self, NAM):
        NAM.setWindowTitle(QtWidgets.QApplication.translate("NAM", "Nozon Alembic Cache Manager", None))
        self.overrideShaders.setText(QtWidgets.QApplication.translate("NAM", "Override All Shaders", None))
        self.overrideDisps.setText(QtWidgets.QApplication.translate("NAM", "Override all displacement shaders", None))
        self.overrideProps.setText(QtWidgets.QApplication.translate("NAM", "Override all object properties", None))
        self.hierarchyWidget.setSortingEnabled(False)
        self.hierarchyWidget.headerItem().setText(0, QtWidgets.QApplication.translate("NAM", "ABC Hierarchy", None))
        self.hierarchyWidget.headerItem().setText(1, QtWidgets.QApplication.translate("NAM", "shaders", None))
        self.hierarchyWidget.headerItem().setText(2, QtWidgets.QApplication.translate("NAM", "displacement", None))
        self.isolateCheckbox.setText(QtWidgets.QApplication.translate("NAM", "Isolate Selected", None))
        self.shadersList.setSortingEnabled(True)
        self.displacementList.setSortingEnabled(True)
        self.wildCardButton.setText(QtWidgets.QApplication.translate("NAM", "Add WildCard Assignation", None))
        self.autoAssignButton.setText(QtWidgets.QApplication.translate("NAM", "Auto-Assign Shaders to Selected Tags", None))
        self.refreshManagerBtn.setText(QtWidgets.QApplication.translate("NAM", "Refresh Manager", None))

