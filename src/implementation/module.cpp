// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.modules.servo;

import :libservo;
import pragma.client;

extern "C"
{
    PR_EXPORT bool pragma_attach(std::string &outErr)
    {
    	auto res = servo::capi::initialize();
    	if(!res) {
    		outErr = res.error();
    		return false;
    	}
    	pragma::get_cengine()->RegisterGuiType<pragma::gui::types::ServoBrowser>("servo_browser");
        return true;
    }

    PR_EXPORT void pragma_detach()
    {
    	servo::capi::close();
    }

    PR_EXPORT void pragma_initialize_lua(Lua::Interface &lua)
    {
    	if(!servo::capi::is_valid())
    		return;
		servo::register_lua_bindings(lua);
    }
};
