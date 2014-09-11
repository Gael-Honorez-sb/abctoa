# Alembic Holder
# Copyright (c) 2014, Gaël Honorez, All rights reserved.
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

from layers import _layers
from layers import _layer
from assignations import _assignationGroup
from assignations import _assignations
import shaderManager

reload(_layers)
reload(_layer)
reload(_assignationGroup)
reload(_assignations)
reload(shaderManager)

from shaderManager import List as manager

