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

from PySide.QtGui import *
from PySide.QtCore import *
from shaderManager.propertywidgets.property_widget_bool import PropertyWidgetBool
from shaderManager.propertywidgets.property_widget_bool2 import PropertyWidgetBool2
from shaderManager.propertywidgets.property_widget_color import *
from shaderManager.propertywidgets.property_widget_enum import *
from shaderManager.propertywidgets.property_widget_float import *
from shaderManager.propertywidgets.property_widget_int import *
from shaderManager.propertywidgets.property_widget_node import *
from shaderManager.propertywidgets.property_widget_pointer import *
from shaderManager.propertywidgets.property_widget_string import *
from shaderManager.propertywidgets.property_widget_vector import *
from shaderManager.propertywidgets.property_widget_visibility import *
from arnold import *

PROPERTY_BLACK_LIST = {
'options'        : ['outputs'],
'polymesh'       : ['nidxs', 'nlist', 'nsides', 'shidxs', 'uvidxs', 'uvlist', 'vidxs', 'vlist', 'autobump_visibility', 'sidedness', 'ray_bias', 'use_light_group', 'use_shadow_group'],
'driver_display' : ['callback', 'callback_data']
}

class PropertyEditor(QWidget):
    propertyChanged = Signal(dict)
    setPropertyValue = Signal(dict)
    reset = Signal()
    def __init__(self, mainEditor, nodetype, parent = None):
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

            #blackhole matte
            propertyWidget = PropertyWidgetBool2(self, False, "matte", frame)
            self.propertyWidgets["matte"] = propertyWidget
            if propertyWidget:
              frameLayout.addWidget(propertyWidget)

            propertyWidget2 = PropertyWidgetBool2(self, False, "forceVisible", frame)
            self.propertyWidgets["forceVisible"] = propertyWidget2
            if propertyWidget2:
              frameLayout.addWidget(propertyWidget2)

          ## Built-in parameters
            iter = AiNodeEntryGetParamIterator(self.node)
            while not AiParamIteratorFinished(iter):
                pentry = AiParamIteratorGetNext(iter)
                paramName = AiParamGetName(pentry)

                blackList = PROPERTY_BLACK_LIST[name] if name in PROPERTY_BLACK_LIST else []
                if paramName != 'name' and not paramName in blackList:
                  propertyWidget = self.GetPropertyWidget(self.node, str(paramName), AiParamGetType(pentry), pentry, frame, False)
                  self.propertyWidgets[paramName] = propertyWidget
                  if propertyWidget:
                    frameLayout.addWidget(propertyWidget)

            AiParamIteratorDestroy(iter)
            frameLayout.addStretch(0)

            scrollArea.setWidget(frame)
        else:
            mainLayout.addStretch()

        self.colorDialog = QColorDialog(self)

    def propertyValue(self, message):
      self.propertyWidgets[message["paramname"]].changed(message)
      self.propertyWidgets[message["paramname"]].title.setText("<font color='red'>%s</font>" % message["paramname"])

    def resetToDefault(self):
      self.reset.emit()
      for param in self.propertyWidgets:
        if hasattr(self.propertyWidgets[param], "title"):
          self.propertyWidgets[param].title.setText(param)

    def GetPropertyWidget(self, nentry, name, type, pentry, parent, userData):
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
