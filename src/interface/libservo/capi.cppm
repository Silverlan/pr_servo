// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.modules.servo:libservo.capi;

export import pragma.util;

export namespace servo::capi {
	// See https://github.com/servo/servo/tree/main/ffi/capi
	struct RenderingContext;
	struct Servo;
	struct ServoBuilder;
	struct ServoOptions;
	struct ServoWebViewBuilder;
	struct WebView;

	struct ServoWebViewDelegate {
		void *userData;
		void(*on_load_status_changed)(WebView *, int32_t, void*);
		void(*on_new_frame_ready)(WebView *, void*);
	};

	Servo*(*servo_builder_build)(ServoBuilder*) = nullptr;
	ServoBuilder*(*servo_builder_create)() = nullptr;
	void(*servo_builder_free)(ServoBuilder*) = nullptr;
	void(*servo_builder_set_event_loop_waker)(ServoBuilder*, void(*)()) = nullptr;
	void(*servo_free)(Servo*) = nullptr;
	ServoOptions*(*servo_options_create)() = nullptr;
	void(*servo_options_set_multiprocess)(ServoOptions*, bool) = nullptr;
	void(*servo_builder_set_options)(ServoBuilder*, ServoOptions*) = nullptr;
	RenderingContext*(*servo_rendering_context_create_software)(uint32_t, uint32_t) = nullptr;
	void(*servo_rendering_context_free)(RenderingContext*) = nullptr;
	void(*servo_setup_logging)(Servo*) = nullptr;
	void(*servo_spin_event_loop)(Servo*) = nullptr;
	WebView*(*servo_webview_builder_build)(ServoWebViewBuilder*) = nullptr;
	ServoWebViewBuilder*(*servo_webview_builder_create)(Servo*, RenderingContext*) = nullptr;
	void(*servo_webview_builder_free)(ServoWebViewBuilder*) = nullptr;
	void(*servo_webview_builder_set_delegate)(ServoWebViewBuilder*, ServoWebViewDelegate) = nullptr;
	int32_t(*servo_webview_builder_set_url)(ServoWebViewBuilder*, const char*) = nullptr;
	void(*servo_webview_free)(WebView*) = nullptr;
	int32_t(*servo_webview_load)(WebView*, const char*) = nullptr;
	void(*servo_webview_paint)(WebView*) = nullptr;
	void(*servo_webview_take_screenshot)(WebView*, void(*)(const uint8_t*, uint32_t, uint32_t, int32_t, void*), void*) = nullptr;

	std::expected<void, std::string> initialize();
	bool is_valid();
	void close();
}
