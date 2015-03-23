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
import json, os

from shaderManager.propertywidgets import property_widget
from shaderManager.propertywidgets import property_widget_bool
from shaderManager.propertywidgets import property_widget_bool2
from shaderManager.propertywidgets import property_widget_color 
from shaderManager.propertywidgets import property_widget_enum 
from shaderManager.propertywidgets import property_widget_float 
from shaderManager.propertywidgets import property_widget_float2 
from shaderManager.propertywidgets import property_widget_int 
from shaderManager.propertywidgets import property_widget_node 
from shaderManager.propertywidgets import property_widget_pointer 
from shaderManager.propertywidgets import property_widget_string 
from shaderManager.propertywidgets import property_widget_string2
from shaderManager.propertywidgets import property_widget_vector 
from shaderManager.propertywidgets import property_widget_visibility 

reload(property_widget)
reload(property_widget_bool)
reload(property_widget_color)
reload(property_widget_enum)
reload(property_widget_float)
reload(property_widget_int)
reload(property_widget_node)
reload(property_widget_pointer)
reload(property_widget_string)
reload(property_widget_vector)
reload(property_widget_visibility)

PROPERTY_ADD_LIST = {
'polymesh'       : [
                    {'name' :'forceVisible', 'type': AI_TYPE_BOOLEAN, 'value' : False},
                    {'name' :'sss_setname', 'type': AI_TYPE_STRING, 'value' : ""},
                  ],
'points'         : [
                    {'name' :'forceVisible', 'type': AI_TYPE_BOOLEAN, 'value' : False}, 
                    {'name' :'radius_multiplier', 'type': AI_TYPE_FLOAT, 'value' : 1.0},
                    {'name' :'velocity_multiplier', 'type': AI_TYPE_FLOAT, 'value' : 1.0}
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
        QWidget.__init__(self, parent)
        mainLayout = QVBoxLayout()
        self.setLayout(mainLayout)

        
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)),"arnold_nodes.json"), "r") as node_definition:
          self.nodes = json.load(node_definition)

        self.mainEditor = mainEditor
        
        name = nodetype
        self.node = self.nodes[name]

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
              propertyWidget = self.GetPropertyWidget(prop, frame, False)

              self.propertyWidgets[prop["name"]] = propertyWidget
              if propertyWidget:
                frameLayout.addWidget(propertyWidget)

          ## Built-in parameters
            for param in self.node["params"]:
  
                paramName = param["name"]

                blackList = PROPERTY_BLACK_LIST[name] if name in PROPERTY_BLACK_LIST else []
                if paramName != 'name' and not paramName in blackList:
                  propertyWidget = self.GetPropertyWidget(param, frame, False)
                  self.propertyWidgets[paramName] = propertyWidget
                  if propertyWidget:
                    frameLayout.addWidget(propertyWidget)


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


    def GetPropertyWidget(self, param, parent, userData):
      widget = None
      type = param["type"]
      if "visibility" in param["name"] or "sidedness" in param["name"]:
         widget = property_widget_visibility.PropertyWidgetVisibility(self, param, parent)
      elif type == AI_TYPE_BYTE:
         widget = property_widget_int.PropertyWidgetInt(self, param, parent)
      elif type == AI_TYPE_INT:
         widget = property_widget_int.PropertyWidgetInt(self, param, parent)
      elif type == AI_TYPE_UINT:
         widget = property_widget_int.PropertyWidgetInt(self, param,  parent)
      elif type == AI_TYPE_FLOAT:
         widget = property_widget_float.PropertyWidgetFloat(self, param, parent)
      elif type == AI_TYPE_VECTOR:
         widget = property_widget_vector.PropertyWidgetVector(self, param, PropertyWidget.VECTOR, parent)
      elif type == AI_TYPE_POINT:
         widget = property_widget_vector.PropertyWidgetVector(self, param, PropertyWidget.POINT, parent)
      elif type == AI_TYPE_POINT2:
         widget = property_widget_vector.PropertyWidgetVector(self, param, PropertyWidget.POINT2, parent)
      if type == AI_TYPE_BOOLEAN:
         widget = property_widget_bool.PropertyWidgetBool(self, param, parent)
      elif type == AI_TYPE_STRING:
         widget = property_widget_string.PropertyWidgetString(self, param, parent)
      elif type == AI_TYPE_ENUM:
         widget = property_widget_enum.PropertyWidgetEnum(self, param, parent)
      elif type == AI_TYPE_RGB:
         widget = property_widget_color.PropertyWidgetColor(self, param, PropertyWidget.RGB, parent)
      elif type == AI_TYPE_RGBA:
         widget = property_widget_color.PropertyWidgetColor(self, param, PropertyWidget.RGBA, parent)
      ##elif type == AI_TYPE_POINTER:
      ##   widget = PropertyWidgetPointer(nentry, name, parent)
      # elif type == AI_TYPE_NODE:
      #    widget = PropertyWidgetNode(self, pentry, name, parent)
      if widget and userData:
         widget.setBackgroundRole(QPalette.Base)
      return widget
