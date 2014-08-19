from PyQt4.QtGui import *
from PyQt4.QtCore import *
from arnold import *
from property_widget import PropertyWidget

class PropertyWidgetEnum(PropertyWidget):
   def __init__(self, controller,  pentry, name, enum, parent = None):
      PropertyWidget.__init__(self, name, parent)
     
      self.paramName = name
         
      self.widget = QComboBox(self)

      self.controller = controller
      self.controller.setPropertyValue.connect(self.changed)
      self.controller.reset.connect(self.resetValue)

      param_value = AiParamGetDefault(pentry)
      param_type = AiParamGetType(pentry)
      self.default = self.GetParamValueAsString(pentry, param_value, param_type)

      self.widget.setCurrentIndex(self.default) 
  
      index = 0
      while True:
         value = AiEnumGetString(enum, index)
         index += 1
         if not value:
            break   
         self.widget.addItem(value)


     
      self.widget.currentIndexChanged.connect(self.ValueChanged)
      self.layout().addWidget(self.widget)

   def ValueChanged(self, value):
    #self.__WriteToArnold()
    self.controller.mainEditor.propertyChanged(dict(propname=self.paramName, default=value == self.default, value=value))

   def changed(self, message):

      value = message["value"]
      self.widget.setCurrentIndex(value) 
  

   def resetValue(self):
      self.widget.setCurrentIndex(self.default) 
