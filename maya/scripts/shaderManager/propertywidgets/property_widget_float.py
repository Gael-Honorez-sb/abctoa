from PyQt4.QtGui import *
from PyQt4.QtCore import *
from arnold import *
from property_widget import *
class PropertyWidgetFloat(PropertyWidget):
   def __init__(self, controller,  pentry, name, parent = None):
      PropertyWidget.__init__(self, name, parent)

      self.paramName = name

      self.controller = controller
      self.controller.setPropertyValue.connect(self.changed)
      self.controller.reset.connect(self.resetValue)

      param_value = AiParamGetDefault(pentry)
      param_type = AiParamGetType(pentry)
      self.default = self.GetParamValueAsString(pentry, param_value, param_type)



      self.widget = QDoubleSpinBox()
      self.widget.setValue(self.default)
      # if AiMetaDataGetFlt(nentry, name, "min", byref(data)):
      #    self.widget.setMinimum(data.value)
      # if AiMetaDataGetFlt(nentry, name, "max", byref(data)):
      #    self.widget.setMaximum(data.value)
      # if AiMetaDataGetFlt(nentry, name, "self.default", byref(data)):
      #    self.widget.setValue(data.value)
      #    self.ValueChanged(data.value)
      # else:
      #    self.__ReadFromArnold()

      self.widget.valueChanged.connect(self.ValueChanged)
      self.layout().addWidget(self.widget)
   def ValueChanged(self, value):
      self.controller.mainEditor.propertyChanged(dict(propname=self.paramName, default=value == self.default, value=value))


   def changed(self, message):

      value = message["value"]
      self.widget.setValue(value)


   def resetValue(self):
      self.widget.setValue(self.default)