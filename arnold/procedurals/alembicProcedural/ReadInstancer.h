#ifndef _Alembic_Arnold_ReadInstancer_h_
#define _Alembic_Arnold_ReadInstancer_h_

#include <Alembic/AbcGeom/All.h>

#include <Alembic/AbcCoreFactory/IFactory.h>
#include <Alembic/AbcCoreOgawa/ReadWrite.h>
#include <Alembic/AbcGeom/Visibility.h>

#include "ProcArgs.h"
#include "../../../common/PathUtil.h"
#include "SampleUtil.h"

#include "ArbGeomParams.h"
#include <boost/regex.hpp>


using namespace Alembic::AbcGeom;
//-*****************************************************************************

static const boost::regex e("(.+-nameprefix\\s)([\\w]+)(\\s.+)");

void WalkObjectForInstancer( IObject & parent, const ObjectHeader &i_ohead, ProcArgs &args,
             PathList::const_iterator I, PathList::const_iterator E,
                    MatrixSampleMap * xformSamples);

#endif
