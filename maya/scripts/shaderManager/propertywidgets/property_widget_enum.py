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

from Qt.QtGui import *
from Qt.QtCore import *
from Qt.QtWidgets import *
from arnold import *
from property_widget import PropertyWidget

class PropertyWidgetEnum(PropertyWidget):
   def __init__(self, controller,  param, parent = None):
      PropertyWidget.__init__(self, param, parent)

      self.paramName = param["name"]

      self.widget = QComboBox(self)

      self.controller = controller
      self.controller.setPropertyValue.connect(self.changed)
      self.controller.reset.connect(self.resetValue)

      self.default = param["value"]["default"]

      self.widget.setCurrentIndex(self.default)

      for value in param["value"]["values"]:
         self.widget.addItem(value)



      self.widget.currentIndexChanged.connect(self.ValueChanged)
      self.layout().addWidget(self.widget)

   def ValueChanged(self, value):
    self.controller.mainEditor.propertyChanged(dict(propname=self.paramName, default=value == self.default, value=value))

   def changed(self, message):

      value = message["value"]
      self.widget.setCurrentIndex(value)


   def resetValue(self):
      self.widget.setCurrentIndex(self.default)
