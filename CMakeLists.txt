add_executable(mcsend mcsend.cpp)
set_target_properties(mcsend PROPERTIES FOLDER apps)
target_link_libraries(mcsend omega)

add_executable(mcserver mcserver.cpp)
set_target_properties(mcserver PROPERTIES FOLDER apps)
target_link_libraries(mcserver omega)

add_executable(oimg oimg.cpp)
set_target_properties(oimg PROPERTIES FOLDER apps)
target_link_libraries(oimg omega)

add_executable(olauncher olauncher.cpp)
set_target_properties(olauncher PROPERTIES FOLDER apps)
target_link_libraries(olauncher omega)


