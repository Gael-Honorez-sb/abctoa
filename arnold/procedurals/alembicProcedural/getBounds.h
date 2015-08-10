#ifndef _getbounds_h_
#define _getbounds_h_

#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreFactory/All.h>

#include <ImathBoxAlgo.h>


#include "ProcArgs.h"

#include <iostream>

//-*****************************************************************************
using namespace ::Alembic::AbcGeom;


void accumXform( M44d &xf, IObject obj, chrono_t seconds );
M44d getFinalMatrix( IObject &iObj, chrono_t seconds );
void getBounds( IObject iObj, chrono_t seconds, Box3d & g_bounds );
void getBoundingBox(IObject iObj, chrono_t seconds, Box3d & g_bounds );

#endif