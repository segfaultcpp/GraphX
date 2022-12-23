workspace "GraphX"
    configurations { "Debug", "Release", "ASan", "StaticAnalyzer" }
    platforms { "Win64", "Linux" }
    location "proj_files"
    language "C++"
    cppdialect "C++20"
    architecture "x86_64"
    includedirs { "3rd_party/includes", "includes/" }
    libdirs { "3rd_party/libs" }
    
    filter "platforms:Win64"
        system "windows"
        defines "GX_WIN64"

    filter "platforms:Linux"
        system "linux"
        defines "GX_LINUX"

    filter "configurations:Debug"
        symbols "ON"
        defines "GX_DEBUG"

    filter "configurations:Release"
        optimize "ON"
        defines "GX_RELEASE"
        symbols "OFF"

    --tested only on msvc v19.34
    filter "configurations:ASan"
        symbols "ON"
        defines "GX_ASAN"

    filter "configurations:StaticAnalyzer"
        symbols "ON"
        defines "GX_STATIC_ANALYZER"

    filter { "configurations:ASan", "toolset:clang" }
        buildoptions  "-fsanitize=address"

    filter { "configurations:ASan", "toolset:msc-v143" }
        buildoptions  "/fsanitize=address /Z7 /DEBUG"

    filter { "configurations:StaticAnalyzer", "toolset:msc-v143" }
        buildoptions "/analyze /analyze:plugin EspxEngine.dll"

    filter "toolset:clang"
        buildoptions { 
            "-Wall", "-Wextra", "-Wpedantic", "-Wno-c++98-compat", "-Wno-c++98-compat-pedantic", "-Wno-pre-c++14-compat", "-Wno-pre-c++17-compat",
            "-Wno-ctad-maybe-unsupported", "-Wno-c++20-compat", "-Wno-newline-eof"
        }

    filter "toolset:msc-v143" 
        buildoptions "/W3"

    filter {}

    project "GrpahX"
        filename "gx"
        location "%{wks.location}/gx"
        kind "StaticLib"
        targetdir "build/%{cfg.buildcfg}/%{cfg.platform}"
        files { "src/**.cpp", "includes/**" }
        includedirs { "includes/" }
        links { "%{cfg.platform}/vulkan/vulkan-1.lib" }

    project "TriangleExample"
        targetdir "samples/build/%{cfg.buildcfg}/%{cfg.platform}"
        filename "tri_example"
        location "%{wks.location}/tri_example"
        files { "samples/tri_example/**.cpp" }

        links { "GrpahX" }
        kind "ConsoleApp"
