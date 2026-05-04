add_library(chess_engine STATIC
  attacks.cpp
  position_engine.cpp
  pv_engine.cpp
  material_engine.cpp
  game.cpp
  gui.cpp
  movegen.cpp
  position.cpp
  random_engine.cpp
  search.cpp
  uci_client.cpp
  zobrist.cpp
)

target_include_directories(chess_engine PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/include
)

# Position independent for flexibility
set_target_properties(chess_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(chess_engine PRIVATE sfml-graphics sfml-window sfml-system)

# Compiler features and warnings (modern CMake + generator expressions)
target_compile_features(chess_engine PUBLIC cxx_std_17)
target_compile_options(chess_engine PRIVATE
  $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:MSVC>>:/W4>
  $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-Wall;-Wextra;-Wpedantic>
)

# Executable - GUI version
add_executable(chess main.cpp)
target_link_libraries(chess PRIVATE chess_engine)

# Executable - UCI White (PositionEngine)
add_executable(chess_uci_position main_uci.cpp uci.cpp)
target_link_libraries(chess_uci_position PRIVATE chess_engine)
target_compile_definitions(chess_uci_position PRIVATE
  UCI_ENGINE_TYPE=1
)

# Executable - UCI Black (MaterialEngine)
add_executable(chess_uci_material main_uci.cpp uci.cpp)
target_link_libraries(chess_uci_material PRIVATE chess_engine)
target_compile_definitions(chess_uci_material PRIVATE
  UCI_ENGINE_TYPE=2
)

# Executable - UCI White (PositionEngine)
add_executable(chess_uci_PV main_uci.cpp uci.cpp)
target_link_libraries(chess_uci_PV PRIVATE chess_engine)
target_compile_definitions(chess_uci_PV PRIVATE
  UCI_ENGINE_TYPE=3
)