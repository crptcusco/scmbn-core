cmake_minimum_required(VERSION 3.10)

# 1. Definición oficial del Proyecto v1.0
project(scmbn_core VERSION 1.0.0 DESCRIPTION "Stationary-Coupled Modular Boolean Networks Core")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Forzar optimización máxima si no se especifica
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# 2. Dependencias del Sistema
find_package(OpenMP REQUIRED)
find_path(MINISAT_INCLUDE_DIR minisat/core/Solver.h REQUIRED)
find_library(MINISAT_LIBRARY minisat REQUIRED)

include_directories(include ${MINISAT_INCLUDE_DIR})

# 3. Código Fuente del Núcleo (Listado explícito para evitar código muerto)
set(CORE_SOURCES
    src/cbnetwork.cpp
    src/directededge.cpp
    src/globaltopology.cpp
    src/localnetwork.cpp
    src/localscene.cpp
    src/network_factory.cpp
    src/campaign_manager.cpp
)

# Compilar la librería core
add_library(cbn_core ${CORE_SOURCES})
target_link_libraries(cbn_core PUBLIC OpenMP::OpenMP_CXX ${MINISAT_LIBRARY})

# 4. Binarios de Producción (Apuntando a la nueva carpeta 'apps/')
add_executable(run_pipeline apps/run_pipeline.cpp)
target_link_libraries(run_pipeline cbn_core)

add_executable(experiment_runner apps/experiment_runner.cpp)
target_link_libraries(experiment_runner cbn_core)

add_executable(scientific_benchmarking apps/scientific_benchmarking.cpp)
target_link_libraries(scientific_benchmarking cbn_core)

# 5. Pruebas Unitarias
add_executable(unit_tests apps/unit_tests.cpp)
target_link_libraries(unit_tests cbn_core)

# 6. Activar Automatización de Pruebas (CTest)
enable_testing()
add_test(NAME CoreLogicValidation COMMAND unit_tests)