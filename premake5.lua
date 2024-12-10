-- Define the project name
local projectName = "main"

workspace (projectName)
    configurations { "Debug", "Release" }
    architecture "x86_64"
    startproject (projectName)

project (projectName)
    kind "ConsoleApp"
    language "C++"
    targetdir ("build/%{cfg.buildcfg}")

    files {
        "src/**.h", 
        "src/**.cpp",
        "src/shaders/RayTracing.frag"
    }

    includedirs {
        "include",
        "/usr/include"
    }

    filter "system:linux"
        libdirs { "/usr/lib" }
        buildoptions { "-std=c++17" }
        links { "glfw", "GLEW", "GL" }
    
    filter "system:windows"
        libdirs { "libs", "libs/GLFW" }
        links { "mingw32", "glfw3", "m", "opengl32", "libs/glad/glad.o" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"
        
        if os.host() == "windows" then
            postbuildcommands { "$(TARGETDIR)/" .. projectName .. ".exe" }
        else
            postbuildcommands { "$(TARGETDIR)/" .. projectName }
        end

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"
