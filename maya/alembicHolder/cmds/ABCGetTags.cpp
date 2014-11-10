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

#include "ABCGetTags.h"

// ABC helpers
#include "ArchiveHelper.h"
#include <maya/MStringArray.h>






MStatus ABCGetTags::doIt( const MArgList& args )
{
    MStatus status;
    // Parse the arguments
    for ( unsigned int i = 0; i < args.length(); i++ )
    {
        if (i == 0)
            m_filename = args.asString( i, &status );
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
    Json::Value tags;
    visitObject( archiveTop, tags );
    Json::FastWriter fastWriter;
   
    MString result(fastWriter.write(tags).c_str());
    setResult(result);


    return MS::kSuccess;

}


void ABCGetTags::visitObject( IObject iObj, Json::Value & results )
{
    const MetaData &md = iObj.getMetaData();

    ICompoundProperty arbGeomParams;
    std::string name;

    if ( IXformSchema::matches(md))
    {
        if ( IXformSchema::matches( iObj.getMetaData() ) )
        {
                IXform xform( iObj, kWrapExisting );
                IXformSchema ms = xform.getSchema();

                arbGeomParams = ms.getArbGeomParams();
                name = xform.getName();
        }

    }
    else if ( IPolyMeshSchema::matches( md ))
    {
        if ( IPolyMesh::matches( iObj.getMetaData() ) )
        {
                IPolyMesh mesh( iObj, kWrapExisting );
                IPolyMeshSchema ms = mesh.getSchema();
                arbGeomParams = ms.getArbGeomParams();
                name = mesh.getName();
        }
    }
    
    if ( arbGeomParams != NULL && arbGeomParams.valid() )
    {
        if (arbGeomParams.getPropertyHeader("mtoa_constant_tags") != NULL)
        {
            const PropertyHeader * tagsHeader = arbGeomParams.getPropertyHeader("mtoa_constant_tags");
            if (IStringGeomParam::matches( *tagsHeader ))
            {
                IStringGeomParam param( arbGeomParams,  "mtoa_constant_tags" );
                if ( param.valid() )
                {
                    IStringGeomParam::prop_type::sample_ptr_type valueSample = param.getExpandedValue().getVals();
                    if ( param.getScope() == kConstantScope || param.getScope() == kUnknownScope)
                    {
                        Json::Value jtags;
                        Json::Reader reader;
                        if(reader.parse(valueSample->get()[0], jtags))
                        {
                            for( Json::ValueIterator itr = jtags.begin() ; itr != jtags.end() ; itr++ )
                            {
                                
                                std::string tag = jtags[itr.key().asUInt()].asString();
                                // Check if we already have this tag or not.
                                bool found = false;
                                for( Json::ValueIterator itr2 = results.begin() ; itr2 != results.end() ; itr2++ )
                                {
                                    if(tag.compare(itr2.key().asString()) == 0)
                                    {
                                        results[tag].append(name);
                                        found = true;
                                    }


                                }
                                if(!found)
                                {
                                    Json::Value newTags;
                                    newTags.append(name);
                                    results[tag] = newTags;
                                }
                            }
                        }
                    }

                }
            }
        }
    }

    // now the child objects
    for ( size_t i = 0 ; i < iObj.getNumChildren() ; i++ )
    {
        visitObject( IObject( iObj, iObj.getChildHeader( i ).getName() ), results );
    }
}

void* ABCGetTags::creator() {

    return new ABCGetTags;

}
