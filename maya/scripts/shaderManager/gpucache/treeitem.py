# Alembic Holder
# Copyright (c) 2014, Gael Honorez, All rights reserved.
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

from PySide import QtGui, QtCore
import os
import functools

import maya.mel as mel
import maya.cmds as cmds

# class MyDelegate(QtGui.QItemDelegate):
#     def __init__(self):
#         QtGui.QItemDelegate.__init__(self)

#     def sizeHint(self, option, index):
#         return QtGui.QSize(32,32)

class abcTreeItem(QtGui.QTreeWidgetItem):
    def __init__(self, cache, path, itemType, parent=None, *args, **kwargs):
        QtGui.QTreeWidgetItem.__init__(self, *args, **kwargs)
        self.parent = parent
        self.cache = cache
        self.path = path

        self.cacheAssignations = self.cache.getAssignations()

        self.shaderToAssign = ""
        if len(self.path) == 0:
            self.setText(0, "/%s" % os.path.basename(self.cache.ABCcache))
        else:
            self.setText(0, self.path[-1])

        #self.parent.hierarchyWidget.resizeColumnToContents(0)

        icon = QtGui.QIcon()

        d = os.path.dirname(__file__)

        if itemType == "Transform":


            icon.addFile(os.path.join(d, "../icons/group.png"), QtCore.QSize(22,22))
        else:
            icon.addFile(os.path.join(d, "../icons/shape.png"), QtCore.QSize(22,22))

        self.setIcon(0, icon)

        icon2 = QtGui.QIcon()
        icon2.addFile(os.path.join(d, "../icons/sg.xpm"), QtCore.QSize(25,25))
        self.setIcon(1, icon2)


    def removeAssigns(self):
        self.setText(1, "")
        self.setText(2, "")

    def getPath(self):
        return "/" + "/".join(self.path)


    def assignShaderFromFile(self, shader):
        self.shaderToAssign = shader
        self.assignShader()

    def assignDisplacementFromFile(self, shader):
        self.shaderToAssign = shader
        self.assignDisplacement()

    def pressed(self):
        menu = QtGui.QMenu(self.parent)
        shader = self.parent.getShader()
        if shader:
            if  cmds.nodeType(shader) == "displacementShader":
                assignDisplacement = QtGui.QAction("Assign %s" % shader, menu)
                assignDisplacement.triggered.connect(self.assignDisplacement)
                self.shaderToAssign = shader
                menu.addAction(assignDisplacement)

            else:
                assignShader = QtGui.QAction("Assign %s" % shader, menu)
                assignShader.triggered.connect(self.assignShader)
                self.shaderToAssign = shader
                menu.addAction(assignShader)

        if len(self.cache.parent.shadersFromFile) > 0:
            menu.addSeparator()
            for sh in self.cache.parent.shadersFromFile:
                assignShader = QtGui.QAction("Assign %s" % sh, menu)
                assignShader.triggered.connect(functools.partial(self.assignShaderFromFile, sh))
                self.shaderToAssign = shader
                menu.addAction(assignShader)

        if len(self.cache.parent.displaceFromFile) > 0:
            menu.addSeparator()
            for sh in self.cache.parent.displaceFromFile:
                sh = sh.replace(".message", "")
                assignShader = QtGui.QAction("Assign displacement %s" % sh, menu)

                assignShader.triggered.connect(functools.partial(self.assignDisplacementFromFile, sh))
                self.shaderToAssign = shader
                menu.addAction(assignShader)

        path = self.getPath()
        menu.addSeparator()
        shader = self.cache.assignations.getShader(path, self.parent.getLayer())
        if shader:
            deassignShader = QtGui.QAction("Deassign %s" % shader["shader"], menu)
            deassignShader.triggered.connect(self.deassignShader)
            menu.addAction(deassignShader)

        menu.addSeparator()
        shader = self.cache.assignations.getDisplace(path, self.parent.getLayer())
        if shader:
                deassignShader = QtGui.QAction("Deassign displace %s" % shader["shader"], menu)
                deassignShader.triggered.connect(self.deassignDisplace)
                menu.addAction(deassignShader)

        menu.addSeparator()
        importinscene= QtGui.QAction("Import in Scene", menu)
        importinscene.triggered.connect(self.importinscene)
        menu.addAction(importinscene)


        menu.popup(QtGui.QCursor.pos())

    def importinscene(self):
        print self.path[-1]
        print self.cache.ABCcache
        cmd = 'AbcImport  -ft "^%s$" "%s"' % (self.path[-1], self.cache.ABCcache)
        try:
            mel.eval(cmd)
        except:
            print "Error running", cmd

    def assignDisplacement(self):
        path = self.getPath()
        shaderName = str(self.shaderToAssign)+".message"
        self.cache.assignDisplacement(path, shaderName)


        selectedItems = self.parent.hierarchyWidget.selectedItems()
        if len(selectedItems) > 1:
            for item in selectedItems:
                if item != self:
                    item.cache.assignDisplacement(item.getPath(), shaderName)


        self.parent.checkShaders(self.parent.getLayer())


    def assignShader(self):

        if not cmds.objExists(str(self.shaderToAssign)):
            if self.shaderToAssign.endswith("SG") and cmds.objExists(str(self.shaderToAssign)[:-2]):
                    self.shaderToAssign = self.parent.createSG(str(self.shaderToAssign)[:-2])
            else:
                return

        path = self.getPath()

        self.cache.assignShader(path, self.shaderToAssign)


        selectedItems = self.parent.hierarchyWidget.selectedItems()
        if len(selectedItems) > 1:
            for item in selectedItems:
                if item != self:
                    item.cache.assignShader(item.getPath(), self.shaderToAssign)


        self.parent.checkShaders(self.parent.getLayer())
        self.parent.hierarchyWidget.resizeColumnToContents(1)


    def deassignDisplace(self):
        path = self.getPath()

        self.cache.assignDisplacement(path, None)


        selectedItems = self.parent.hierarchyWidget.selectedItems()
        if len(selectedItems) > 1:
            for item in selectedItems:
                if item != self:
                    item.cache.assignDisplacement(item.getPath(), None)


        self.parent.checkShaders(self.parent.getLayer())

        self.parent.hierarchyWidget.resizeColumnToContents(2)


    def deassignShader(self):
        path = self.getPath()

        self.cache.assignShader(path, None)


        selectedItems = self.parent.hierarchyWidget.selectedItems()
        if len(selectedItems) > 1:
            for item in selectedItems:
                if item != self:
                    item.cache.assignShader(item.getPath(), None)


        self.parent.checkShaders(self.parent.getLayer())
        self.parent.hierarchyWidget.resizeColumnToContents(1)

    def checkShaders(self, layer=None):
        path = self.getPath()

        shader = self.cacheAssignations.getShader(path, layer)
        displace = self.cacheAssignations.getDisplace(path, layer)

        shaderFromMainLayer = False
        displaceFromMainLayer = False
        layerOverrides = None
        if layer:
            layerOverrides = self.cacheAssignations.getLayerOverrides(layer)
            if not layerOverrides:
                layerOverrides = dict(removeDisplacements=False, removeProperties=False, removeShaders=False)

        if not shader and layer != None and layerOverrides["removeShaders"] == False:
            shader = self.cacheAssignations.getShader(path, None)
            if shader:
                shaderFromMainLayer = True


        if not displace and layer != None and layerOverrides["removeDisplacements"] == False:
            displace = self.cacheAssignations.getDisplace(path, None)
            if displace:
                displaceFromMainLayer = True

        if shader:
            self.setText(1, shader.get("shader"))
            if shader.get("fromfile", False) or shaderFromMainLayer:
                self.setForeground(1, QtCore.Qt.darkGray)
            else:
                self.setForeground(1, QtCore.Qt.white)

            font = QtGui.QFont()
            if shader.get("inherited", False):
                self.setForeground(1, QtCore.Qt.darkGray)
                font.setItalic(1)
                font.setBold (0)
            else:
                font.setItalic(0)
                font.setBold (1)
            self.setFont( 1,  font )

        else:
            self.setText(1, "")


        if displace:
            self.setText(2, displace.get("shader"))
            if displace.get("fromfile", False) or displaceFromMainLayer:
                self.setForeground(2, QtCore.Qt.darkGray)
            else:
                self.setForeground(2, QtCore.Qt.white)

            font = QtGui.QFont()
            if shader.get("inherited", False):
                self.setForeground(2, QtCore.Qt.darkGray)
                font.setItalic(1)
                font.setBold (0)
            else:
                font.setItalic(0)
                font.setBold (1)
            self.setFont( 2,  font )

        else:
            self.setText(2, "")
