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

TRANSFORM = 1
SHAPE = 2
SHADER = 3
WILDCARD = 4
DISPLACE = 5

class abcTreeItem(QtGui.QTreeWidgetItem):
    def __init__(self, cache, path, itemType, parent=None, *args, **kwargs):
        QtGui.QTreeWidgetItem.__init__(self, *args, **kwargs)
        self.interface = parent
        self.cache = cache
        self.path = path

        self.isWildCard = False
        self.hasChildren = False

        self.cacheAssignations = self.cache.getAssignations()

        self.displayPath = ""

        self.shaderToAssign = ""
        if len(self.path) == 0:

            self.displayPath = "/%s" % os.path.basename(self.cache.ABCcache)
        else:
            self.displayPath = self.path[-1]

        #self.interface.hierarchyWidget.resizeColumnToContents(0)

        self.icon = None
        
        if itemType == "Transform":
            self.icon= TRANSFORM
        else:
            self.icon = SHAPE

        self.shaderText = ""
        self.displaceText = ""
        self.attributeText = ""

        #self.setIcon(0, icon)

        # icon2 = QtGui.QIcon()
        # icon2.addFile(os.path.join(d, "../../../icons/sg.xpm"), QtCore.QSize(25,25))
        # self.setIcon(1, icon2)

    def setHasChildren(self, hasChildren):
        self.hasChildren = hasChildren

    def hasChidren(self):
        return self.hasChildren

    def getDisplayPath(self):
        return self.displayPath

    def display(self, column):
        if column == 0 :
            text = self.getDisplayPath()

            if self.icon == SHAPE or self.icon == WILDCARD:    
                ### display all attributes
                text += "<br>" + self.attributeText

            return text

        elif column == 1 :
            return self.shaderText

        elif column == 2 :
            return self.displaceText
        

        #shaderFromMainLayer = False
        #if shader.get("fromfile", False) or shaderFromMainLayer:


    def getIcon(self, column):
        if column == 0 :
            return self.icon
        if column == 2:
            return DISPLACE
        else:
            return SHADER
 
    def data(self, column, role):
        if role == QtCore.Qt.DisplayRole:
            return self.display(column)  
        elif role == QtCore.Qt.UserRole :
            return self.getIcon(column)
        return super(abcTreeItem, self).data(column, role)


    def removeAssigns(self):
        self.setText(1, "")
        self.shaderText = ""
        self.setText(2, "")
        self.displaceText = ""

    def getPath(self):
        return "/" + "/".join(self.path)



    def assignShaderFromFile(self, shader):
        self.shaderToAssign = shader
        self.assignShader()

    def assignDisplacementFromFile(self, shader):
        self.shaderToAssign = shader
        self.assignDisplacement()

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
        importinscene= QtGui.QAction("Import in Scene", menu)
        importinscene.triggered.connect(self.importinscene)
        menu.addAction(importinscene)


        menu.popup(QtGui.QCursor.pos())

    def importinscene(self):
        cmd = 'AbcImport  -ft "^%s$" "%s"' % (self.path[-1], self.cache.ABCcache.replace(os.path.sep, "/"))
        try:
            mel.eval(cmd)
        except:
            print "Error running", cmd

    def assignDisplacement(self):
        path = self.getPath()
        shaderName = str(self.shaderToAssign)+".message"
        self.cache.assignDisplacement(path, shaderName)


        selectedItems = self.interface.hierarchyWidget.selectedItems()
        if len(selectedItems) > 1:
            for item in selectedItems:
                if item != self:
                    item.cache.assignDisplacement(item.getPath(), shaderName)


        self.interface.checkShaders(self.interface.getLayer())


    def assignShader(self):

        if not cmds.objExists(str(self.shaderToAssign)):
            if self.shaderToAssign.endswith("SG") and cmds.objExists(str(self.shaderToAssign)[:-2]):
                    self.shaderToAssign = self.interface.createSG(str(self.shaderToAssign)[:-2])
            else:
                return
        
        if not cmds.nodeType(self.shaderToAssign) == "shadingEngine":
            self.shaderToAssign = self.interface.createSG(str(self.shaderToAssign))

        path = self.getPath()

        self.cache.assignShader(path, self.shaderToAssign)


        selectedItems = self.interface.hierarchyWidget.selectedItems()
        if len(selectedItems) > 1:
            for item in selectedItems:
                if item != self:
                    item.cache.assignShader(item.getPath(), self.shaderToAssign)


        self.interface.checkShaders(self.interface.getLayer())
        self.interface.hierarchyWidget.resizeColumnToContents(1)


    def deassignDisplace(self):
        path = self.getPath()

        self.cache.assignDisplacement(path, None)


        selectedItems = self.interface.hierarchyWidget.selectedItems()
        if len(selectedItems) > 1:
            for item in selectedItems:
                if item != self:
                    item.cache.assignDisplacement(item.getPath(), None)


        self.interface.checkShaders(self.interface.getLayer())

        self.interface.hierarchyWidget.resizeColumnToContents(2)


    def deassignShader(self):
        path = self.getPath()

        self.cache.assignShader(path, None)


        selectedItems = self.interface.hierarchyWidget.selectedItems()
        if len(selectedItems) > 1:
            for item in selectedItems:
                if item != self:
                    item.cache.assignShader(item.getPath(), None)


        self.interface.checkShaders(self.interface.getLayer())
        self.interface.hierarchyWidget.resizeColumnToContents(1)

    def checkShaders(self, layer=None):
        path = self.getPath()
        shader = self.cacheAssignations.getShader(path, layer)
        displace = self.cacheAssignations.getDisplace(path, layer)

        shaderFromMainLayer = False
        displaceFromMainLayer = False

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
            self.shaderText = shader.get("shader")

            if shader.get("fromfile", False) or shaderFromMainLayer:
                self.setForeground(1, QtCore.Qt.darkGray)
                self.shaderText = "<font color='#848484'>%s</font>" % self.shaderText
            else:
                self.setForeground(1, QtCore.Qt.white)

            font = QtGui.QFont()
            if shader.get("inherited", False):
                self.setForeground(1, QtCore.Qt.darkGray)
                font.setItalic(1)
                font.setBold (0)
                self.shaderText = "<font color='#848484'><i>%s</i></font>" % self.shaderText
            else:
                font.setItalic(0)
                font.setBold (1)
                self.shaderText = "<b>%s</b>" % self.shaderText
            self.setFont( 1,  font )

            

        else:
            self.setText(1, "")
            self.shaderText = ""


        if displace:
            self.setText(2, displace.get("shader"))
            self.displaceText = displace.get("shader").replace(".message", "")

            if displace.get("fromfile", False) or displaceFromMainLayer:
                self.setForeground(2, QtCore.Qt.darkGray)
                self.displaceText = "<font color='#848484'>%s</font>" % self.displaceText
            else:
                self.setForeground(2, QtCore.Qt.white)

            font = QtGui.QFont()
            if displace.get("inherited", False):
                self.setForeground(2, QtCore.Qt.darkGray)
                font.setItalic(1)
                font.setBold (0)
                self.displaceText = "<font color='#848484'><i>%s</i></font>" % self.displaceText
            else:
                font.setItalic(0)
                font.setBold (1)
                self.displaceText = "<b>%s</b>" % self.displaceText

            self.setFont( 2,  font )
            

        else:
            self.setText(2, "")
            self.displaceText = ""



    def checkProperties(self, layer=None):
        path = self.getPath()
        attributes = self.cacheAssignations.getOverrides(path, layer)
        attributesFromMainLayer = False
        layerOverrides = None

        if layer:
            layerOverrides = self.cacheAssignations.getLayerOverrides(layer)
            if not layerOverrides:
                layerOverrides = dict(removeDisplacements=False, removeProperties=False, removeShaders=False)            
        
        if not attributes and layer != None and layerOverrides["removeProperties"] == False:
            attributes = self.cacheAssignations.getShader(path, None)
            if attributes:
                attributesFromMainLayer = True    

        if attributes:
            self.attributeText = ""
            attributeTextTooltip = "<u><b>Attributes:</u><b><br>"
            fromFile = False
            inherited = attributes.get("inherited", False)

            if attributes.get("fromfile", False) or attributesFromMainLayer:
                fromFile = True

            attributesList = attributes.get("overrides", [])

            for attribute in attributesList:
                color = "#FFFFFF"
                colortip = "#000000"
                if fromFile or inherited:
                    color = "#848484"
                    colortip = "#848484"
                tag = "b"
                if inherited:
                    tag= "i"

                self.attributeText += "<br><font color='%s'><%s><u>%s</u> : %s</%s></font>" % (color, tag, attribute, str(attributesList[attribute]), tag)
                attributeTextTooltip += "<br><font color='%s'><%s>%s : %s</%s></font>" % (colortip, tag, attribute, str(attributesList[attribute]), tag)

        else:
            self.attributeText = ""
            attributeTextTooltip = ""

        self.setToolTip(0, attributeTextTooltip)        

        self.interface.hierarchyWidget.resizeColumnToContents(0)