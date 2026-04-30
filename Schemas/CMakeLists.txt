# ---- EvoEngine Schema Generation ----
# This project manages all .fbs schema files used by the engine.
# Building "EvoSchemasGen" in VS triggers flatc to generate _generated.h headers.

file(GLOB EVO_SCHEMA_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.fbs)

# Output directory for generated headers
set(EVO_SCHEMA_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/Generated)
file(MAKE_DIRECTORY ${EVO_SCHEMA_GENERATED_DIR})

# Generate a _generated.h for each .fbs schema
set(EVO_GENERATED_HEADERS)
foreach(SCHEMA_FILE ${EVO_SCHEMA_FILES})
	get_filename_component(SCHEMA_NAME ${SCHEMA_FILE} NAME_WE)
	set(GENERATED_HEADER ${EVO_SCHEMA_GENERATED_DIR}/${SCHEMA_NAME}_generated.h)

	add_custom_command(
		OUTPUT  ${GENERATED_HEADER}
		COMMAND flatc -c -I "${CMAKE_CURRENT_SOURCE_DIR}" -o "${EVO_SCHEMA_GENERATED_DIR}" "${SCHEMA_FILE}"
		DEPENDS flatc ${EVO_SCHEMA_FILES}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "FlatBuffers: generating ${SCHEMA_NAME}_generated.h"
	)
	list(APPEND EVO_GENERATED_HEADERS ${GENERATED_HEADER})
endforeach()

# Custom target — visible in VS solution, triggers generation when built
add_custom_target(EvoSchemasGen
	DEPENDS ${EVO_GENERATED_HEADERS}
	SOURCES ${EVO_SCHEMA_FILES}
)

# INTERFACE library — provides generated header include path to consumers
add_library(EvoSchemas INTERFACE)
target_include_directories(EvoSchemas INTERFACE ${EVO_SCHEMA_GENERATED_DIR})
add_dependencies(EvoSchemas EvoSchemasGen)

# VS solution folder
set_target_properties(EvoSchemasGen PROPERTIES FOLDER "Schemas")


