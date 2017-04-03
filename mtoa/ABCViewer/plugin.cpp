/*
 * plugin.cpp
 *
 *  Created on: Jan 13, 2012
 *      Author: nozon
 */

#include "ABCViewer.h"
#include "extension/Extension.h"

extern "C"
{

    EXPORT_API_VERSION

    DLLEXPORT void initializeExtension(CExtension &plugin)
    {
        MStatus status;

            plugin.Requires("alembicHolder");
            status = plugin.RegisterTranslator("alembicHolder",
                                                "",
                                                CABCViewerTranslator::creator,
                                                CABCViewerTranslator::NodeInitializer);
    }

    DLLEXPORT void deinitializeExtension(CExtension &plugin)
    {
    }

}
