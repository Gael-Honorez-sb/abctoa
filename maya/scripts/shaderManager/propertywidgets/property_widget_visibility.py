from PyQt4.QtGui import *
from PyQt4.QtCore import *
from arnold import *
from property_widget import PropertyWidget


class PropertyWidgetVisibility(PropertyWidget):
  def __init__(self, controller, pentry, name, parent = None):
    PropertyWidget.__init__(self, name, parent)

    self.paramName = name


    self.controller = controller
    self.controller.setPropertyValue.connect(self.changed)
    self.controller.reset.connect(self.resetValue)

    self.viz = {}
    self.viz["camera"] = QCheckBox(self)
    self.viz["cast_shadows"] = QCheckBox(self)
    self.viz["diffuse"] = QCheckBox(self)
    self.viz["glossy"] = QCheckBox(self)
    self.viz["reflection"] = QCheckBox(self)
    self.viz["refraction"] = QCheckBox(self)

    param_value = AiParamGetDefault(pentry)
    param_type = AiParamGetType(pentry)


    self.default = self.GetParamValueAsString(pentry, param_value, param_type)
    self.setViz(self.default)
    #self.widget.setChecked(self.default)
    #self.PropertyChanged(self.default)


    grid= QGridLayout()
    row = 1
    for v in self.viz:
      label = QLabel(v)
      grid.addWidget(label, row, 1)
      grid.addWidget(self.viz[v], row, 2)
      self.viz[v].stateChanged.connect(self.PropertyChanged)
      row = row + 1


    self.layout().addLayout(grid)


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

    return value

  def setViz(self, value):

    for v in self.viz:
      self.viz[v].setChecked(0)

    if value > AI_RAY_ALL & ~AI_RAY_GLOSSY:
        self.viz["glossy"].setChecked(1)
        value = value - AI_RAY_GLOSSY
    if value > AI_RAY_ALL & ~(AI_RAY_GLOSSY|AI_RAY_DIFFUSE):
        self.viz["diffuse"].setChecked(1)
        value = value - AI_RAY_DIFFUSE
    if value > AI_RAY_ALL & ~(AI_RAY_GLOSSY|AI_RAY_DIFFUSE|AI_RAY_REFRACTED):
        self.viz["refraction"].setChecked(1)
        value = value - AI_RAY_REFRACTED
    if value > AI_RAY_ALL & ~(AI_RAY_GLOSSY|AI_RAY_DIFFUSE|AI_RAY_REFRACTED|AI_RAY_REFLECTED):
        self.viz["reflection"].setChecked(1)
        value = value - AI_RAY_REFLECTED
    if value > AI_RAY_ALL & ~(AI_RAY_GLOSSY|AI_RAY_DIFFUSE|AI_RAY_REFRACTED|AI_RAY_REFLECTED|AI_RAY_SHADOW):
        self.viz["cast_shadows"].setChecked(1)
        value = value - AI_RAY_SHADOW
    if value > AI_RAY_ALL & ~(AI_RAY_GLOSSY|AI_RAY_DIFFUSE|AI_RAY_REFRACTED|AI_RAY_REFLECTED|AI_RAY_SHADOW|AI_RAY_CAMERA):
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