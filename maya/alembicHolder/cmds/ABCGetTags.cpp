#include <stdio.h>
#include <maya/MArgList.h>

// Alembic headers

#include "ABCGetTags.h"

// ABC helpers
#include "ArchiveHelper.h"
#include <maya/MStringArray.h>
#include <json/json.h>





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

	MStringArray results;

	IObject archiveTop = archive.getTop();

	if (m_path.length() != 0)
	{

		if (getNamedObj( archiveTop, m_path.asChar()) == false)
			return MS::kFailure;


	}

	visitObject( archiveTop, &results );
	setResult(results);
		
	
	return MS::kSuccess;

}


void ABCGetTags::visitObject( IObject iObj, MStringArray* results )
{
    const MetaData &md = iObj.getMetaData();

    if ( IPolyMeshSchema::matches( md ))
    {
		if ( IPolyMesh::matches( iObj.getMetaData() ) )
		{
				IPolyMesh mesh( iObj, kWrapExisting );
				IPolyMeshSchema ms = mesh.getSchema();

				ICompoundProperty arbGeomParams = ms.getArbGeomParams();
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
										for( Json::ValueIterator itr = jtags.begin() ; itr != jtags.end() ; itr++ )
										{
											MString tag(jtags[itr.key().asUInt()].asCString());
											bool found = false;
											for (unsigned int i = 0; i < results->length(); ++i)
											{
												if(tag == (*results)[i])
												{
													found = true;
													break;
												}
											}
											if (!found)
												results->append(tag);							
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
