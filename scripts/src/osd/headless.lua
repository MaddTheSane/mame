-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

---------------------------------------------------------------------------
--
--   sdl.lua
--
--   Rules for the building with SDL
--
---------------------------------------------------------------------------

dofile("headless_modules.lua")

function maintargetosdoptions(_target,_subtarget)
	osdmodulestargetconf()
end

--forcedincludes {
--	MAME_DIR .. "src/osd/headless/headlessprefix.h"
--}

newoption {
	trigger = "HEADLESS_INI_PATH",
	description = "Default search path for .ini files",
}

BASE_TARGETOS       = "unix"
SDLOS_TARGETOS      = "unix"
SDL_NETWORK         = ""
if _OPTIONS["targetos"]=="macosx" then
	SDLOS_TARGETOS      = "macosx"
	SDL_NETWORK         = "pcap"
end

_OPTIONS["USE_QTDEBUG"] = "0"
links {
	"m",
	"pthread",
	"util",
}
--[[
project ("osd_" .. _OPTIONS["osd"])
	targetsubdir(_OPTIONS["target"] .."_" .._OPTIONS["subtarget"])
	uuid (os.uuid("osd_" .. _OPTIONS["osd"]))
--	kind (LIBTYPE)
	kind "StaticLib"

	dofile("headless_cfg.lua")
	osdmodulesbuild()

	includedirs {
		MAME_DIR .. "src/emu",
		MAME_DIR .. "src/devices", -- accessing imagedev from debugger
		MAME_DIR .. "src/osd",
		MAME_DIR .. "src/lib",
		MAME_DIR .. "src/lib/util",
		MAME_DIR .. "src/osd/headless",
		MAME_DIR .. "src/osd/headless/public",
		MAME_DIR .. "src/osd/modules/file",
		MAME_DIR .. "src/osd/modules/render",
		MAME_DIR .. "3rdparty",
	}

	files {
		MAME_DIR .. "src/osd/headless/headlessmain.mm",
		MAME_DIR .. "src/osd/headless/driver.mm",
		MAME_DIR .. "src/osd/osdepend.h",

        -- headless interface
		MAME_DIR .. "src/osd/headless/public/osd.h",
	}

--]]

project ("ocore_" .. _OPTIONS["osd"])
	targetsubdir(_OPTIONS["target"] .."_" .. _OPTIONS["subtarget"])
	uuid (os.uuid("ocore_" .. _OPTIONS["osd"]))
--	kind (LIBTYPE)
	kind "StaticLib"

	removeflags {
		"SingleOutputDir",
	}

	dofile("headless_cfg.lua")

	includedirs {
		MAME_DIR .. "src/emu",
		MAME_DIR .. "src/osd",
		MAME_DIR .. "src/lib",
		MAME_DIR .. "src/lib/util",
		MAME_DIR .. "src/osd/headless",
	}

	if _OPTIONS["targetos"]=="macosx" then
		BASE_TARGETOS = "unix"
		SDLOS_TARGETOS = "macosx"
		SYNC_IMPLEMENTATION = "ntc"
	end

	files {
		MAME_DIR .. "src/osd/modules/lib/osdlib_headless.cpp",
		MAME_DIR .. "src/osd/osdcore.cpp",
		MAME_DIR .. "src/osd/osdcore.h",
		MAME_DIR .. "src/osd/strconv.cpp",
		MAME_DIR .. "src/osd/strconv.h",
		MAME_DIR .. "src/osd/osdsync.cpp",
		MAME_DIR .. "src/osd/osdsync.h",
		MAME_DIR .. "src/osd/modules/osdmodule.cpp",
		MAME_DIR .. "src/osd/modules/osdmodule.h",
		MAME_DIR .. "src/osd/modules/lib/osdlib.h",
		MAME_DIR .. "src/osd/modules/file/posixdir.cpp",
		MAME_DIR .. "src/osd/modules/file/posixdomain.cpp",
		MAME_DIR .. "src/osd/modules/file/posixfile.cpp",
		MAME_DIR .. "src/osd/modules/file/posixfile.h",
		MAME_DIR .. "src/osd/modules/file/posixptty.cpp",
		MAME_DIR .. "src/osd/modules/file/posixsocket.cpp",
	}

