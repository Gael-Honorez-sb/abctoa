add_library(AttributeFile_In MODULE
    src/op.cpp
    src/FileReaderFactory.cpp
    src/KatanaAttrFileReader.cpp
	)

target_compile_definitions(AttributeFile_In PRIVATE)

target_link_libraries(AttributeFile_In
    PRIVATE
    Katana::FnAttribute
    Katana::FnGeolibOpPlugin
    Katana::FnGeolibServices
    Katana::FnPlatform
)

target_include_directories(AttributeFile_In PRIVATE
    include
    ${Boost_INCLUDE_DIRS})

set_target_properties(AttributeFile_In PROPERTIES PREFIX "")
install(TARGETS AttributeFile_In DESTINATION Ops)
