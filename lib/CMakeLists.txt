#添加源文件
target_sources(
    example_lib
    PUBLIC
    
    header/lib.hpp

    lib.cpp
)

#添加头文件目录，否则example_main无法找到lib.hpp
target_include_directories(example_lib PUBLIC header)