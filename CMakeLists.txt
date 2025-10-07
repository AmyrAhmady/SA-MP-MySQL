cmake_minimum_required(VERSION 3.1)
project(mysql)

set(CMAKE_CXX_STANDARD 17)

# Enable multi-processor compilation on Windows
if(MSVC)
	add_compile_options(/MP)
endif()

set(MYSQL_PLUGIN_VERSION "R41-4")

option (BUILD_TESTS "Build tests" OFF)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
# Set target architecture (default: x86)
if(NOT DEFINED TARGET_ARCH)
	set(TARGET_ARCH "x86")
endif()

if(UNIX)
	if(TARGET_ARCH STREQUAL "x86")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
		add_link_options(-m32)
	endif()

	# hide non-exported symbols
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fPIC")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -fPIC")
endif()

# Download and build MySQL/MariaDB Connector C if not found
if(WIN32)
	# On Windows, download and install MariaDB Connector C MSI if not found
	set(MARIADB_CONNECTOR_VERSION "3.0.2")

	# Determine architecture
	message(STATUS "Checking which MariaDB Connector C build needs to be downloaded...")
	if(CMAKE_SIZEOF_VOID_P EQUAL 8 OR TARGET_ARCH STREQUAL "x64")
		set(MARIADB_ARCH "win64")
	else()
		set(MARIADB_ARCH "win32")
	endif()

	if(NOT DEFINED MYSQLCAPI_ROOT_DIR)
		set(MYSQLCAPI_ROOT_DIR "${PROJECT_SOURCE_DIR}/temp/${MARIADB_ARCH}/MariaDB/MariaDB Connector C")
	endif()

	set(MARIADB_MSI_FILE "${PROJECT_SOURCE_DIR}/temp/mariadb-connector-c-${MARIADB_CONNECTOR_VERSION}-${MARIADB_ARCH}.msi")

	message(STATUS ${MARIADB_MSI_FILE})
	if(NOT EXISTS "${MARIADB_MSI_FILE}")
		message(STATUS "MariaDB Connector C ${MARIADB_ARCH} not found, downloading and extracting...")

		set(DOWNLOAD_NEEDED FALSE)
		if(NOT EXISTS "${MARIADB_MSI_FILE}")
			set(DOWNLOAD_NEEDED TRUE)
		endif()

		if(DOWNLOAD_NEEDED)
			file(DOWNLOAD
				"https://downloads.mariadb.com/Connectors/c/connector-c-${MARIADB_CONNECTOR_VERSION}/mariadb-connector-c-${MARIADB_CONNECTOR_VERSION}-${MARIADB_ARCH}.msi"
				"${MARIADB_MSI_FILE}"
				SHOW_PROGRESS
				STATUS DOWNLOAD_STATUS
				TLS_VERIFY ON
			)

			list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
			if(NOT STATUS_CODE EQUAL 0)
				message(FATAL_ERROR "Failed to download MariaDB Connector C MSI")
			endif()

			# Ensure file is fully written
			execute_process(COMMAND ${CMAKE_COMMAND} -E sleep 1)
		endif()

		if(NOT EXISTS "${MARIADB_MSI_FILE}")
			message(FATAL_ERROR "File downloaded but it's not found.")
		endif()

		# Extract the MSI
		message(STATUS "Extracting MariaDB Connector C...")
		file(TO_NATIVE_PATH "${PROJECT_SOURCE_DIR}/temp/${MARIADB_ARCH}" EXTRACT_DIR_NATIVE)
		file(TO_NATIVE_PATH "${MARIADB_MSI_FILE}" MSI_FILE_NATIVE)

		message(STATUS "Target: ${EXTRACT_DIR_NATIVE}")
		message(STATUS "MSI: ${MSI_FILE_NATIVE}")

		# Run msiexec directly
		execute_process(
			COMMAND msiexec /a ${MSI_FILE_NATIVE} /qn TARGETDIR=${EXTRACT_DIR_NATIVE}
			RESULT_VARIABLE INSTALL_RESULT
			OUTPUT_VARIABLE INSTALL_OUTPUT
			ERROR_VARIABLE INSTALL_ERROR
			TIMEOUT 300
		)

		message(STATUS "Result: ${INSTALL_RESULT}")
		message(STATUS "Output: ${INSTALL_OUTPUT}")
		message(STATUS "Error: ${INSTALL_ERROR}")

		if(NOT INSTALL_RESULT EQUAL 0)
			message(FATAL_ERROR "Failed to extract MariaDB Connector C")
		endif()

		message(STATUS "MariaDB Connector C extracted successfully")
	endif()
elseif(UNIX AND NOT MYSQLCAPI_ROOT_DIR)
	# On Linux, try to find in system paths first, otherwise download and build
	set(MYSQL_CONNECTOR_VERSION "6.1.6")
	set(MYSQL_CONNECTOR_TARBALL "${PROJECT_SOURCE_DIR}/temp/mysql-connector-c-${MYSQL_CONNECTOR_VERSION}-src.tar.gz")
	set(MYSQL_CONNECTOR_DIR "${PROJECT_SOURCE_DIR}/temp/unix/mysql-connector-c-${MYSQL_CONNECTOR_VERSION}-src")

	find_path(MYSQLCAPI_CHECK NAMES "mysql_version.h" PATHS "${MYSQL_CONNECTOR_DIR}/include/mysql" NO_DEFAULT_PATH)
	if(NOT MYSQLCAPI_CHECK)
		message(STATUS "MySQL Connector C not found in system, downloading and building...")

		# Download tarball to temp folder
		if(NOT EXISTS "${MYSQL_CONNECTOR_TARBALL}")
			message(STATUS "Downloading MySQL Connector C ${MYSQL_CONNECTOR_VERSION}...")
			file(DOWNLOAD
				"http://dev.mysql.com/get/Downloads/Connector-C/mysql-connector-c-${MYSQL_CONNECTOR_VERSION}-src.tar.gz"
				"${MYSQL_CONNECTOR_TARBALL}"
				SHOW_PROGRESS
				STATUS DOWNLOAD_STATUS
			)

			list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
			if(NOT STATUS_CODE EQUAL 0)
				message(FATAL_ERROR "Failed to download MySQL Connector C")
			endif()
		endif()

		# Extract to temp/unix folder
		if(NOT EXISTS "${MYSQL_CONNECTOR_DIR}/CMakeLists.txt")
			message(STATUS "Extracting MySQL Connector C...")
			execute_process(COMMAND mkdir "${PROJECT_SOURCE_DIR}/temp/unix")
			execute_process(
				COMMAND ${CMAKE_COMMAND} -E tar xzf "${MYSQL_CONNECTOR_TARBALL}"
				WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/temp/unix"
			)
		endif()

		# Build MySQL Connector C
		if(NOT EXISTS "${MYSQL_CONNECTOR_DIR}/libmysql/libmysqlclient.a")
			message(STATUS "Building MySQL Connector C...")
			if(TARGET_ARCH STREQUAL "x86")
				execute_process(
					COMMAND ${CMAKE_COMMAND}
						-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
						-DCMAKE_CXX_FLAGS=-m32
						-DCMAKE_C_FLAGS=-m32
						-DCMAKE_INSTALL_PREFIX=/usr
						-DINSTALL_LAYOUT=DEB
						"${MYSQL_CONNECTOR_DIR}"
					WORKING_DIRECTORY "${MYSQL_CONNECTOR_DIR}"
				)
			else()
				execute_process(
					COMMAND ${CMAKE_COMMAND}
						-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
						-DCMAKE_INSTALL_PREFIX=/usr
						-DINSTALL_LAYOUT=DEB
						"${MYSQL_CONNECTOR_DIR}"
					WORKING_DIRECTORY "${MYSQL_CONNECTOR_DIR}"
				)
			endif()

			execute_process(
				COMMAND ${CMAKE_COMMAND} --build . -j2
				WORKING_DIRECTORY "${MYSQL_CONNECTOR_DIR}"
			)
		endif()

		set(MYSQLCAPI_ROOT_DIR "${MYSQL_CONNECTOR_DIR}")
	endif()
endif()

find_package(MySQLCAPI REQUIRED)

add_subdirectory(libs/log-core)
mark_as_advanced(FMT_DOC FMT_INSTALL FMT_PEDANTIC FMT_TEST FMT_USE_CPP11)

include(CheckIncludeFile)

check_include_file(alloca.h HAVE_ALLOCA_H)
if(HAVE_ALLOCA_H)
    add_definitions(-DHAVE_ALLOCA_H)
endif()

check_include_file(inttypes.h HAVE_INTTYPES_H)
if(HAVE_INTTYPES_H)
    add_definitions(-DHAVE_INTTYPES_H)
endif()

check_include_file(malloc.h HAVE_MALLOC_H)
if(HAVE_MALLOC_H)
    add_definitions(-DHAVE_MALLOC_H)
endif()

check_include_file(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

add_definitions(-DPAWN_CELL_SIZE=32)

add_subdirectory(src)
