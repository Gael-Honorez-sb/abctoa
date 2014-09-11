from PySide.QtGui import *
from PySide.QtCore import *
from arnold import *
from property_widget import *
class PropertyWidgetVector(PropertyWidget):
   def __init__(self, pentry, name, vectorType, parent = None):
      PropertyWidget.__init__(self, name, parent)
      self.node       = node
      self.paramName  = name
      self.vectorType = vectorType
      self.widget1 = QDoubleSpinBox(self)
      self.widget1.valueChanged.connect(self.PropertyChanged)
      self.layout().addWidget(self.widget1)
      self.widget2 = QDoubleSpinBox(self)
      self.widget2.valueChanged.connect(self.PropertyChanged)
      self.layout().addWidget(self.widget2)
      if self.vectorType != PropertyWidget.POINT2:
         self.widget3 = QDoubleSpinBox(self)
         self.widget3.valueChanged.connect(self.PropertyChanged)
         self.layout().addWidget(self.widget3)
      if self.vectorType == PropertyWidget.VECTOR:
         data = AtVector()
         if AiMetaDataGetVec(nentry, name, "min", byref(data)):
            self.widget1.setMinimum(data.x)
            self.widget2.setMinimum(data.y)
            self.widget3.setMinimum(data.z)
         if AiMetaDataGetVec(nentry, name, "max", byref(data)):
            self.widget1.setMaximum(data.x)
            self.widget2.setMaximum(data.y)
            self.widget3.setMaximum(data.z)
         if AiMetaDataGetVec(nentry, name, "default", byref(data)):
            self.widget1.setValue(data.x)
            self.widget2.setValue(data.y)
            self.widget3.setValue(data.z)
            self.PropertyChanged()
      elif self.vectorType == PropertyWidget.POINT:
         data = AtPoint()
         if AiMetaDataGetPnt(nentry, name, "min", byref(data)):
            self.widget1.setMinimum(data.x)
            self.widget2.setMinimum(data.y)
            self.widget3.setMinimum(data.z)
         if AiMetaDataGetPnt(nentry, name, "max", byref(data)):
            self.widget1.setMaximum(data.x)
            self.widget2.setMaximum(data.y)
            self.widget3.setMaximum(data.z)
         if AiMetaDataGetPnt(nentry, name, "default", byref(data)):
            self.widget1.setValue(data.x)
            self.widget2.setValue(data.y)
            self.widget3.setValue(data.z)
            self.PropertyChanged()
      elif self.vectorType == PropertyWidget.POINT2:
         data = AtPoint2()
         if AiMetaDataGetPnt2(nentry, name, "min", byref(data)):
            self.widget1.setMinimum(data.x)
            self.widget2.setMinimum(data.y)
         if AiMetaDataGetPnt2(nentry, name, "max", byref(data)):
            self.widget1.setMaximum(data.x)
            self.widget2.setMaximum(data.y)
         if AiMetaDataGetPnt2(nentry, name, "default", byref(data)):
            self.widget1.setValue(data.x)
            self.widget2.setValue(data.y)
            self.PropertyChanged()
      self.__ReadFromArnold()
   def PropertyChanged(self):
      self.__WriteToArnold()
      self.propertyChanged.emit(self.paramName)
   def __ReadFromArnold(self):
      if self.vectorType == PropertyWidget.VECTOR:
         v = AiNodeGetVec(self.node, self.paramName)
         self.widget1.setValue(v.x)
         self.widget2.setValue(v.y)
         self.widget3.setValue(v.z)
      elif self.vectorType == PropertyWidget.POINT:
         p = AiNodeGetPnt(self.node, self.paramName)
         self.widget1.setValue(p.x)
         self.widget2.setValue(p.y)
         self.widget3.setValue(p.z)
      elif self.vectorType == PropertyWidget.POINT2:
         p = AiNodeGetPnt2(self.node, self.paramName)
         self.widget1.setValue(p.x)
         self.widget2.setValue(p.y)
   def __WriteToArnold(self):
      if self.vectorType == PropertyWidget.VECTOR:
         AiNodeSetVec(self.node, self.paramName, self.widget1.value(), self.widget2.value(), self.widget3.value())
      elif self.vectorType == PropertyWidget.POINT:
         AiNodeSetPnt(self.node, self.paramName, self.widget1.value(), self.widget2.value(), self.widget3.value())
      elif self.vectorType == PropertyWidget.POINT2:
         AiNodeSetPnt2(self.node, self.paramName, self.widget1.value(), self.widget2.value())