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
from property_widget import *

class PropertyWidgetInt(PropertyWidget):

   def __init__(self, controller,  param, parent = None):
      PropertyWidget.__init__(self, param, parent)


      self.paramName = param["name"]

      self.controller = controller
      self.controller.setPropertyValue.connect(self.changed)
      self.controller.reset.connect(self.resetValue)

      self.default = param["value"]


      self.widget = QSpinBox()
      self.widget.setMaximum(65535)
      self.widget.setValue(self.default)
      self.widget.valueChanged.connect(self.ValueChanged)

      self.layout().addWidget(self.widget)

   def ValueChanged(self, value):
      self.controller.mainEditor.propertyChanged(dict(propname=self.paramName, default=value == self.default, value=value))


   def changed(self, message):

      value = message["value"]
      self.widget.setValue(value)


   def resetValue(self):
    self.widget.setValue(self.default)



