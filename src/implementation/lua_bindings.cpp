// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.modules.servo;

import :lua_bindings;
import :libservo.instance;
import :gui;

void servo::register_lua_bindings(Lua::Interface &lua)
{
	auto &libServo = lua.RegisterLibrary("servo");
	libServo[luabind::def("spin_event_loop",+[]() {
		capi::Instance::Get()->SpinEventLoop();
	})];
	libServo[luabind::def("create_browser",+[](uint32_t width, uint32_t height) -> std::shared_ptr<capi::Browser> {
		auto browser = capi::Browser::Create(width, height);
		if(!browser)
			return nullptr;
		return std::shared_ptr{std::move(*browser)};
	})];

	auto classDef = luabind::class_<capi::Browser>("Browser");
	classDef.def("LoadUrl", &capi::Browser::LoadUrl);
	classDef.def("GetTexture", &capi::Browser::GetTexture);
	libServo[classDef];

	auto classDefWeb = luabind::class_<pragma::gui::types::ServoBrowser, luabind::bases<pragma::gui::types::WITexturedShape, pragma::gui::types::WIShape, pragma::gui::types::WIBase>>("ServoBrowser");
	classDefWeb.def("LoadUrl", +[](pragma::gui::types::ServoBrowser &browser, std::string url) {
		browser.LoadUrl(std::string {url});
	});
	classDefWeb.def("SetInitialUrl", +[](pragma::gui::types::ServoBrowser &browser, std::string url) {
		browser.SetInitialUrl(std::string {url});
	});
	classDefWeb.def("GetUrl", &pragma::gui::types::ServoBrowser::GetUrl);
	classDefWeb.def("SetBrowserViewSize", &pragma::gui::types::ServoBrowser::SetBrowserViewSize);
	classDefWeb.def("GetBrowserViewSize", &pragma::gui::types::ServoBrowser::GetBrowserViewSize, luabind::copy_policy<0> {});
	libServo[classDefWeb];
}
