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

from PySide import QtGui, QtCore
import os

TRANSFORM = 1
SHAPE = 2
SHADER = 3
WILDCARD = 4

class treeDelegate(QtGui.QStyledItemDelegate):
    def __init__(self, *args, **kwargs):
        QtGui.QStyledItemDelegate.__init__(self, *args, **kwargs)

    def paint(self, painter, option, index, *args, **kwargs):
        
        options = QtGui.QStyleOptionViewItemV4(option)

        self.initStyleOption(options, index)
        


        style = options.widget.style() if options.widget else  QtGui.QApplication.style()

        options.text = ""
        style.drawControl(style.CE_ItemViewItem, options, painter, options.widget)

        text = index.data(QtCore.Qt.DisplayRole)
        fieldType = index.data(QtCore.Qt.UserRole)

        d = os.path.dirname(__file__)

        if fieldType == 1:
            iconfile = os.path.join(d, "../../../icons/group.png")
        elif fieldType == 2:
            iconfile = os.path.join(d, "../../../icons/shape.png")
        elif fieldType == 3:
            iconfile = os.path.join(d, "../../../icons/sg.xpm")
        elif fieldType == 4:
            iconfile = os.path.join(d, "../../../icons/wildcard.png")

        painter.save()

        if options.state & QtGui.QStyle.State_MouseOver:
            painter.fillRect(option.rect, QtGui.QColor("#202020"))
            #painter.fillRect(option.rect, painter.brush())
        
        html = QtGui.QTextDocument()
        html.setHtml(text)

        iconsize = QtCore.QSize(22,22)

        if text != "":
            icon = QtGui.QIcon()    
            icon.addFile(iconfile, QtCore.QSize(22,22))
            icon.paint(painter, option.rect.adjusted(5-2, -2, 0, 0), QtCore.Qt.AlignLeft|QtCore.Qt.AlignVCenter)   
            iconsize = icon.actualSize(option.rect.size())

        
        #Description
        painter.translate(option.rect.left() + iconsize.width() + 10, option.rect.top()+10)
        clip = QtCore.QRectF(0, 0, option.rect.width()-iconsize.width() - 10 - 5, option.rect.height())
        html.drawContents(painter, clip)
  
        painter.restore()


    def sizeHint(self, option, index, *args, **kwargs):
        text = index.data(QtCore.Qt.DisplayRole)
        html = QtGui.QTextDocument()
        html.setHtml(text)
        height = max(html.size().height() + 10, 35)
        if text != "":
            return QtCore.QSize(option.rect.left() + html.size().width()+22+10+50, height)
        else :
            return QtCore.QSize(35, height)

