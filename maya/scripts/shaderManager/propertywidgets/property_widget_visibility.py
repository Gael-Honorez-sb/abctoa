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


class PropertyWidgetVisibility(PropertyWidget):
  def __init__(self, controller, params, parent = None):
    PropertyWidget.__init__(self, params, parent)

    self.paramName = params["name"]

    self.minVal = AI_RAY_ALL & ~(AI_RAY_GLOSSY|AI_RAY_DIFFUSE|AI_RAY_SUBSURFACE|AI_RAY_REFRACTED|AI_RAY_REFLECTED|AI_RAY_SHADOW|AI_RAY_CAMERA)

    self.controller = controller
    self.controller.setPropertyValue.connect(self.changed)
    self.controller.reset.connect(self.resetValue)

    self.allSwitch = QPushButton(self)
    self.allSwitch.setText("All Off")
    self.allSwitch.pressed.connect(self.switchPressed)

    self.switch = True

    self.viz = {}
    self.viz["camera"] = QCheckBox(self)
    self.viz["cast_shadows"] = QCheckBox(self)
    self.viz["diffuse"] = QCheckBox(self)
    self.viz["glossy"] = QCheckBox(self)
    self.viz["reflection"] = QCheckBox(self)
    self.viz["refraction"] = QCheckBox(self)
    self.viz["subsurface"] = QCheckBox(self)

    self.default = params["value"]
    self.setViz(self.default)

    grid= QGridLayout()
    grid.addWidget(self.allSwitch, 1, 1)
    row = 2
    for v in self.viz:
      label = QLabel(v)
      grid.addWidget(label, row, 1)
      grid.addWidget(self.viz[v], row, 2)
      self.viz[v].stateChanged.connect(self.PropertyChanged)
      row = row + 1


    self.layout().addLayout(grid)

  def switchPressed(self):
    if self.switch:
        self.setViz(self.minVal)
        self.allSwitch.setText("All On")
        self.switch = False
    else:
        self.setViz(255)
        self.allSwitch.setText("All Off")
        self.switch = True

  def computeViz(self):
    value = AI_RAY_ALL

    if not self.viz["camera"].isChecked():
        value &= ~AI_RAY_CAMERA
    if not self.viz["cast_shadows"].isChecked():
        value &= ~AI_RAY_SHADOW
    if not self.viz["diffuse"].isChecked():
        value &= ~AI_RAY_DIFFUSE
    if not self.viz["glossy"].isChecked():
        value &= ~AI_RAY_GLOSSY
    if not self.viz["reflection"].isChecked():
        value &= ~AI_RAY_REFLECTED
    if not self.viz["refraction"].isChecked():
        value &= ~AI_RAY_REFRACTED
    if not self.viz["subsurface"].isChecked():
        value &= ~AI_RAY_SUBSURFACE

    return value

  def setViz(self, value):

    if value <= self.minVal:
        self.allSwitch.setText("All On")
        self.switch = False
    else:
        self.allSwitch.setText("All Off")
        self.switch = True

    for v in self.viz:
      self.viz[v].setChecked(0)

    if value > AI_RAY_ALL & ~AI_RAY_GLOSSY:
        self.viz["glossy"].setChecked(1)
        value = value - AI_RAY_GLOSSY
    if value > AI_RAY_ALL & ~(AI_RAY_GLOSSY|AI_RAY_DIFFUSE):
        self.viz["diffuse"].setChecked(1)
        value = value - AI_RAY_DIFFUSE
    if value > AI_RAY_ALL & ~(AI_RAY_GLOSSY|AI_RAY_DIFFUSE|AI_RAY_SUBSURFACE):
        self.viz["subsurface"].setChecked(1)
        value = value - AI_RAY_SUBSURFACE        
    if value > AI_RAY_ALL & ~(AI_RAY_GLOSSY|AI_RAY_DIFFUSE|AI_RAY_SUBSURFACE|AI_RAY_REFRACTED):
        self.viz["refraction"].setChecked(1)
        value = value - AI_RAY_REFRACTED
    if value > AI_RAY_ALL & ~(AI_RAY_GLOSSY|AI_RAY_DIFFUSE|AI_RAY_SUBSURFACE|AI_RAY_REFRACTED|AI_RAY_REFLECTED):
        self.viz["reflection"].setChecked(1)
        value = value - AI_RAY_REFLECTED
    if value > AI_RAY_ALL & ~(AI_RAY_GLOSSY|AI_RAY_DIFFUSE|AI_RAY_SUBSURFACE|AI_RAY_REFRACTED|AI_RAY_REFLECTED|AI_RAY_SHADOW):
        self.viz["cast_shadows"].setChecked(1)
        value = value - AI_RAY_SHADOW
    if value > AI_RAY_ALL & ~(AI_RAY_GLOSSY|AI_RAY_DIFFUSE|AI_RAY_SUBSURFACE|AI_RAY_REFRACTED|AI_RAY_REFLECTED|AI_RAY_SHADOW|AI_RAY_CAMERA):
        self.viz["camera"].setChecked(1)
        value = value - AI_RAY_CAMERA





  def PropertyChanged(self, state):
    value= self.computeViz()
    self.controller.mainEditor.propertyChanged(dict(propname=self.paramName, default=value == self.default, value=value))


  def changed(self, message):

    value = message["value"]

    for v in self.viz:
      self.viz[v].stateChanged.disconnect()
    self.setViz(value)
    for v in self.viz:
      self.viz[v].stateChanged.connect(self.PropertyChanged)

  def resetValue(self):
    for v in self.viz:
      self.viz[v].stateChanged.disconnect()
    self.setViz(self.default)
    for v in self.viz:
      self.viz[v].stateChanged.connect(self.PropertyChanged)
