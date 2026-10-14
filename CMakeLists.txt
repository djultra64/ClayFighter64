cmake_minimum_required(VERSION 3.20)
project(ClayTality64 LANGUAGES C CXX)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(CMAKE_CXX_STANDARD 20)
set(CLAY_SDK_ROOT "/tmp/Blast Corps source review b48mc3gh/independent SDK" CACHE PATH "Read-only N64 SDK")
set(RUNTIME "${CLAY_SDK_ROOT}/deps/N64ModernRuntime")
set(RT64 "${CLAY_SDK_ROOT}/deps/rt64")
set(RT64_BUILD "${CLAY_SDK_ROOT}/rt64-build")
add_subdirectory("${RUNTIME}/ultramodern" "${CMAKE_BINARY_DIR}/ultramodern")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
 "${CMAKE_SOURCE_DIR}/tools/prepare_threadqueue_trace.py"
 "${RUNTIME}/ultramodern/src/threadqueue.cpp")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/prepare_threadqueue_trace.py"
 "${RUNTIME}" "${CMAKE_SOURCE_DIR}/generated/threadqueue_trace.cpp" RESULT_VARIABLE CLAY_QUEUE_TRACE_RESULT)
if(NOT CLAY_QUEUE_TRACE_RESULT EQUAL 0)
 message(FATAL_ERROR "Cannot prepare immutable SDK queue diagnostic")
endif()
get_target_property(CLAY_UM_SOURCES ultramodern SOURCES)
list(REMOVE_ITEM CLAY_UM_SOURCES "${RUNTIME}/ultramodern/src/threadqueue.cpp")
list(APPEND CLAY_UM_SOURCES "${CMAKE_SOURCE_DIR}/generated/threadqueue_trace.cpp")
set_property(TARGET ultramodern PROPERTY SOURCES "${CLAY_UM_SOURCES}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
 "${CMAKE_SOURCE_DIR}/tools/graphics_interpolation_observer.py"
 "${RT64}/src/hle/rt64_workload_queue.cpp")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/graphics_interpolation_observer.py"
 "${RT64}" "${CMAKE_SOURCE_DIR}/generated/graphics_workload_queue.cpp" RESULT_VARIABLE CLAY_GRAPHICS_OBSERVER_RESULT)
if(NOT CLAY_GRAPHICS_OBSERVER_RESULT EQUAL 0)
 message(FATAL_ERROR "Cannot prepare immutable RT64 interpolation observer")
endif()
file(GLOB GUEST CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/generated/guest/funcs_*.c")
set_source_files_properties(${GUEST} PROPERTIES LANGUAGE CXX COMPILE_OPTIONS "-w;-O1;-include;${CMAKE_SOURCE_DIR}/host/guest_support.h")
add_executable(claytality64 host/main.cpp host/main_menu.cpp host/sc_bios.cpp host/logo64.cpp host/title_audio.cpp host/selector_layout.cpp host/runtime.cpp host/renderer.cpp host/graphics_config.cpp
 host/input.cpp host/input_sdl.cpp host/input_hud.cpp host/input_mode.cpp host/test_input.cpp host/native_prompts.cpp host/port_options.cpp host/options_border.cpp host/memory_budget.cpp host/interpolation.cpp host/sprite_seams.cpp host/ui_background.cpp host/vs_alignment.cpp host/sc_assets.cpp host/sc_roster.cpp host/sc_preview.cpp generated/graphics_workload_queue.cpp
 generated/fallbacks.cpp generated/audio.cpp "${RUNTIME}/librecomp/src/rsp.cpp"
 "${RUNTIME}/librecomp/src/ultra_translation.cpp"
 "${RUNTIME}/librecomp/src/cont.cpp" "${RUNTIME}/librecomp/src/vi.cpp" ${GUEST})
target_compile_options(claytality64 PRIVATE -mssse3 -msse4.1 -g)
target_compile_options(ultramodern PRIVATE -g)
target_compile_definitions(claytality64 PRIVATE PLUME_SDL_VULKAN_ENABLED HLSL_CPU IMGUI_IMPL_VULKAN_NO_PROTOTYPES FFX_GCC)
target_include_directories(claytality64 PRIVATE host generated/guest
 "${RUNTIME}/N64Recomp/include" "${RUNTIME}/ultramodern/include" "${RUNTIME}/librecomp/include" "${RUNTIME}/librecomp/include/librecomp"
 "${RUNTIME}/thirdparty" "${RUNTIME}/thirdparty/concurrentqueue" "${RUNTIME}/thirdparty/json"
 "${RT64}/src/contrib/plume" "${RT64}/src/contrib/json" "${RT64}/src/contrib/volk" "${RT64}/src" "${RT64}/src/contrib" "${RT64}/src/contrib/hlslpp/include"
 "${RT64}/src/hle" "${RT64}/src/contrib/imgui" "${RT64}/src/contrib/mupen64plus-core/src/api"
 "${RT64}/src/contrib/plume/contrib/volk"
 "${RT64}/src/contrib/plume/contrib/VulkanMemoryAllocator/include"
 "${RT64}/src/contrib/plume/contrib/Vulkan-Headers/include"
 /home/linuxbrew/.linuxbrew/include/SDL2 /home/linuxbrew/.linuxbrew/include /home/linuxbrew/.linuxbrew/opt/xorgproto/include)
target_link_libraries(claytality64 PRIVATE ultramodern "${RT64_BUILD}/rt64.a"
 "${RT64_BUILD}/src/contrib/re-spirv/libre-spirv.a"
 "${RT64_BUILD}/src/contrib/nativefiledialog-extended/src/libnfd.a"
 "${RT64_BUILD}/src/contrib/zstd/build/cmake/lib/libzstd.a"
 "${RT64_BUILD}/src/contrib/plume/libplume.a"
 /home/linuxbrew/.linuxbrew/lib/libSDL2.so /home/linuxbrew/.linuxbrew/lib/libdbus-1.so pthread dl)
set_target_properties(claytality64 PROPERTIES BUILD_RPATH "/home/linuxbrew/.linuxbrew/lib;${RT64}/src/contrib/dxc/lib/x64")
target_link_options(claytality64 PRIVATE -Wl,--wrap=osStartThread_recomp -Wl,--wrap=osStopThread_recomp)

enable_testing()
if(EXISTS "${CMAKE_SOURCE_DIR}/probe-output/base.z64")
 add_test(NAME original_sc_sound_dispatch COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_sc_sound_dispatch.py" --sdk-root "${CLAY_SDK_ROOT}")
 set_tests_properties(original_sc_sound_dispatch PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" TIMEOUT 60)
 if(EXISTS "${CMAKE_SOURCE_DIR}/probe-output/sc.z64")
  add_test(NAME sc_sound_bank_compatibility COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_sc_sound_banks.py")
  set_tests_properties(sc_sound_bank_compatibility PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" TIMEOUT 60)
 endif()
endif()
add_executable(scripted_input_test tests/test_input.cpp host/test_input.cpp)
target_include_directories(scripted_input_test PRIVATE host "${RUNTIME}/thirdparty/json" /home/linuxbrew/.linuxbrew/include/SDL2)
target_compile_options(scripted_input_test PRIVATE -UNDEBUG)
target_link_libraries(scripted_input_test PRIVATE /home/linuxbrew/.linuxbrew/lib/libSDL2.so)
add_test(NAME two_player_validation_driver COMMAND scripted_input_test "${CMAKE_BINARY_DIR}/scripted-input-fixture.json")
set_tests_properties(two_player_validation_driver PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
add_test(NAME rt64_fractional_fighter_matching COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_rt64_interpolation.py" --sdk-root "${CLAY_SDK_ROOT}")
set_tests_properties(rt64_fractional_fighter_matching PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" TIMEOUT 180)
add_test(NAME native_fighter_interpolation COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_interpolation.py")
set_tests_properties(native_fighter_interpolation PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" TIMEOUT 120)
add_test(NAME native_port_options COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_options.py")
set_tests_properties(native_port_options PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" TIMEOUT 60)
if(EXISTS "${CMAKE_SOURCE_DIR}/local/sc_roster/asset-image-roster.z64")
 add_test(NAME native_sixteen_character_selector COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/sc_roster_test.py")
 set_tests_properties(native_sixteen_character_selector PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" TIMEOUT 90)
 add_test(NAME native_selector_grid_mesh COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/sc_roster_mesh_test.py")
 set_tests_properties(native_selector_grid_mesh PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" TIMEOUT 90)
endif()
add_test(NAME native_game_font_prompts
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_native_prompts.py" --sdk-root "${CLAY_SDK_ROOT}")
set_tests_properties(native_game_font_prompts PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" TIMEOUT 60)
add_executable(sc_asset_scope_test tests/sc_assets.cpp host/sc_assets.cpp)
target_include_directories(sc_asset_scope_test PRIVATE host "${RUNTIME}/N64Recomp/include")
if(EXISTS "${CMAKE_SOURCE_DIR}/analysis/sc_assets/asset-image.z64")
 add_test(NAME sc_asset_scope COMMAND sc_asset_scope_test "${CMAKE_SOURCE_DIR}/analysis/sc_assets/asset-image.z64")
endif()
add_test(NAME framebuffer_transition_wait
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/async_resource_wait.py" --sdk "${RUNTIME}")
set_tests_properties(framebuffer_transition_wait PROPERTIES TIMEOUT 60)
add_executable(graphics_config_test tests/graphics_config.cpp host/graphics_config.cpp)
target_include_directories(graphics_config_test PRIVATE host)
add_test(NAME graphics_profiles COMMAND graphics_config_test)
add_executable(action_input_test tests/input.cpp host/input.cpp)
target_include_directories(action_input_test PRIVATE host)
target_compile_options(action_input_test PRIVATE -UNDEBUG)
add_test(NAME action_input_mapping COMMAND action_input_test)
add_executable(sdl_input_test tests/input_sdl.cpp host/input.cpp host/input_sdl.cpp)
target_include_directories(sdl_input_test PRIVATE host /home/linuxbrew/.linuxbrew/include/SDL2)
target_compile_options(sdl_input_test PRIVATE -UNDEBUG)
target_link_libraries(sdl_input_test PRIVATE /home/linuxbrew/.linuxbrew/lib/libSDL2.so)
add_test(NAME sdl_controller_events COMMAND sdl_input_test)
set_tests_properties(sdl_controller_events PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
add_executable(sdl_two_player_test tests/input_two_player.cpp host/input.cpp host/input_sdl.cpp)
target_include_directories(sdl_two_player_test PRIVATE host /home/linuxbrew/.linuxbrew/include/SDL2)
target_compile_options(sdl_two_player_test PRIVATE -UNDEBUG)
target_link_libraries(sdl_two_player_test PRIVATE /home/linuxbrew/.linuxbrew/lib/libSDL2.so)
add_test(NAME sdl_two_player_routing COMMAND sdl_two_player_test)
set_tests_properties(sdl_two_player_routing PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
add_test(NAME original_secret_character_filter
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_unlocks.py"
 --include-dir "${RUNTIME}/N64Recomp/include")
add_executable(thread_lifecycle_test tests/thread_lifecycle.cpp "${CMAKE_SOURCE_DIR}/generated/threadqueue_trace.cpp")
target_include_directories(thread_lifecycle_test PRIVATE host "${RUNTIME}/ultramodern/include" "${RUNTIME}/thirdparty" "${RUNTIME}/thirdparty/concurrentqueue")
foreach(CLAY_THREAD_CASE stop stop_nonhead stop_running remove)
 add_test(NAME thread_lifecycle_${CLAY_THREAD_CASE} COMMAND thread_lifecycle_test ${CLAY_THREAD_CASE})
 set_tests_properties(thread_lifecycle_${CLAY_THREAD_CASE} PROPERTIES TIMEOUT 5)
endforeach()

add_test(NAME native_sprite_strip_continuity COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_sprite_seams.py" --sdk-root "${CLAY_SDK_ROOT}")
add_test(NAME native_widescreen_ui_coverage COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_ui_background.py" --sdk-root "${CLAY_SDK_ROOT}")

add_test(NAME native_donor_vs_alignment COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_vs_alignment.py" --sdk-root "${CLAY_SDK_ROOT}")

add_test(NAME native_options_border COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/native_options_border_test.py" --sdk-root "${CLAY_SDK_ROOT}")

add_test(NAME native_expansion_heap_budget COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_memory_budget.py" --sdk-root "${CLAY_SDK_ROOT}")

add_test(NAME native_title_audio_replacement COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_title_audio.py" --sdk-root "${CLAY_SDK_ROOT}")
set_tests_properties(native_title_audio_replacement PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" TIMEOUT 60)

add_test(NAME native_logo64_mesh COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/logo64_test.py")
set_tests_properties(native_logo64_mesh PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" TIMEOUT 60)

add_test(NAME native_selector_layout COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_selector_layout.py" --sdk-root "${CLAY_SDK_ROOT}")
set_tests_properties(native_selector_layout PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" TIMEOUT 60)

add_test(NAME sc_bios_native COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_sc_bios.py" --sdk-root "${CLAY_SDK_ROOT}")
set_tests_properties(sc_bios_native PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" TIMEOUT 60)

add_test(NAME captured_title_audio_detection COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/test_title_audio_capture.py")
set_tests_properties(captured_title_audio_detection PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" TIMEOUT 60)

add_test(NAME native_main_menu_exit COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_main_menu.py" --sdk-root "${CLAY_SDK_ROOT}")
set_tests_properties(native_main_menu_exit PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" TIMEOUT 60)
