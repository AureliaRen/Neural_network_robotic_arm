cmake_minimum_required(VERSION 3.24)

set(SNN_COMPUTE_BACKEND "GPU" CACHE STRING "Compute backend: GPU or CPU")
set_property(CACHE SNN_COMPUTE_BACKEND PROPERTY STRINGS GPU CPU)

if(SNN_COMPUTE_BACKEND STREQUAL "CPU")
    project(SNN_Simulator LANGUAGES CXX)
    set(SNN_USE_CUDA OFF)
elseif(SNN_COMPUTE_BACKEND STREQUAL "GPU")
    project(SNN_Simulator LANGUAGES CXX CUDA)
    set(SNN_USE_CUDA ON)
else()
    message(FATAL_ERROR "SNN_COMPUTE_BACKEND must be GPU or CPU")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(SNN_USE_CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_ARCHITECTURES 52 CACHE STRING "CUDA architectures")
endif()

option(SNN_BUILD_ALL_MAINS "Build every mainprogram/*.cpp as a standalone executable" ON)
set(SNN_DEFAULT_MAIN "main_program" CACHE STRING "Main source stem to build when SNN_BUILD_ALL_MAINS=OFF")
set(SNN_ENABLE_ZMQ "AUTO" CACHE STRING "Enable ZeroMQ support: AUTO, ON, or OFF")
set_property(CACHE SNN_ENABLE_ZMQ PROPERTY STRINGS AUTO ON OFF)
set(SNN_ENABLE_PINOCCHIO "AUTO" CACHE STRING "Enable Pinocchio outer dynamics support: AUTO, ON, or OFF")
set_property(CACHE SNN_ENABLE_PINOCCHIO PROPERTY STRINGS AUTO ON OFF)

set(SNN_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/source_file_realtime_v1_async")
set(SNN_MAINPROGRAM_DIR "${SNN_SOURCE_ROOT}/mainprogram")
set(SNN_CUDA_LINK_STUB "${SNN_MAINPROGRAM_DIR}/cuda_link_stub.cu")

if(SNN_USE_CUDA)
    find_package(CUDAToolkit REQUIRED)
endif()
find_package(OpenMP)

set(SNN_ZMQ_HINT_DIRS
    "D:/Programing/Pakage_management_related/VCPKG/vcpkg/installed/x64-windows"
    "D:/ProgramData/anaconda3/Library")

set(SNN_PINOCCHIO_HINT_DIRS
    "D:/ProgramData/anaconda3/envs/pinocchio-cpp"
    "D:/ProgramData/anaconda3/envs/pinocchio-cpp/Library"
    "D:/ProgramData/anaconda3/envs/pinocchio-cpp/Library/lib/cmake/pinocchio")

set(SNN_BOOST_HINT_DIRS
    "D:/anaconda/envs/pinocchio-cpp/Library"
    "D:/ProgramData/anaconda3/envs/pinocchio-cpp/Library")

find_path(SNN_ZMQ_INCLUDE_DIR
    NAMES zmq.hpp
    HINTS ${SNN_ZMQ_HINT_DIRS}
    PATH_SUFFIXES include)

find_library(SNN_ZMQ_LIBRARY
    NAMES libzmq-mt-4_3_5 libzmq zmq
    HINTS ${SNN_ZMQ_HINT_DIRS}
    PATH_SUFFIXES lib)

find_path(SNN_BOOST_INCLUDE_DIR
    NAMES boost/any.hpp
    HINTS ${SNN_BOOST_HINT_DIRS}
    PATH_SUFFIXES include)

if(NOT SNN_BOOST_INCLUDE_DIR)
    message(FATAL_ERROR "Boost headers were not found; expected boost/any.hpp")
endif()

set(SNN_WITH_ZMQ OFF)
if(SNN_ENABLE_ZMQ STREQUAL "ON")
    if(NOT SNN_ZMQ_INCLUDE_DIR OR NOT SNN_ZMQ_LIBRARY)
        message(FATAL_ERROR "SNN_ENABLE_ZMQ=ON but ZeroMQ headers or libraries were not found")
    endif()
    set(SNN_WITH_ZMQ ON)
elseif(SNN_ENABLE_ZMQ STREQUAL "AUTO")
    if(SNN_ZMQ_INCLUDE_DIR AND SNN_ZMQ_LIBRARY)
        set(SNN_WITH_ZMQ ON)
    endif()
endif()

set(SNN_WITH_PINOCCHIO OFF)
if(SNN_ENABLE_PINOCCHIO STREQUAL "ON")
    find_package(pinocchio CONFIG REQUIRED HINTS ${SNN_PINOCCHIO_HINT_DIRS})
    set(SNN_WITH_PINOCCHIO ON)
elseif(SNN_ENABLE_PINOCCHIO STREQUAL "AUTO")
    find_package(pinocchio CONFIG QUIET HINTS ${SNN_PINOCCHIO_HINT_DIRS})
    if(pinocchio_FOUND)
        set(SNN_WITH_PINOCCHIO ON)
    endif()
endif()

file(GLOB_RECURSE SNN_CPP_SOURCES CONFIGURE_DEPENDS
    "${SNN_SOURCE_ROOT}/*.cpp"
    "${SNN_SOURCE_ROOT}/*.cc"
    "${SNN_SOURCE_ROOT}/*.cxx")

set(SNN_ALL_SOURCES ${SNN_CPP_SOURCES})
if(SNN_USE_CUDA)
    file(GLOB_RECURSE SNN_CUDA_SOURCES CONFIGURE_DEPENDS
        "${SNN_SOURCE_ROOT}/*.cu")
    list(APPEND SNN_ALL_SOURCES ${SNN_CUDA_SOURCES})
endif()

set(SNN_CORE_SOURCES ${SNN_ALL_SOURCES})
set(SNN_MAIN_SOURCES)
foreach(_src IN LISTS SNN_ALL_SOURCES)
    file(TO_CMAKE_PATH "${_src}" _src_norm)
    if(_src_norm MATCHES "/mainprogram/.*\\.(cpp|cc|cxx|cu)$")
        list(APPEND SNN_MAIN_SOURCES "${_src}")
        list(REMOVE_ITEM SNN_CORE_SOURCES "${_src}")
    endif()
endforeach()

set(SNN_ZMQ_CORE_SOURCES
    "${SNN_SOURCE_ROOT}/communication/src/ZmqSocket.cpp"
    "${SNN_SOURCE_ROOT}/communication/src/ZMQInputOutputSpikeDriver.cpp"
    "${SNN_SOURCE_ROOT}/communication/src/ZMQAsyncInputOutputSpikeDriver.cpp")

set(SNN_OUTER_DYNAMIC_COMMON_SOURCES
    "${SNN_SOURCE_ROOT}/OuterDynamic/src/OuterDynamicModel.cpp"
    "${SNN_SOURCE_ROOT}/OuterDynamic/src/OuterDynamicSpikeBuffer.cpp"
    "${SNN_SOURCE_ROOT}/ModelFactory/OuterDynamicModelFactory.cpp"
    "${SNN_SOURCE_ROOT}/Event/src/Outer/OuterUpdateEvent.cpp"
    "${SNN_SOURCE_ROOT}/OuterDynamic/src/TotalModelPlanarArm2DOF.cpp")

set(SNN_PINOCCHIO_SOURCES
    "${SNN_SOURCE_ROOT}/OuterDynamic/src/PlanarArm2DOFPinocchio.cpp")

if(NOT SNN_WITH_ZMQ)
    list(REMOVE_ITEM SNN_CORE_SOURCES ${SNN_ZMQ_CORE_SOURCES})
endif()

list(REMOVE_ITEM SNN_CORE_SOURCES ${SNN_PINOCCHIO_SOURCES})
list(REMOVE_ITEM SNN_CORE_SOURCES ${SNN_OUTER_DYNAMIC_COMMON_SOURCES})
list(APPEND SNN_CORE_SOURCES ${SNN_OUTER_DYNAMIC_COMMON_SOURCES})

if(SNN_USE_CUDA)
    list(REMOVE_ITEM SNN_MAIN_SOURCES "${SNN_CUDA_LINK_STUB}")
endif()

set(SNN_CPU_BACKEND_SOURCES
    "${SNN_SOURCE_ROOT}/Openmp/src/OpenmpCPU.cpp"
    "${SNN_SOURCE_ROOT}/Network/src/NetworkCPU.cpp"
    "${SNN_SOURCE_ROOT}/Simulation/src/SimulationCPU.cpp")

set(SNN_GPU_BACKEND_SOURCES
    "${SNN_SOURCE_ROOT}/Openmp/src/OpenmpGPU.cu"
    "${SNN_SOURCE_ROOT}/Network/src/Network.cu"
    "${SNN_SOURCE_ROOT}/Simulation/src/Simulation.cu")

if(SNN_USE_CUDA)
    list(REMOVE_ITEM SNN_CORE_SOURCES ${SNN_CPU_BACKEND_SOURCES})
else()
    list(FILTER SNN_CORE_SOURCES EXCLUDE REGEX ".*/TimeDrivenGPU/.*")
    list(FILTER SNN_CORE_SOURCES EXCLUDE REGEX ".*/Openmp/src/OpenmpGPU\\.cu$")
    list(FILTER SNN_CORE_SOURCES EXCLUDE REGEX ".*/Network/src/Network\\.cu$")
    list(FILTER SNN_CORE_SOURCES EXCLUDE REGEX ".*/Simulation/src/Simulation\\.cu$")
    list(FILTER SNN_CORE_SOURCES EXCLUDE REGEX ".*/NeuralStateVector/Neuron_State_Vector_Interface\\.cu$")
    list(REMOVE_ITEM SNN_CORE_SOURCES ${SNN_GPU_BACKEND_SOURCES})
endif()

add_library(snn_core STATIC ${SNN_CORE_SOURCES})
target_include_directories(snn_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${SNN_SOURCE_ROOT}"
    "${SNN_SOURCE_ROOT}/MotionEnergy/inc"
    "${SNN_BOOST_INCLUDE_DIR}")
target_compile_definitions(snn_core PUBLIC
    _CRT_SECURE_NO_WARNINGS
    NOMINMAX
    SNN_WITH_ZMQ=$<BOOL:${SNN_WITH_ZMQ}>
    SNN_WITH_PINOCCHIO=$<BOOL:${SNN_WITH_PINOCCHIO}>
    SNN_CPU_ONLY=$<NOT:$<BOOL:${SNN_USE_CUDA}>>)
if(SNN_USE_CUDA)
    set_target_properties(snn_core PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CUDA_RESOLVE_DEVICE_SYMBOLS ON)
endif()

if(OpenMP_CXX_FOUND AND NOT SNN_USE_CUDA)
    target_link_libraries(snn_core PUBLIC OpenMP::OpenMP_CXX)
endif()

if(SNN_USE_CUDA)
    target_link_libraries(snn_core PUBLIC CUDA::cudart_static)
endif()

if(MSVC)
    target_compile_options(snn_core PUBLIC
        $<$<COMPILE_LANGUAGE:CXX>:/openmp>)
    if(SNN_USE_CUDA)
        target_compile_options(snn_core PUBLIC
            $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/openmp>)
    endif()
endif()

if(SNN_WITH_ZMQ)
    target_include_directories(snn_core PUBLIC "${SNN_ZMQ_INCLUDE_DIR}")
    target_link_libraries(snn_core PUBLIC "${SNN_ZMQ_LIBRARY}")
    if(WIN32)
        target_link_libraries(snn_core PUBLIC ws2_32 rpcrt4 iphlpapi)
    endif()
endif()

if(SNN_WITH_PINOCCHIO)
    add_library(snn_outer_dynamic_pinocchio STATIC ${SNN_PINOCCHIO_SOURCES})
    target_include_directories(snn_outer_dynamic_pinocchio PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${SNN_SOURCE_ROOT}"
        "${SNN_SOURCE_ROOT}/MotionEnergy/inc"
        "${SNN_BOOST_INCLUDE_DIR}")
    target_compile_definitions(snn_outer_dynamic_pinocchio PUBLIC
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        SNN_WITH_ZMQ=$<BOOL:${SNN_WITH_ZMQ}>
        SNN_WITH_PINOCCHIO=1
        SNN_CPU_ONLY=$<NOT:$<BOOL:${SNN_USE_CUDA}>>)
    if(MSVC)
        target_compile_options(snn_outer_dynamic_pinocchio PUBLIC /openmp)
    endif()
    if(OpenMP_CXX_FOUND AND NOT SNN_USE_CUDA)
        target_link_libraries(snn_outer_dynamic_pinocchio PUBLIC OpenMP::OpenMP_CXX)
    endif()
    target_link_libraries(snn_outer_dynamic_pinocchio PUBLIC pinocchio::pinocchio)
endif()

function(snn_add_main_target main_stem)
    set(main_file "${SNN_MAINPROGRAM_DIR}/${main_stem}.cpp")
    if(NOT EXISTS "${main_file}")
        message(FATAL_ERROR "Main source not found: ${main_file}")
    endif()

    if(SNN_USE_CUDA)
        add_executable(${main_stem} "${main_file}" "${SNN_CUDA_LINK_STUB}")
    else()
        add_executable(${main_stem} "${main_file}")
    endif()
    target_include_directories(${main_stem} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${SNN_SOURCE_ROOT}"
        "${SNN_SOURCE_ROOT}/MotionEnergy/inc"
        "${SNN_BOOST_INCLUDE_DIR}")
    target_compile_definitions(${main_stem} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        SNN_WITH_ZMQ=$<BOOL:${SNN_WITH_ZMQ}>
        SNN_WITH_PINOCCHIO=$<BOOL:${SNN_WITH_PINOCCHIO}>
        SNN_CPU_ONLY=$<NOT:$<BOOL:${SNN_USE_CUDA}>>)
    if(SNN_USE_CUDA)
        set_target_properties(${main_stem} PROPERTIES
            CUDA_SEPARABLE_COMPILATION ON
            CUDA_RESOLVE_DEVICE_SYMBOLS ON)
        target_link_libraries(${main_stem} PRIVATE snn_core CUDA::cudart_static)
        if(SNN_WITH_PINOCCHIO)
            target_link_libraries(${main_stem} PRIVATE snn_outer_dynamic_pinocchio)
        endif()
    else()
        target_link_libraries(${main_stem} PRIVATE snn_core)
        if(SNN_WITH_PINOCCHIO)
            target_link_libraries(${main_stem} PRIVATE snn_outer_dynamic_pinocchio)
        endif()
    endif()
    if(SNN_WITH_ZMQ)
        target_include_directories(${main_stem} PRIVATE "${SNN_ZMQ_INCLUDE_DIR}")
        target_link_libraries(${main_stem} PRIVATE "${SNN_ZMQ_LIBRARY}")
        if(WIN32)
            target_link_libraries(${main_stem} PRIVATE ws2_32 rpcrt4 iphlpapi)
        endif()
    endif()
    if(MSVC)
        target_compile_options(${main_stem} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:/openmp>)
        if(SNN_USE_CUDA)
            target_compile_options(${main_stem} PRIVATE
                $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/openmp>)
        endif()
    endif()
endfunction()

function(snn_add_explicit_main_target main_stem)
    if(NOT TARGET ${main_stem})
        snn_add_main_target("${main_stem}")
    endif()
endfunction()

set(SNN_CPU_INCOMPATIBLE_MAINS
    "RobotControl6DOF"
    "RobotControl6DOF_double"
    "ZMQcommunication_test"
    "memory_check_main"
    "motion_energy_test"
    "pattern_motion_network_test")

if(SNN_BUILD_ALL_MAINS)
    foreach(_main_file IN LISTS SNN_MAIN_SOURCES)
        get_filename_component(_main_stem "${_main_file}" NAME_WE)
        if(NOT TARGET ${_main_stem})
            if((NOT SNN_USE_CUDA) AND (_main_stem IN_LIST SNN_CPU_INCOMPATIBLE_MAINS))
                message(STATUS "Skipping ${_main_stem} because CPU backend does not provide the GPU-only neuron models it uses")
            else()
                snn_add_main_target("${_main_stem}")
            endif()
        endif()
    endforeach()
else()
    snn_add_main_target("${SNN_DEFAULT_MAIN}")
endif()

snn_add_explicit_main_target("r_stdp_zmq_wta_test")
snn_add_explicit_main_target("r_stdp_zmq_ring_attractor_test")
snn_add_explicit_main_target("izhikevich_ampa_nmda_gaba_test")
snn_add_explicit_main_target("izhikevich_gpu_network_test")
snn_add_explicit_main_target("motion_energy_test")
snn_add_explicit_main_target("motion_energy_source_check")
snn_add_explicit_main_target("motion_energy_direction_maps")
snn_add_explicit_main_target("mt_cds_direction_maps")
snn_add_explicit_main_target("mt_cds_pds_lip_direction_maps")
snn_add_explicit_main_target("pattern_motion_network_test")
snn_add_explicit_main_target("science_robotics_cerebellum_repro")
if(SNN_WITH_PINOCCHIO)
    snn_add_explicit_main_target("outer_dynamics_validation")
endif()

if(TARGET main_program)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT main_program)
endif()
