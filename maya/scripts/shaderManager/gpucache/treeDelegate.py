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
DISPLACE = 5
TAG = 6

ICONSIZE = 22

ICONS = {
1: QtGui.QIcon(),
2: QtGui.QIcon(),
3: QtGui.QIcon(),
4: QtGui.QIcon(),
5: QtGui.QIcon(),
6: QtGui.QIcon(),
7: QtGui.QIcon(),
8: QtGui.QIcon(),
9: QtGui.QIcon(),
}

d = os.path.dirname(__file__)

ICONS[1].addFile(os.path.join(d, "../../../icons/group.png"),       QtCore.QSize(ICONSIZE,ICONSIZE) )
ICONS[2].addFile(os.path.join(d, "../../../icons/shape.png"),       QtCore.QSize(ICONSIZE,ICONSIZE) )
ICONS[3].addFile(os.path.join(d, "../../../icons/sg.xpm"),          QtCore.QSize(ICONSIZE,ICONSIZE) )
ICONS[4].addFile(os.path.join(d, "../../../icons/wildcard.png"),    QtCore.QSize(ICONSIZE,ICONSIZE) )
ICONS[5].addFile(os.path.join(d, "../../../icons/displacement.xpm"),QtCore.QSize(ICONSIZE,ICONSIZE) )
ICONS[6].addFile(os.path.join(d, "../../../icons/tag.png"),QtCore.QSize(ICONSIZE,ICONSIZE) )
ICONS[7].addFile(os.path.join(d, "../../../icons/points.png"),QtCore.QSize(ICONSIZE,ICONSIZE) )
ICONS[8].addFile(os.path.join(d, "../../../icons/light.png"),QtCore.QSize(ICONSIZE,ICONSIZE) )
ICONS[9].addFile(os.path.join(d, "../../../icons/curves.png"),QtCore.QSize(ICONSIZE,ICONSIZE) )

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

        painter.save()

        if options.state & QtGui.QStyle.State_MouseOver:
            painter.fillRect(option.rect, QtGui.QColor("#202020"))
        
        html = QtGui.QTextDocument()
        html.setHtml(text)


        if text != "":
            fieldType = index.data(QtCore.Qt.UserRole)
            icon =  ICONS[fieldType]
            painter.drawPixmap(option.rect.left()+5, option.rect.top()+10, icon.pixmap(ICONSIZE, ICONSIZE))
            #icon.paint(painter, option.rect.adjusted(5-2, -2, 0, 0), QtCore.Qt.AlignLeft|QtCore.Qt.AlignVCenter)   

        #Description
        painter.translate(option.rect.left() + ICONSIZE + 10, option.rect.top()+10)
        clip = QtCore.QRectF(0, 0, option.rect.width()- ICONSIZE - 10 - 5, option.rect.height())
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

