# -*- cmake -*-

include(00-Common)

project(cmake)

set(cmake_SOURCE_FILES
    CMakeLists.txt

    00-Common.cmake
    APR.cmake
    Audio.cmake
    BerkeleyDB.cmake
    Boost.cmake
    BuildVersion.cmake
    CARes.cmake
    CMakeCopyIfDifferent.cmake
    CURL.cmake
    CopyWinLibs.cmake
    CSharpMacros.cmake
    DBusGlib.cmake
    DirectX.cmake
    DownloadPrebuilt.cmake.in
    ELFIO.cmake
    EXPAT.cmake
    FindAPR.cmake
    FindBerkeleyDB.cmake
    FindCARes.cmake
    FindELFIO.cmake
    FindGooglePerfTools.cmake
    FindHunSpell.cmake
    FindJsonCpp.cmake
    FindNDOF.cmake
    FindOpenJPEG.cmake
    FindXmlRpcEpi.cmake
    FMOD.cmake
    FMODEX.cmake
    FreeType.cmake
    GLOD.cmake
    GStreamer010Plugin.cmake
    Glui.cmake
    Glut.cmake
    GooglePerfTools.cmake
    Hunspell.cmake
    JPEG.cmake
    JsonCpp.cmake
    LLAddBuildTest.cmake
    LLAppearance.cmake
    LLAudio.cmake
    LLCharacter.cmake
    LLCommon.cmake
    LLCrashLogger.cmake
    LLImage.cmake
    LLImageJ2COJ.cmake
    LLInventory.cmake
    LLKDU.cmake
    LLMath.cmake
    LLMessage.cmake
    LLPhysicsExtensions.cmake
    LLPlugin.cmake
    LLPrimitive.cmake
    LLQtWebkit.cmake
    LLRender.cmake
    LLUI.cmake
    LLVFS.cmake
    LLWindow.cmake
    LLXML.cmake
#    LScript.cmake
    Linking.cmake
    MediaPluginBase.cmake
    NDOF.cmake
    OPENAL.cmake
    OpenGL.cmake
    OpenJPEG.cmake
    OpenSSL.cmake
    PNG.cmake
    Prebuilt.cmake
    PulseAudio.cmake
    Python.cmake
    Qt4.cmake
    QuickTimePlugin.cmake
    RunBuildTest.cmake
    TemplateCheck.cmake
    Tut.cmake
    UI.cmake
    UnixInstall.cmake
    Variables.cmake
    WebKitLibPlugin.cmake
    XmlRpcEpi.cmake
    ZLIB.cmake
    )

source_group("Shared Rules" FILES ${cmake_SOURCE_FILES})

set(master_SOURCE_FILES
    ../CMakeLists.txt
    ../develop.py
    )

source_group("Master Rules" FILES ${master_SOURCE_FILES})

set_source_files_properties(${cmake_SOURCE_FILES} ${master_SOURCE_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

add_library(cmake
            cmake_dummy.cpp
            ${cmake_SOURCE_FILES}
            ${master_SOURCE_FILES}
            )
