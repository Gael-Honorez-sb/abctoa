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
from arnold import *
from property_widget import *
class PropertyWidgetFloat(PropertyWidget):
   def __init__(self, controller,  param, parent = None):
      PropertyWidget.__init__(self, param, parent)

      self.paramName = param["name"]

      self.controller = controller
      self.controller.setPropertyValue.connect(self.changed)
      self.controller.reset.connect(self.resetValue)

      self.default = param["value"]

      self.widget = QDoubleSpinBox()
      self.widget.setValue(self.default)
      self.widget.setSingleStep(0.1)
      self.widget.setMaximum(9999)
      self.widget.setMinimum(-999)
      # if AiMetaDataGetFlt(nentry, name, "min", byref(data)):
      #    self.widget.setMinimum(data.value)
      # if AiMetaDataGetFlt(nentry, name, "max", byref(data)):
      #    self.widget.setMaximum(data.value)
      # if AiMetaDataGetFlt(nentry, name, "self.default", byref(data)):
      #    self.widget.setValue(data.value)
      #    self.ValueChanged(data.value)
      # else:
      #    self.__ReadFromArnold()

      #self.widget.editingFinished.connect(self.ValueChanged)
      self.widget.valueChanged.connect(self.ValueChanged)

      self.layout().addWidget(self.widget)
   def ValueChanged(self, value):
      self.controller.mainEditor.propertyChanged(dict(propname=self.paramName, default=value == self.default, value=value))


   def changed(self, message):

      value = message["value"]
      self.widget.setValue(value)


   def resetValue(self):
      self.widget.setValue(self.default)