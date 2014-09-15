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
import functools
import os

import maya.mel as mel
import maya.cmds as cmds

import treeitem

# Herited from normal tree item.
class wildCardItem(treeitem.abcTreeItem):
    def __init__(self, cache, expression, parent=None, *args, **kwargs):
        QtGui.QTreeWidgetItem.__init__(self, *args, **kwargs)
        self.interface = parent
        self.cache = cache
        self.expression = expression

        self.cacheAssignations = self.cache.getAssignations()

        self.shaderToAssign = ""

        self.isWildCard = True
        self.protected = False

        self.setText(0, "\"%s\"" % self.expression)

        d = os.path.dirname(__file__)        

        icon = QtGui.QIcon()
        icon.addFile(os.path.join(d, "../../../icons/wildcard.png"), QtCore.QSize(22,22))
        self.setIcon(0, icon)


        icon2 = QtGui.QIcon()
        icon2.addFile(os.path.join(d, "../../../icons/sg.xpm"), QtCore.QSize(25,25))
        self.setIcon(1, icon2)


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
        self.setText(0, "\"%s\"" % self.expression)

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
        menu = QtGui.QMenu(self.interface)
        shader = self.interface.getShader()
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
        shader = self.cache.assignations.getShader(path, self.interface.getLayer())
        if shader:
            deassignShader = QtGui.QAction("Deassign %s" % shader["shader"], menu)
            deassignShader.triggered.connect(self.deassignShader)
            menu.addAction(deassignShader)

        menu.addSeparator()
        shader = self.cache.assignations.getDisplace(path, self.interface.getLayer())
        if shader:
                deassignShader = QtGui.QAction("Deassign displace %s" % shader["shader"], menu)
                deassignShader.triggered.connect(self.deassignDisplace)
                menu.addAction(deassignShader)

        menu.addSeparator()
        if not self.protected:
            deleteWildcard = QtGui.QAction("Delete WildCard", menu)
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
