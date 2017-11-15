
set(THIRDPARTY_SUB
	jsoncpp
	pystring
	openEXR/IlmBase
)

foreach(SUBDIR ${THIRDPARTY_SUB})
	add_subdirectory(${SUBDIR})
endforeach()

set_target_properties (pystring_lib_static PROPERTIES
    FOLDER ThirdParty
)

set_target_properties (jsoncpp_lib_static PROPERTIES
    FOLDER ThirdParty
)

set_target_properties (eLut PROPERTIES
    FOLDER ThirdParty/ilmbase
)

set_target_properties (Half PROPERTIES
    FOLDER ThirdParty/ilmbase
)

set_target_properties (HalfTest PROPERTIES
    FOLDER ThirdParty/ilmbase/Tests
)

set_target_properties (Iex PROPERTIES
    FOLDER ThirdParty/ilmbase
)

set_target_properties (IexTest PROPERTIES
    FOLDER ThirdParty/ilmbase/Tests
)


set_target_properties (IexMath PROPERTIES
    FOLDER ThirdParty/ilmbase
)


set_target_properties (IlmThread PROPERTIES
    FOLDER ThirdParty/ilmbase
)

set_target_properties (Imath PROPERTIES
    FOLDER ThirdParty/ilmbase
)

set_target_properties (ImathTest PROPERTIES
    FOLDER ThirdParty/ilmbase/Tests
)

set_target_properties (toFloat PROPERTIES
    FOLDER ThirdParty/ilmbase
)