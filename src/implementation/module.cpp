// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.modules.servo;

import :libservo;
import pragma.client;

#pragma clang optimize off

extern "C"
{
    PR_EXPORT bool pragma_attach(std::string &outErr)
    {
    	servo::capi::initialize();
    		/*ServoWebViewBuilder* webview_builder = servo_webview_builder_create(servo, context);
    		//auto *builder = servo_builder_create();

    		ServoOptions* options = servo_options_create();
    		servo_builder_set_options(builder, options);

    		ServoPreferences* preferences = servo_preferences_create();
    		servo_builder_set_preferences(builder, preferences);

    servo_options_set_hard_fail(options, true);
    servo_options_set_multiprocess(options, false);
    servo_options_set_force_ipc(options, true);
    servo_options_set_background_hang_monitor(options, false);
    servo_options_set_sandbox(options, true);
    servo_options_set_temporary_storage(options, false);
    servo_options_set_ignore_certificate_errors(options, true);
    servo_options_set_unminify_js(options, false);
    servo_options_set_unminify_css(options, true);
    servo_options_set_debug_option(
        options, SERVO_DIAGNOSTICS_LOGGING_OPTION_STYLE_TREE, true);
    servo_options_enable_time_profiling_to_stdout(options, 0.5);
    servo_options_set_random_pipeline_closure_probability(options, 0.25f);
    servo_options_free(options);

    		 int result = servo_webview_builder_set_url(webview_builder, "https://example.com");

    		servo_builder_free(builder);
    		//servo_builder_set_options(builder, options)
    		//servo_builder_set_preferences(builder, preferences)
    		//servo_builder_set_event_loop_waker(builder, waker)

    		auto *servo_options_create = lib->FindSymbolAddress<ServoOptions*(*)()>("servo_options_create");
    		if(servo_options_create) {
    			Con::COUT<<"Initializing servo..."<<Con::endl;
    			auto options = servo_options_create();
    			Con::COUT<<"Options: "<<options<<Con::endl;
    		}*/


	//auto *state = pragma::Engine::Get()->GetNetworkState(l);
	//std::string err;
	//bool b = state->InitializeLibrary(path, &err, l) != nullptr;


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
        auto &libDemo = lua.RegisterLibrary("servo");
        libDemo[luabind::def("print",+[]() {
            Con::COUT<<"Hello World"<<Con::endl;
        })];
    	libDemo[luabind::def("spin_event_loop",+[]() {
    		servo::capi::Instance::Get()->SpinEventLoop();
		})];
    	libDemo[luabind::def("create_browser",+[](uint32_t width, uint32_t height) -> std::shared_ptr<servo::capi::Browser> {
			auto browser = servo::capi::Browser::Create(width, height);
    		if(!browser)
    			return nullptr;
    		return std::shared_ptr{std::move(*browser)};
		})];

        auto classDef = luabind::class_<servo::capi::Browser>("Browser");
        classDef.def("LoadUrl", &servo::capi::Browser::LoadUrl);
        classDef.def("GetTexture", &servo::capi::Browser::GetTexture);
        libDemo[classDef];
    }
};
