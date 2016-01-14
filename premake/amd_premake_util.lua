-- amd_premake_util.lua
-- utility code shared by AMD build scripts

-- _ACTION is a premake global variable and for our usage will be vs2012, vs2013, etc.
-- Strip "vs" from this string to make a suffix for solution and project files.
_AMD_VS_SUFFIX = "_" .. string.gsub(_ACTION, "vs", "")

-- Specify build output directory structure here: e.g. Desktop_2012\x64\DLL_Debug
_AMD_SAMPLE_DIR_LAYOUT  = "Desktop%{_AMD_VS_SUFFIX}/%{cfg.platform}/%{cfg.buildcfg}"

-- Specify WindowsTargetPlatformVersion here for VS2015
_AMD_WIN_SDK_VERSION = "10.0.10240.0"

