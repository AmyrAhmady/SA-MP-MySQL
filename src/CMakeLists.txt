if(NOT WIN32 AND MYSQLCAPI_LIBRARY_STATIC)
	option(BUILD_STATIC "Build static library" OFF)
endif()

set(MYSQL_PLUGIN_CONFIG_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/config_headers")
include_directories(
	${PROJECT_SOURCE_DIR}/libs/sdk
	${PROJECT_SOURCE_DIR}/libs/sdk/amx
	${MYSQL_PLUGIN_CONFIG_INCLUDE_DIR}
	${PROJECT_SOURCE_DIR}/libs/log-core/include
)

configure_file(
	"../a_mysql.inc.in"
	"a_mysql.inc"
	@ONLY
)

configure_file(
	"version.hpp.in"
	"${MYSQL_PLUGIN_CONFIG_INCLUDE_DIR}/version.hpp"
	@ONLY
)

set(SOURCES
	${PROJECT_SOURCE_DIR}/libs/sdk/amxplugin.cpp
	CCallback.cpp
	CCallback.hpp
	CConnection.cpp
	CConnection.hpp
	CDispatcher.cpp
	CDispatcher.hpp
	CError.hpp
	CHandle.cpp
	CHandle.hpp
	CLog.cpp
	CLog.hpp
	COptions.cpp
	COptions.hpp
	COrm.cpp
	COrm.hpp
	CQuery.cpp
	CQuery.hpp
	CResult.cpp
	CResult.hpp
	natives.cpp
	natives.hpp
	CSingleton.hpp
	main.cpp
	misc.hpp
	mysql.hpp
	sdk.hpp
	types.hpp
	plugin.def
)

# Create shared library
add_library(mysql SHARED ${SOURCES})

target_include_directories(mysql PRIVATE
	${MYSQLCAPI_INCLUDE_DIR}
	${LOGCORE_INCLUDE_DIR}
)

add_dependencies(mysql log-core fmt)

if(MSVC)
	set_target_properties(mysql PROPERTIES
		LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/plugin.def"
	)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX -D_ENABLE_ATOMIC_ALIGNMENT_FIX -D_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS)
endif()

target_link_libraries(mysql log-core fmt)

if(UNIX)
	add_definitions(-DLINUX)
	if(NOT APPLE)
		target_link_libraries(mysql rt)
	endif()

	set_target_properties(mysql PROPERTIES
		INSTALL_RPATH "$ORIGIN/..:$ORIGIN/")
endif()

if(BUILD_STATIC)
	target_link_libraries(mysql "${MYSQLCAPI_LIBRARY_STATIC}")
else()
	target_link_libraries(mysql "${MYSQLCAPI_LIBRARY}")
endif()

install(TARGETS mysql DESTINATION "plugins/")
install(TARGETS log-core DESTINATION "./")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/a_mysql.inc" DESTINATION "pawno/include/")
install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" DESTINATION "./")
if(WIN32 AND MYSQLCAPI_LIBRARY_SHARED)
	install(FILES "${MYSQLCAPI_LIBRARY_SHARED}" DESTINATION "./")
endif()

set(CPACK_PACKAGE_VERSION ${MYSQL_PLUGIN_VERSION})
if(WIN32)
	set(CPACK_GENERATOR ZIP)
else()
	set(CPACK_GENERATOR TGZ)
endif()

include(CPack)
