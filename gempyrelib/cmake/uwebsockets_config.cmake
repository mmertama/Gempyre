if(WIN32)
	find_dependency(uva)
	set_target_properties(gempyre::gempyre_uv PROPERTIES
        IMPORTED_LOCATION "${uva_LIBRARY}")
elseif(RASPBERRY)
	find_dependency(uva)	
	set_target_properties(gempyre::gempyre_uv PROPERTIES
        IMPORTED_LOCATION "${uva_LIBRARY}"
        ) 
else()
	find_dependency(uSockets)
endif()
