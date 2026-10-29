// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.modules.servo;

import :libservo.capi;
import :libservo.instance;
import :logging;

namespace servo::capi {
	template<typename T>
	void load_symbol(pragma::util::Library &lib, std::string_view symbolName, T *outFunc)
	{
		*outFunc = lib.FindSymbolAddress<T>(std::string{symbolName});
		if(!*outFunc)
			throw std::runtime_error {std::format("Failed to find symbol '{}' in servo library!", symbolName)};
	}

	void load_symbols(pragma::util::Library &lib);
}

void servo::capi::load_symbols(pragma::util::Library &lib)
{
	load_symbol(lib, "servo_builder_build", &servo_builder_build);
	load_symbol(lib, "servo_builder_create", &servo_builder_create);
	load_symbol(lib, "servo_builder_free", &servo_builder_free);
	load_symbol(lib, "servo_builder_set_event_loop_waker", &servo_builder_set_event_loop_waker);
	load_symbol(lib, "servo_free", &servo_free);
	load_symbol(lib, "servo_options_create", &servo_options_create);
	load_symbol(lib, "servo_options_set_config_dir", &servo_options_set_config_dir);
	load_symbol(lib, "servo_options_set_hard_fail", &servo_options_set_hard_fail);
	load_symbol(lib, "servo_options_set_multiprocess", &servo_options_set_multiprocess);
	load_symbol(lib, "servo_options_set_temporary_storage", &servo_options_set_temporary_storage);
	load_symbol(lib, "servo_builder_set_options", &servo_builder_set_options);
	load_symbol(lib, "servo_rendering_context_create_software", &servo_rendering_context_create_software);
	load_symbol(lib, "servo_rendering_context_free", &servo_rendering_context_free);
	load_symbol(lib, "servo_setup_logging", &servo_setup_logging);
	load_symbol(lib, "servo_spin_event_loop", &servo_spin_event_loop);
	load_symbol(lib, "servo_webview_builder_build", &servo_webview_builder_build);
	load_symbol(lib, "servo_webview_builder_create", &servo_webview_builder_create);
	load_symbol(lib, "servo_webview_builder_set_delegate", &servo_webview_builder_set_delegate);
	load_symbol(lib, "servo_webview_builder_free", &servo_webview_builder_free);
	load_symbol(lib, "servo_webview_builder_set_url", &servo_webview_builder_set_url);
	load_symbol(lib, "servo_webview_free", &servo_webview_free);
	load_symbol(lib, "servo_webview_load", &servo_webview_load);
	load_symbol(lib, "servo_webview_paint", &servo_webview_paint);
	load_symbol(lib, "servo_webview_take_screenshot", &servo_webview_take_screenshot);
}

static std::shared_ptr<pragma::util::Library> g_capiLib;
static std::optional<bool> g_initializationResult {};
std::expected<void, std::string> servo::capi::initialize()
{
	if(g_initializationResult) {
		if(*g_initializationResult)
			return {};
		return std::unexpected {"Servo was unable to initialize due to a previous error."};
	}
	g_initializationResult = false;
	auto &logger = get_logger();
	std::string err;
	std::string_view libPath;
#ifdef _WIN32
	libPath = "servo/servo_capi";
#else
	libPath = "servo/libservo_capi";
#endif
	auto lib = pragma::util::load_library_module(std::string {libPath}, {}, {}, &err);
	if(!lib)
		return std::unexpected(std::format("Failed to load servo library '{}': {}", libPath, err));

	try {
		load_symbols(*lib);
	}
	catch (const std::runtime_error &e) {
		return std::unexpected(std::format("Failed to lookup servo capi symbols: {}", e.what()));
	}

	g_capiLib = lib;

	if (auto res = Instance::Initialize(); !res)
		return std::unexpected(std::format("Failed to initialize servo instance: {}", res.error()));

	g_initializationResult = true;
	return {};
}
bool servo::capi::is_valid() { return g_initializationResult && *g_initializationResult; }
void servo::capi::close()
{
	Instance::Close();
	g_capiLib = nullptr;
}
