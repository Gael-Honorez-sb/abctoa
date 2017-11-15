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

from PySide2 import QtWidgets, QtGui, QtCore
import functools
import os

import maya.mel as mel
import maya.cmds as cmds

import treeitem
import autoAssignRule

TRANSFORM = 1
SHAPE = 2
SHADER = 3
WILDCARD = 4
DISPLACE = 5
TAG = 6

# Herited from normal tree item.
class tagItem(treeitem.abcTreeItem):
    def __init__(self, cache, tag, parent=None, *args, **kwargs):
        QtWidgets.QTreeWidgetItem.__init__(self, *args, **kwargs)
        self.interface = parent
        self.cache = cache
        self.tag = tag

        self.cacheAssignations = self.cache.getAssignations()

        self.shaderToAssign = ""

        self.isWildCard = False
        self.isTag = True
        self.protected = False
        self.hasChildren = False

        self.displayPath = "\"%s\"" % self.tag

        self.icon = TAG

        self.shaderText = ""
        self.displaceText = ""
        self.attributeText = ""
                

    def removeAssigns(self):
        self.setText(1, "")
        self.setText(2, "")


    def getPath(self):
        return self.tag

    def getAssignation(self):
        return self.tag

    def autoAssignShader(self):
        ''' 
        Assign the shader returned by the auto Assign rule.
        '''
        path = self.getPath()
        reload(autoAssignRule)
        shader = autoAssignRule.applyRules(path)
        if shader is not None:
            self.cache.assignShader(path, shader)
            self.interface.checkShaders(self.interface.getLayer(), item=self)
            self.interface.hierarchyWidget.resizeColumnToContents(1)


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


        menu.popup(QtGui.QCursor.pos())


    def importinscene(self):
        pass
