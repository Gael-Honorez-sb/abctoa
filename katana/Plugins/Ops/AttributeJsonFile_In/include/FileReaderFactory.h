// Copyright (c) 2014 The Foundry Visionmongers Ltd. All Rights Reserved.

#ifndef FILEREADERFACTORY_H
#define FILEREADERFACTORY_H

#include <boost/shared_ptr.hpp>

#include "FileReader.h"

typedef boost::shared_ptr<FileReader> FileReaderPtr;

class FileReaderFactory
{
public:
    FileReaderFactory() {};
    virtual ~FileReaderFactory() {};
    FileReaderPtr get();
    FileReaderPtr getDefault();
};

#endif

