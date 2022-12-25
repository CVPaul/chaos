# CMakeList.txt: lightio 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#

SET(THIRDPARTY_PREFIX_DIR "D:/Program Files (x86)")

# add link directory
LINK_DIRECTORIES(
    ${THIRDPARTY_PREFIX_DIR}/protobuf/lib
)
LINK_LIBRARIES(libprotobuf)

# include extern dir
INCLUDE_DIRECTORIES(
    .
    ./include
    ${THIRDPARTY_PREFIX_DIR}/protobuf/include
)

# 将源代码添加到此项目的可执行文件。
ADD_EXECUTABLE(lightio "lightio.cpp" "lightio.h" "tick.pb.h" "tick.pb.cc")

# TODO: 如有需要，请添加测试并安装目标。
