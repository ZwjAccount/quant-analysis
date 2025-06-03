CMAKE_MINIMUM_REQUIRED(VERSION 3.2)

PROJECT(quant_trading)

SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unused-result")


# 添加库搜索路径
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../hqrec/lib)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ta-lib/c/lib)

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../hqrec/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../hqrec/third_party
    ${PROJECT_SOURCE_DIR}/../hqrec/src/efh_api/
    ${PROJECT_SOURCE_DIR}/../hqrec/src/common/
    ${PROJECT_SOURCE_DIR}/../ta-lib/c/include/
)


# 设置 QUANT_TRADING
SET(QUANT_TRADING
  quant_manager.cc
  quant_data_proc.cc
  quant_factor_cal.cc
  quant_kline_proc.cc
  quant_bin.cc
  ../hqrec/src/hq_ops_efh.cc
  ../hqrec/src/efh_sse_spi.cc
  ../hqrec/src/efh_szse_spi.cc
  ../hqrec/src/efh_conf_profile.cc
  ../hqrec/src/message.cc
)
    

# Build hqops binaries.
ADD_EXECUTABLE(quant_trading ${QUANT_TRADING})
TARGET_LINK_LIBRARIES(quant_trading -static-libstdc++
    pthread
    dl
    rt
    libta_common_cmr.a
    libta_libc_cmr.a
    libta_abstract_cmr.a
    libta_func_cmr.a
)

