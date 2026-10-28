import pragma.client;
#pragma clang optimize off

namespace servolib {
	// See https://github.com/servo/servo/tree/main/ffi/capi
	struct RenderingContext;
	struct Servo;
	struct ServoBuilder;
	struct ServoOptions;
	struct ServoWebViewBuilder;
	struct WebView;

	struct ServoWebViewDelegate {
		const void *userData;
		void(*on_load_status_changed)(WebView *, int32_t, const void*);
		void(*on_new_frame_ready)(WebView *, const void*);
	};

	static Servo*(*servo_builder_build)(ServoBuilder*) = nullptr;
	static ServoBuilder*(*servo_builder_create)() = nullptr;
	static void(*servo_builder_free)(ServoBuilder*) = nullptr;
	static void(*servo_builder_set_event_loop_waker)(ServoBuilder*, void(*)()) = nullptr;
	static void(*servo_free)(Servo*) = nullptr;
	static ServoOptions*(*servo_options_create)() = nullptr;
	static void(*servo_options_set_multiprocess)(ServoOptions*, bool) = nullptr;
	static void(*servo_builder_set_options)(ServoBuilder*, ServoOptions*) = nullptr;
	static RenderingContext*(*servo_rendering_context_create_software)(uint32_t, uint32_t) = nullptr;
	static void(*servo_rendering_context_free)(RenderingContext*) = nullptr;
	static void(*servo_setup_logging)(Servo*) = nullptr;
	static void(*servo_spin_event_loop)(Servo*) = nullptr;
	static WebView*(*servo_webview_builder_build)(ServoWebViewBuilder*) = nullptr;
	static ServoWebViewBuilder*(*servo_webview_builder_create)(Servo*, RenderingContext*) = nullptr;
	static void(*servo_webview_builder_free)(ServoWebViewBuilder*) = nullptr;
	static void(*servo_webview_builder_set_delegate)(ServoWebViewBuilder*, ServoWebViewDelegate) = nullptr;
	static int32_t(*servo_webview_builder_set_url)(ServoWebViewBuilder*, const char*) = nullptr;
	static void(*servo_webview_free)(WebView*) = nullptr;
	static int32_t(*servo_webview_load)(WebView*, const char*) = nullptr;
	static void(*servo_webview_paint)(WebView*) = nullptr;
	static void(*servo_webview_take_screenshot)(WebView*, void(*)(const uint8_t*, uint32_t, uint32_t, int32_t, const void*), const void*) = nullptr;

	template<typename T>
	void load_symbol(pragma::util::Library &lib, std::string_view symbolName, T *outFunc)
	{
		*outFunc = lib.FindSymbolAddress<T>(std::string{symbolName});
		if(!*outFunc)
			throw std::runtime_error {std::format("Failed to find symbol '{}' in servo library!", symbolName)};
	}

	static void load_symbols(pragma::util::Library &lib) {
		load_symbol(lib, "servo_builder_build", &servo_builder_build);
		load_symbol(lib, "servo_builder_create", &servo_builder_create);
		load_symbol(lib, "servo_builder_free", &servo_builder_free);
		load_symbol(lib, "servo_builder_set_event_loop_waker", &servo_builder_set_event_loop_waker);
		load_symbol(lib, "servo_free", &servo_free);
		load_symbol(lib, "servo_options_create", &servo_options_create);
		load_symbol(lib, "servo_options_set_multiprocess", &servo_options_set_multiprocess);
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
}

namespace servo {
	class Instance {
	public:
		static std::expected<void, std::string> Initialize();
		static Instance *Get();
		servolib::Servo *GetServo() { return m_instance; }
		void SpinEventLoop();
		~Instance();
	private:
		Instance(servolib::Servo *instance);
		servolib::Servo *m_instance = nullptr;
	};

	class Browser {
	public:
		static std::expected<std::unique_ptr<Browser>, std::string> Create(uint32_t width, uint32_t height);
		~Browser();
		void DebugDumpScreenshot();
		void LoadUrl();
	private:
		Browser(servolib::WebView *webView);
		servolib::WebView *m_webView = nullptr;
	};
}

static std::atomic_int waker_triggered;
static void wake_callback(void) { std::atomic_store(&waker_triggered, 1); }
static std::unique_ptr<servo::Instance> g_servoInstance {};

std::expected<void, std::string> servo::Instance::Initialize()
{
	if(g_servoInstance)
		return {};
	auto *builder = servolib::servo_builder_create();
	if (builder == nullptr)
		return std::unexpected {"Failed to create servo builder."};
	auto *options = servolib::servo_options_create();
	if (options != nullptr) {
		servolib::servo_options_set_multiprocess(options, false);
		servolib::servo_builder_set_options(builder, options);
	}

	servolib::servo_builder_set_event_loop_waker(builder, wake_callback);
	auto *servo = servolib::servo_builder_build(builder);
	if(!servo) {
		servolib::servo_builder_free(builder);
		return std::unexpected {"Failed to build servo builder."};
	}
	servolib::servo_setup_logging(servo);

	auto instance = std::unique_ptr<Instance>{new Instance {servo}};
	g_servoInstance = std::move(instance);
	return {};
}
void servo::Instance::SpinEventLoop() { servolib::servo_spin_event_loop(m_instance); }
servo::Instance *servo::Instance::Get() { return g_servoInstance.get(); }

servo::Instance::Instance(servolib::Servo *instance) : m_instance {instance} {}
servo::Instance::~Instance() { servolib::servo_free(m_instance); }

typedef enum {
	TEST_STEP_STARTING = 0,
	TEST_STEP_HEAD_PARSED,
	TEST_STEP_LOAD_COMPLETE,
	TEST_STEP_FRAME_READY,
	TEST_STEP_SCREENSHOT_DONE,
	TEST_STEP_SCREENSHOT_FAILED,
} test_step_t;
typedef struct {
	// `step` is not atomic since it is modified by the delegate
	// callbacks that are run on the main thread during spin.
	test_step_t step;
	// The dimensions of the software rendering context.
	uint32_t expected_width;
	uint32_t expected_height;
} test_state_t;

static void on_screenshot(const uint8_t* data, uint32_t width, uint32_t height,
	int32_t error, const void* user_data)
{
    test_state_t* state = (test_state_t*)user_data;
    if (data == nullptr) {
        state->step = TEST_STEP_SCREENSHOT_FAILED;
        std::cout<<std::format("  [screenshot] failed with error=%d\n", error)<<std::endl;
        return;
    }
    if (width != state->expected_width || height != state->expected_height) {
        state->step = TEST_STEP_SCREENSHOT_FAILED;
        std::cout<<std::format("  [screenshot] incorrect dimensions: expected %ux%u, got %ux%u\n",
            state->expected_width, state->expected_height, width, height)<<std::endl;
        return;
    }
    uint32_t pixel_count = width * height;
    for (uint32_t i = 0; i < pixel_count; i++) {
        uint32_t offset = i * 4;
        if (data[offset] != 255 || data[offset + 1] != 0 || data[offset + 2] != 0 || data[offset + 3] != 255) {
            state->step = TEST_STEP_SCREENSHOT_FAILED;
            std::cout<<std::format("  [screenshot] non-red pixel at (%u, %u) (R=%d, G=%d, B=%d, A=%d)\n",
                i % width, i / width, data[offset], data[offset + 1],
                data[offset + 2], data[offset + 3])<<std::endl;
            return;
        }
    }
    state->step = TEST_STEP_SCREENSHOT_DONE;
    std::cout<<std::format("  [screenshot] all %u pixels red (255,0,0,255)  size=%ux%u\n",
        pixel_count, width, height)<<std::endl;
}

std::expected<std::unique_ptr<servo::Browser>, std::string> servo::Browser::Create(uint32_t width, uint32_t height)
{
	if (auto res = Instance::Initialize(); !res)
		return std::unexpected {std::format("Failed to initialize servo instance: {}", res.error())};
	auto *context = servolib::servo_rendering_context_create_software(width, height);
	if (!context)
		return std::unexpected {"Failed to create servo rendering context."};
	auto *servo = Instance::Get()->GetServo();
	auto *webviewBuilder = servolib::servo_webview_builder_create(servo, context);
	if (!webviewBuilder) {
		servolib::servo_rendering_context_free(context);
		return std::unexpected {"Failed to create servo webview."};
	}

	test_state_t state;
	memset(&state, 0, sizeof(state));
	servolib::ServoWebViewDelegate delegate {
		&state,
		+[](servolib::WebView *webView, int32_t status, const void *userData) {
			std::cout<<"Load status changed: "<<status<<std::endl;
		},
		+[](servolib::WebView *webView, const void *userData) {
			std::cout<<"New frame ready"<<std::endl;
			test_state_t* state = (test_state_t*)userData;
			state->step = TEST_STEP_FRAME_READY;
			servolib::servo_webview_paint(webView);
			servolib::servo_webview_take_screenshot(webView, on_screenshot, userData);
		},
	};
	servolib::servo_webview_builder_set_delegate(webviewBuilder, delegate);

	const char* url = "data:text/html,<html><body style='background:red'></body></html>";
	if (servolib::servo_webview_builder_set_url(webviewBuilder, url) != 0) {
		std::cout<<"FAILED"<<std::endl;
		//log("  FAIL: servo_webview_builder_set_url failed for url '%s'\n", url);
		servolib::servo_webview_builder_free(webviewBuilder);
		return std::unexpected {"Test"};
	}
	std::cout<<"SET URL SUCCESS"<<std::endl;

	auto *webView = servolib::servo_webview_builder_build(webviewBuilder);
	if(!webView) {
		servolib::servo_webview_builder_free(webviewBuilder);
		servolib::servo_rendering_context_free(context);
		return std::unexpected {"Failed to build servo webview."};
	}


	state.expected_width = width;
	state.expected_height = height;
	std::cout<<std::format("  Spinning event loop... (timeout 30s)\n")<<std::endl;
	{
		int elapsed_ms = 0;
		int poll_ms = 15;
		while (elapsed_ms < 30000) {
			servolib::servo_spin_event_loop(servo);
			if (state.step >= TEST_STEP_SCREENSHOT_DONE) {
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds{poll_ms});
			elapsed_ms += poll_ms;
		}
	}

	// Validate the screenshot
	test_step_t final_step = state.step;
	if (final_step == TEST_STEP_SCREENSHOT_FAILED) {
		std::cout<<std::format("  FAIL: screenshot failed\n")<<std::endl;
		return std::unexpected {""};
		//goto cleanup;
	}
	if (final_step != TEST_STEP_SCREENSHOT_DONE) {
		std::cout<<std::format("  FAIL: screenshot callback never fired (step=%d)\n", (int)final_step)<<std::endl;
		return std::unexpected {""};
		//goto cleanup;
	}

	if (!atomic_load(&waker_triggered)) {
		std::cout<<std::format("  FAIL: event loop waker was never called\n")<<std::endl;
		return std::unexpected {""};
		//goto cleanup;
	}

	//result = 0;
	std::cout<<std::format("  PASS: all pixels verified\n")<<std::endl;




	auto browser = std::unique_ptr<Browser> {new Browser{webView}};
	return std::move(browser);
}

servo::Browser::Browser(servolib::WebView *webView) : m_webView {webView} {}
servo::Browser::~Browser()
{
	servolib::servo_webview_free(m_webView);
}

void servo::Browser::DebugDumpScreenshot()
{
	servolib::servo_webview_paint(m_webView);
	servolib::servo_webview_take_screenshot(m_webView, +[](const uint8_t *data, uint32_t width, uint32_t height, int32_t error, const void *userData) {
		std::cout<<"Got data..."<<std::endl;
	}, nullptr);
}

void servo::Browser::LoadUrl()
{
    const char* url = "data:text/html,<html><body style='background:red'></body></html>";
    if (servolib::servo_webview_load(m_webView, url) != 0) {
    	std::cout<<"FAILED"<<std::endl;
        //log("  FAIL: servo_webview_builder_set_url failed for url '%s'\n", url);
        return;
    }
	std::cout<<"LOAD SUCCESS"<<std::endl;
}

extern "C"
{
    // Called after the module has been loaded
    PR_EXPORT bool pragma_attach(std::string &outErr)
    {
        // Return true to indicate that the module has been loaded successfully.
        // If the module could not be loaded properly, return false and populate 'outErr' with a descriptive error message.
        Con::COUT<<"Custom module \"pr_servo\" has been loaded!"<<Con::endl;

    	std::string err;
    	auto lib = pragma::util::load_library_module("servo/libservo_capi", {}, {}, &err);
    	if (lib) {
    		lib->SetDontFreeLibraryOnDestruct();

    		try {
    			servolib::load_symbols(*lib);
    		}
    		catch(const std::runtime_error &e) {
    			std::cout<<"Error: "<<e.what()<<std::endl;
    		}

    		auto browser = servo::Browser::Create(512, 512);
    		if (browser)
    			std::cout<<"Browser created!"<<std::endl;

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
    	}

	//auto *state = pragma::Engine::Get()->GetNetworkState(l);
	//std::string err;
	//bool b = state->InitializeLibrary(path, &err, l) != nullptr;


        return true;
    }

    // Called when the module is about to be unloaded
    PR_EXPORT void pragma_detach()
    {
        Con::COUT<<"Custom module \"pr_servo\" is about to be unloaded!"<<Con::endl;
    }

    // Lua bindings can be initialized here
    PR_EXPORT void pragma_initialize_lua(Lua::Interface &lua)
    {
        auto &libDemo = lua.RegisterLibrary("servo");
        libDemo[luabind::def("print",+[]() {
            Con::COUT<<"Hello World"<<Con::endl;
        })];
    	libDemo[luabind::def("spin_event_loop",+[]() {
    		servo::Instance::Get()->SpinEventLoop();
		})];
    	libDemo[luabind::def("create_browser",+[](uint32_t width, uint32_t height) -> std::shared_ptr<servo::Browser> {
			auto browser = servo::Browser::Create(width, height);
    		if(!browser)
    			return nullptr;
    		return std::shared_ptr{std::move(*browser)};
		})];

        auto classDef = luabind::class_<servo::Browser>("Browser");
        classDef.def("DebugDumpScreenshot", &servo::Browser::DebugDumpScreenshot);
        classDef.def("LoadUrl", &servo::Browser::LoadUrl);
        libDemo[classDef];
    }

    // Called when the Lua state is about to be closed.
    PR_EXPORT void pragma_terminate_lua(Lua::Interface &lua)
    {
        
    }
};
