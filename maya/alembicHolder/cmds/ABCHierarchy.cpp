/*Alembic Holder
Copyright (c) 2014, Gaël Honorez, All rights reserved.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3.0 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public
License along with this library.*/


#include <stdio.h>
#include <maya/MArgList.h>

// Alembic headers
#include <Alembic/AbcCoreFactory/IFactory.h>
#include <Alembic/AbcGeom/Visibility.h>
#include "Alembic/AbcGeom/All.h"
#include "Alembic/Abc/All.h"

#include "ABCHierarchy.h"

// ABC helpers
#include "ArchiveHelper.h"



using namespace Alembic::AbcGeom;


MStatus ABCHierarchy::doIt( const MArgList& args )
{
    MStatus status;
    // Parse the arguments
    for ( unsigned int i = 0; i < args.length(); i++ )
    {
        if (i == 0)
        {
            MFileObject fileObject;
            fileObject.setRawFullName(args.asString( i, &status ).expandFilePath());
            fileObject.setResolveMethod(MFileObject::kInputFile);
            m_filename = fileObject.resolvedFullName();
        }
        else if (i == 1)
            m_path = args.asString( i, &status );
    }

    Alembic::Abc::IArchive archive;
    Alembic::AbcCoreFactory::IFactory factory;
    factory.setPolicy(Alembic::Abc::ErrorHandler::kQuietNoopPolicy);
    archive = factory.getArchive(filename());

    if (!archive.valid()) {
        std::cout << "error reading archive" << std::endl;
        displayError("Unable to read file");
        return MS::kFailure;
    }


    IObject archiveTop = archive.getTop();

    if (m_path.length() != 0)
    {

        if (getNamedObj( archiveTop, m_path.asChar()) == false)
            return MS::kFailure;


    }

    std::vector<Alembic::AbcGeom::IObject>  _objs;


    getABCGeos(archiveTop, _objs);




    int obj = 0;
    for( std::vector<Alembic::AbcGeom::IObject>::const_iterator iObj( _objs.begin() ); iObj != _objs.end(); ++iObj )
    {
        MString typeObj;
        if(IXform::matches(iObj->getHeader()))
            typeObj = "Transform:";
        else if(IPolyMesh::matches(iObj->getHeader()))
            typeObj = "Shape:";
        else if(IPoints::matches(iObj->getHeader()))
            typeObj = "Points:";
        else if(ICurves::matches(iObj->getHeader()))
            typeObj = "Curves:";
        else if(ILight::matches(iObj->getHeader()))
        {
            typeObj = "Light:";
            ILight light( iObj->getParent(), iObj->getName() );
            ILightSchema ps = light.getSchema();
            ICompoundProperty arbGeomParams = ps.getArbGeomParams();
            const PropertyHeader * lightTypeHeader = arbGeomParams.getPropertyHeader("light_type");
            if (IInt32GeomParam::matches( *lightTypeHeader ))
            {
                IInt32GeomParam param( arbGeomParams,  "light_type" );
                if ( param.valid() )
                {
                    if ( param.getScope() == kConstantScope || param.getScope() == kUnknownScope)
                    {
                        IInt32GeomParam::prop_type::sample_ptr_type valueSample = param.getExpandedValue().getVals();
                        Alembic::Abc::int32_t lightType = valueSample->get()[0];
                        switch(lightType)
                        {
                            case 0:
                                typeObj = "DistantLight:";
                                break;
                            case 1:
                                typeObj = "PointLight:";
                                break;
                            case 2:
                                typeObj = "SpotLight:";
                                break;
                            case 3:
                                typeObj = "QuadLight:";
                                break;
                            case 4:
                                typeObj = "PhotometricLight:";
                                break;
                            case 5:
                                typeObj = "DiskLight:";
                                break;
                            case 6:
                                typeObj = "CylinderLight:";
                                break;
                            default:
                                return MS::kSuccess;
                        }
                    }
                }

            }
        }
        else
            typeObj = "Unknown:";

        appendToResult(typeObj + iObj->getName().c_str());
        obj++;
    }

    return MS::kSuccess;

}



void* ABCHierarchy::creator() {

    return new ABCHierarchy;

}
