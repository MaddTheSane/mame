-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

dofile("headless_modules.lua")

--forcedincludes {
--	MAME_DIR .. "src/osd/headless/headlessprefix.h"
--}

if SDL_NETWORK~="" and not _OPTIONS["DONT_USE_NETWORK"] then
	defines {
		"USE_NETWORK",
		"OSD_NET_USE_" .. string.upper(SDL_NETWORK),
	}
end

if _OPTIONS["HEADLESS_INI_PATH"]~=nil then
	defines {
		"'INI_PATH=\"" .. _OPTIONS["HEADLESS_INI_PATH"] .. "\"'",
	}
end

defines {
	"SDLMAME_NO_X11",
	"HEADLESSMAME",
	"USE_XINPUT=0",
	"OSD_HEADLESS",
}

if BASE_TARGETOS=="unix" then
	defines {
		"SDLMAME_UNIX",
		"HEADLESSMAME_UNIX",
	}
end

if _OPTIONS["targetos"]=="macosx" then
	defines {
		"SDLMAME_DARWIN",
	}
end

-- configuration { "macosx" }
-- 	includedirs {
-- 		MAME_DIR .. "3rdparty/bx/include/compat/osx",
-- 	}

configuration { }

