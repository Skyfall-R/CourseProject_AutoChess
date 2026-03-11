# 指定CMake版本，防止兼容性问题
cmake_minimum_required(VERSION 3.21)

# 项目名字、版本、语言，注意项目名字和生成的可执行文件名字没啥关系
project(
	AutoChess
	VERSION 1.0.0
	LANGUAGES CXX
)

# MSVC启用UTF-8编码支持
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# 设置C++标准，cpp17算常用的
set(CMAKE_CXX_STANDARD 17)

### 上面的都没有什么用（？），每个项目复制一遍就好了，下面才是重点 ###


# 创建一个链接库
add_library(example_lib)


# 创建一个可执行文件
add_executable(example_main)

target_compile_features(example_main PRIVATE cxx_std_17)
set_target_properties(example_main PROPERTIES CXX_STANDARD 17)
set_target_properties(example_main PROPERTIES CXX_STANDARD_REQUIRED ON)

# example_main 需要依赖 example_lib 才能工作，因为 say_hello() 的实现在 example_lib 里
target_link_libraries(example_main PRIVATE example_lib)


# 添加子路径。add_subdirectory(folder)相当于继续去执行 folder/CMakeLists.txt 的内容，便于管理
add_subdirectory(lib)
add_subdirectory(main)