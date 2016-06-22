// Copyright (c) 2014 The Foundry Visionmongers Ltd. All Rights Reserved.

#ifdef _WIN32
#include <FnPlatform/Windows.h>
#else
#include <dlfcn.h>
#endif // _WIN32

#include <stdexcept>
#include <sstream>

#include <boost/shared_ptr.hpp>

#include "FileReaderFactory.h"
#include "KatanaAttrFileReader.h"


FileReaderPtr FileReaderFactory::get()
{
    // If the .so file is not specified, use the default FileReader
    return getDefault();
}

FileReaderPtr FileReaderFactory::getDefault()
{
    return FileReaderPtr(new KatanaAttrFileReader());
}

