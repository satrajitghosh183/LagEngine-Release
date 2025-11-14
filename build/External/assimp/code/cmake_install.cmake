# Install script for directory: /mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xlibassimp5.3.0-devx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/mnt/d/MasterThings/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/build/lib/libassimp.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xassimp-devx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/assimp" TYPE FILE FILES
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/anim.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/aabb.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/ai_assert.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/camera.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/color4.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/color4.inl"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/build/External/assimp/code/../include/assimp/config.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/ColladaMetaData.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/commonMetaData.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/defs.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/cfileio.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/light.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/material.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/material.inl"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/matrix3x3.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/matrix3x3.inl"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/matrix4x4.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/matrix4x4.inl"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/mesh.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/ObjMaterial.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/pbrmaterial.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/GltfMaterial.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/postprocess.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/quaternion.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/quaternion.inl"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/scene.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/metadata.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/texture.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/types.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/vector2.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/vector2.inl"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/vector3.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/vector3.inl"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/version.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/cimport.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/AssertHandler.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/importerdesc.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/Importer.hpp"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/DefaultLogger.hpp"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/ProgressHandler.hpp"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/IOStream.hpp"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/IOSystem.hpp"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/Logger.hpp"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/LogStream.hpp"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/NullLogger.hpp"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/cexport.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/Exporter.hpp"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/DefaultIOStream.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/DefaultIOSystem.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/ZipArchiveIOSystem.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/SceneCombiner.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/fast_atof.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/qnan.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/BaseImporter.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/Hash.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/MemoryIOWrapper.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/ParsingUtils.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/StreamReader.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/StreamWriter.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/StringComparison.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/StringUtils.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/SGSpatialSort.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/GenericProperty.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/SpatialSort.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/SkeletonMeshBuilder.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/SmallVector.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/SmoothingGroups.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/SmoothingGroups.inl"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/StandardShapes.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/RemoveComments.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/Subdivision.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/Vertex.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/LineSplitter.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/TinyFormatter.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/Profiler.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/LogAux.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/Bitmap.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/XMLTools.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/IOStreamBuffer.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/CreateAnimMesh.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/XmlParser.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/BlobIOSystem.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/MathFunctions.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/Exceptional.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/ByteSwapper.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/Base64.hpp"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xassimp-devx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/assimp/Compiler" TYPE FILE FILES
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/Compiler/pushpack1.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/Compiler/poppack1.h"
    "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine/External/assimp/code/../include/assimp/Compiler/pstdint.h"
    )
endif()

