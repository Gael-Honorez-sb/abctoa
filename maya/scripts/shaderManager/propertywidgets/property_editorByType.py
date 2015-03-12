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

from PySide.QtGui import *
from PySide.QtCore import *

from shaderManager.propertywidgets.property_widget_bool import PropertyWidgetBool
from shaderManager.propertywidgets.property_widget_bool2 import PropertyWidgetBool2
from shaderManager.propertywidgets.property_widget_color import *
from shaderManager.propertywidgets.property_widget_enum import *
from shaderManager.propertywidgets.property_widget_float import PropertyWidgetFloat
from shaderManager.propertywidgets.property_widget_float2 import PropertyWidgetFloat2
from shaderManager.propertywidgets.property_widget_int import *
from shaderManager.propertywidgets.property_widget_node import *
from shaderManager.propertywidgets.property_widget_pointer import *
from shaderManager.propertywidgets.property_widget_string import *
from shaderManager.propertywidgets.property_widget_vector import *
from shaderManager.propertywidgets.property_widget_visibility import *
from arnold import *

PROPERTY_ADD_LIST = {
'polymesh'       : [
                    {'name' :'forceVisible', 'type': AI_TYPE_BOOLEAN, 'default' : False}
],
'points'         : [
                    {'name' :'forceVisible', 'type': AI_TYPE_BOOLEAN, 'default' : False}, 
                    {'name' :'radius_multiplier', 'type': AI_TYPE_FLOAT, 'default' : 1.0},
                    {'name' :'velocity_multiplier', 'type': AI_TYPE_FLOAT, 'default' : 1.0}
                    ],
}

PROPERTY_BLACK_LIST = {
'options'        : ['outputs'],
'polymesh'       : ['nidxs', 'nlist', 'nsides', 'shidxs', 'uvidxs', 'uvlist', 'vidxs', 'vlist', 'autobump_visibility', 'sidedness', 'ray_bias'],
'points'         : ['sidedness', 'ray_bias'],
'driver_display' : ['callback', 'callback_data']
}

class PropertyEditor(QWidget):
    propertyChanged = Signal(dict)
    setPropertyValue = Signal(dict)
    reset = Signal()
    def __init__(self, mainEditor, nodetype, parent = None):
        
        if not AiUniverseIsActive():
          AiBegin()

        QWidget.__init__(self, parent)
        mainLayout = QVBoxLayout()
        self.setLayout(mainLayout)

        self.mainEditor = mainEditor
        self.node = AiNodeEntryLookUp(nodetype)
        name = AiNodeEntryGetName (self.node) if self.node else "<No node selected>"

        labelLayout = QHBoxLayout()
        mainLayout.addLayout(labelLayout)
        labelLayout.addWidget(QLabel("Node: %s" % name))
        labelLayout.addStretch()

        #shaderButton = QPushButton("S", self)

        self.propertyWidgets = {}

        #labelLayout.addWidget(shaderButton)
        mainLayout.addSpacing(10)

        if self.node:
            frameLayout = QVBoxLayout()
            scrollArea = QScrollArea()
            scrollArea.setWidgetResizable(True)
            frame = QFrame()
            frame.setLayout(frameLayout)
            mainLayout.addWidget(scrollArea)

            for prop in PROPERTY_ADD_LIST[name]:
              propertyWidget = self.GetPropertyWidgetAdd(prop["name"], prop["type"], prop["default"], frame, False)

              self.propertyWidgets[prop["name"]] = propertyWidget
              if propertyWidget:
                frameLayout.addWidget(propertyWidget)

          ## Built-in parameters
            iter = AiNodeEntryGetParamIterator(self.node)
            while not AiParamIteratorFinished(iter):
                pentry = AiParamIteratorGetNext(iter)
                paramName = AiParamGetName(pentry)

                blackList = PROPERTY_BLACK_LIST[name] if name in PROPERTY_BLACK_LIST else []
                if paramName != 'name' and not paramName in blackList:
                  propertyWidget = self.GetPropertyWidget(str(paramName), AiParamGetType(pentry), pentry, frame, False)
                  self.propertyWidgets[paramName] = propertyWidget
                  if propertyWidget:
                    frameLayout.addWidget(propertyWidget)

            AiParamIteratorDestroy(iter)
            frameLayout.addStretch(0)

            scrollArea.setWidget(frame)
        else:
            mainLayout.addStretch()

        self.colorDialog = QColorDialog(self)

        if AiUniverseIsActive() and AiRendering() == False:
             AiEnd()

    def propertyValue(self, message):
      self.propertyWidgets[message["paramname"]].changed(message)
      self.propertyWidgets[message["paramname"]].title.setText("<font color='red'>%s</font>" % message["paramname"])

    def resetToDefault(self):
      self.reset.emit()
      for param in self.propertyWidgets:
        if hasattr(self.propertyWidgets[param], "title"):
          self.propertyWidgets[param].title.setText(param)

    def GetPropertyWidgetAdd(self, name, type, default, parent, userData):
      widget = None
      if type == AI_TYPE_BOOLEAN:
         widget = PropertyWidgetBool2(self, default, name, parent)
      elif type == AI_TYPE_FLOAT:
         widget = PropertyWidgetFloat2(self, default, name, parent)
      ##elif type == AI_TYPE_POINTER:
      ##   widget = PropertyWidgetPointer(nentry, name, parent)
      # elif type == AI_TYPE_NODE:
      #    widget = PropertyWidgetNode(self, pentry, name, parent)
      if widget and userData:
         widget.setBackgroundRole(QPalette.Base)
      return widget

    def GetPropertyWidget(self, name, type, pentry, parent, userData):
      widget = None
      if "visibility" in name or "sidedness" in name:
         widget = PropertyWidgetVisibility(self, pentry, name, parent)
      elif type == AI_TYPE_BYTE:
         widget = PropertyWidgetInt(self, pentry, name, parent)
      elif type == AI_TYPE_INT:
         widget = PropertyWidgetInt(self, pentry, name, parent)
      elif type == AI_TYPE_UINT:
         widget = PropertyWidgetInt(self, pentry, name,  parent)
      elif type == AI_TYPE_FLOAT:
         widget = PropertyWidgetFloat(self, pentry, name, parent)
      elif type == AI_TYPE_VECTOR:
         widget = PropertyWidgetVector(self, pentry, name, PropertyWidget.VECTOR, parent)
      elif type == AI_TYPE_POINT:
         widget = PropertyWidgetVector(self, pentry, name, PropertyWidget.POINT, parent)
      elif type == AI_TYPE_POINT2:
         widget = PropertyWidgetVector(self, pentry, name, PropertyWidget.POINT2, parent)
      if type == AI_TYPE_BOOLEAN:
         widget = PropertyWidgetBool(self, pentry, name, parent)
      elif type == AI_TYPE_STRING:
         widget = PropertyWidgetString(self, pentry, name, parent)
      elif type == AI_TYPE_ENUM:
         widget = PropertyWidgetEnum(self, pentry, name, AiParamGetEnum(pentry), parent)
      elif type == AI_TYPE_RGB:
         widget = PropertyWidgetColor(self, pentry, name, PropertyWidget.RGB, parent)
      elif type == AI_TYPE_RGBA:
         widget = PropertyWidgetColor(self, pentry, name, PropertyWidget.RGBA, parent)
      ##elif type == AI_TYPE_POINTER:
      ##   widget = PropertyWidgetPointer(nentry, name, parent)
      # elif type == AI_TYPE_NODE:
      #    widget = PropertyWidgetNode(self, pentry, name, parent)
      if widget and userData:
         widget.setBackgroundRole(QPalette.Base)
      return widget
