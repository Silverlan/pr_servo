// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.modules.servo:lua_bindings;

export import pragma.lua;

export namespace servo {
	void register_lua_bindings(Lua::Interface &l);
};
