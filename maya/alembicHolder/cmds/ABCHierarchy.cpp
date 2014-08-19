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

    std::vector<Alembic::AbcGeom::IObject>  _objs;


    getABCGeos(archiveTop, _objs);




    int obj = 0;
    for( std::vector<Alembic::AbcGeom::IObject>::const_iterator iObj( _objs.begin() ); iObj != _objs.end(); ++iObj )
    {
        MString typeObj;
        if(IXform::matches(iObj->getHeader()))
            typeObj = "Transform:";
        else
            typeObj = "Shape:";

        appendToResult(typeObj + iObj->getName().c_str());
        obj++;
    }

    return MS::kSuccess;

}



void* ABCHierarchy::creator() {

    return new ABCHierarchy;

}
