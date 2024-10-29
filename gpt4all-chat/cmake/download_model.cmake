if(NOT DEFINED URL OR NOT DEFINED OUTPUT_PATH OR NOT DEFINED EXPECTED_MD5)
    message(FATAL_ERROR "Usage: cmake -DURL=<url> -DOUTPUT_PATH=<path> -DEXPECTED_MD5=<md5> -P download_model.cmake")
endif()

message(STATUS "Downloading model from ${URL} to ${OUTPUT_PATH} ...")

file(DOWNLOAD "${URL}" "${OUTPUT_PATH}" EXPECTED_MD5 "${EXPECTED_MD5}" STATUS status)

list(GET status 0 status_code)
if(NOT status_code EQUAL 0)
    message(FATAL_ERROR "Failed to download model: ${status}")
endif()
