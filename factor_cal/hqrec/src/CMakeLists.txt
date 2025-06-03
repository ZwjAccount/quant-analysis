CMAKE_MINIMUM_REQUIRED(VERSION 3.2)

PROJECT(hqrec)

SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unused-result")


# 添加库搜索路径
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib)

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
    ${PROJECT_SOURCE_DIR}/efh_api/
    ${PROJECT_SOURCE_DIR}/include/
)


# 设置 HQREC
SET(HQREC
    hq_ops_efh.cc
    efh_sse_spi.cc
    efh_szse_spi.cc
    efh_conf_profile.cc
    message.cc
)
    

# Build hqops binaries.
ADD_LIBRARY(hqrec ${HQREC})
TARGET_LINK_LIBRARIES(hqrec -static-libstdc++
    pthread
    dl
    rt
)

