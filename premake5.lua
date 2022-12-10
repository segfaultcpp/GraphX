workspace "GraphX"
    configurations { "Debug", "Release" }
    platforms { "Win64", "Linux" }
    location "proj_files"
    language "C++"
    cppdialect "C++20"
    architecture "x86_64"
    includedirs { "3rd_party/includes", "includes/" }
    libdirs { "3rd_party/libs" }

    filter "configurations:Debug"
        symbols "ON"
        defines "GX_DEBUG"

    filter "configurations:Release"
        optimize "ON"
        defines "GX_RELEASE"

    filter "platforms:Win64"
        system "windows"
        defines "GX_WIN64"

    filter "platforms:Linux"
        system "linux"
        defines "GX_LINUX"

    filter "toolset:clang or gcc"
        buildoptions "-Wall -Wextra -Wpedantic"

    filter "toolset:msc" 
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
        location "proj_files/tri_example"
        files { "samples/tri_example/**.cpp" }

        links { "GrpahX" }
        kind "ConsoleApp"
