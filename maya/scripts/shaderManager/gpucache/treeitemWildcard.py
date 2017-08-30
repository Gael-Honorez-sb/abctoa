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

from Qt import QtGui, QtCore, QtWidgets
import functools
import os

import maya.mel as mel
import maya.cmds as cmds

import treeitem

TRANSFORM = 1
SHAPE = 2
SHADER = 3
WILDCARD = 4
DISPLACE = 5
TAG = 6

# Herited from normal tree item.
class wildCardItem(treeitem.abcTreeItem):
    def __init__(self, cache, expression, parent=None, *args, **kwargs):
        QtWidgets.QTreeWidgetItem.__init__(self, *args, **kwargs)
        self.interface = parent
        self.cache = cache
        self.expression = expression

        self.cacheAssignations = self.cache.getAssignations()

        self.shaderToAssign = ""

        self.isWildCard = True
        self.isTag = False
        self.protected = False
        self.hasChildren = False

        self.displayPath = "\"%s\"" % self.expression

        self.icon = 4

        self.shaderText = ""
        self.displaceText = ""
        self.attributeText = ""
                

    def removeAssigns(self):
        self.setText(1, "")
        self.setText(2, "")

    def setExpression(self, text):
        if self.protected:
            return

        path = self.getPath()
        # save current assignaitions
        shader = self.cache.assignations.getShader(path, self.interface.getLayer())
        displacement = self.cache.assignations.getDisplace(path, self.interface.getLayer())

        # remove previous assignations
        self.cache.assignDisplacement(path, None)
        self.cache.assignShader(path, None)
        
        # change the path
        self.expression = text
        self.displayPath = "\"%s\"" % self.expression

        # reassign the shaders
        if shader:
            self.cache.assignShader(self.expression, shader["shader"])
        if displacement:
            self.cache.assignDisplacement(self.expression, displacement["shader"])

        self.interface.checkShaders(self.interface.getLayer())
        self.interface.hierarchyWidget.resizeColumnToContents(1)
        self.interface.hierarchyWidget.resizeColumnToContents(2)


    def getPath(self):
        return self.expression

    def getAssignation(self):
        return self.expression

    def pressed(self):
        menu = QtWidgets.QMenu(self.interface)
        shader = self.interface.getShader()
        if shader:
            if  cmds.nodeType(shader) == "displacementShader":
                assignDisplacement = QtWidgets.QAction("Assign %s" % shader, menu)
                assignDisplacement.triggered.connect(self.assignDisplacement)
                self.shaderToAssign = shader
                menu.addAction(assignDisplacement)

            else:
                assignShader = QtWidgets.QAction("Assign %s" % shader, menu)
                assignShader.triggered.connect(self.assignShader)
                self.shaderToAssign = shader
                menu.addAction(assignShader)

        if len(self.cache.parent.shadersFromFile) > 0:
            menu.addSeparator()
            for sh in self.cache.parent.shadersFromFile:
                assignShader = QtWidgets.QAction("Assign %s" % sh, menu)
                assignShader.triggered.connect(functools.partial(self.assignShaderFromFile, sh))
                self.shaderToAssign = shader
                menu.addAction(assignShader)

        if len(self.cache.parent.displaceFromFile) > 0:
            menu.addSeparator()
            for sh in self.cache.parent.displaceFromFile:
                sh = sh.replace(".message", "")
                assignShader = QtWidgets.QAction("Assign displacement %s" % sh, menu)

                assignShader.triggered.connect(functools.partial(self.assignDisplacementFromFile, sh))
                self.shaderToAssign = shader
                menu.addAction(assignShader)

        path = self.getPath()
        menu.addSeparator()
        shader = self.cache.assignations.getShader(path, self.interface.getLayer())
        if shader:
            deassignShader = QtWidgets.QAction("Deassign %s" % shader["shader"], menu)
            deassignShader.triggered.connect(self.deassignShader)
            menu.addAction(deassignShader)

        menu.addSeparator()
        shader = self.cache.assignations.getDisplace(path, self.interface.getLayer())
        if shader:
                deassignShader = QtWidgets.QAction("Deassign displace %s" % shader["shader"], menu)
                deassignShader.triggered.connect(self.deassignDisplace)
                menu.addAction(deassignShader)

        menu.addSeparator()
        if not self.protected:
            deleteWildcard = QtWidgets.QAction("Delete WildCard", menu)
            deleteWildcard.triggered.connect(self.deleteWildcard)
            menu.addAction(deleteWildcard)

        menu.popup(QtGui.QCursor.pos())

    def deleteWildcard(self):
        # remove previous assignations
        self.cache.assignDisplacement(self.getPath(), None)
        self.cache.assignShader(self.getPath(), None)     
        self.interface.checkShaders(self.interface.getLayer())   
        #FIXME
        #Deleting item is kind of dangerous, let's hide for the moment.
        self.setHidden (True)

    def importinscene(self):
        pass
