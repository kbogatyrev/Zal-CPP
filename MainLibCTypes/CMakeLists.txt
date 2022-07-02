cmake_minimum_required (VERSION 3.16)

project (ZalPythonItf VERSION 1.0.1)

if (MSVC)
	if (CMAKE_BUILD_TYPE MATCHES "Debug")
		message (STATUS "Windows debug mode")
		add_compile_definitions(
			WIN32
			_WINDOWS
			_UNICODE
			UNICODE
			_DEBUG
			_CONSOLE
		)
		add_compile_options(
			/source-charset:utf-8
			/wd4996		# unsafe function -- ignored
			/JMC /ifcOutput "out/build/x64-Debug/" /GS /W3 /Zc:wchar_t 
			/Zi /Gm- /Od 
			/Zc:inline /fp:precise #/D "_DEBUG" 
			/errorReport:prompt /WX- /Zc:forScope /RTC1 /Gd /MDd /std:c++17 /FC 
			/diagnostics:column 
		)
	else()
		message (STATUS "Windows release mode")
		add_compile_definitions(
			WIN32
			_WINDOWS
			_UNICODE
			UNICODE
			_CONSOLE
			NDEBUG
		)
		add_compile_options(
			/source-charset:utf-8
			/wd4996		# unsafe function -- ignored
			/ifcOutput "out/build/x64-Release/" /GS /GL /W3 /Gy /Zc:wchar_t 
			/Zi /Gm- /O2 
			/Zc:inline /fp:precise 
			/errorReport:prompt /WX- /Zc:forScope /Gd /Oi /MD 
			/std:c++17 /FC 
			/EHsc /nologo 
			/diagnostics:column 
		)
	endif()
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

add_library (ZalPythonItf SHARED)

target_include_directories (ZalPythonItf PUBLIC
	../../Zal-Core/MainLib
	../../Zal-Core/HLib
)

target_sources (ZalPythonItf PRIVATE
#	dllmain.cpp
	ZalPythonItf.cpp
)

target_link_directories (ZalPythonItf PRIVATE
	../../Zal-Core/out/build/x64-debug
)

target_link_libraries (ZalPythonItf 
	ZalMainLib)

#if (CMAKE_BUILD_TYPE MATCHES "Debug")
#	target_link_libraries(ZalPythonItf "../../../../../Zal-Core/out/build/x64-Debug/ZalMainLib")
#	add_custom_command(TARGET ZalPythonItf POST_BUILD
#		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ZalPythonItf> ../../Zal-Python/PyZal
#	)
#else()
#	target_link_libraries(ZalPythonItf "../../../../../Zal-Core/out/build/x64-Release/ZalMainLib")
#	add_custom_command(TARGET ZalPythonItf POST_BUILD
#		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ZalPythonItf> ../../Zal-Python/PyZal
#	)
#endif()
