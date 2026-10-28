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

	typedef enum {
		TEST_STEP_STARTING = 0,
		TEST_STEP_HEAD_PARSED,
		TEST_STEP_LOAD_COMPLETE,
		TEST_STEP_FRAME_READY,
		TEST_STEP_SCREENSHOT_DONE,
		TEST_STEP_SCREENSHOT_FAILED,
	} test_step_t;
class Browser;
	typedef struct {
		Browser *browser = nullptr;
		// `step` is not atomic since it is modified by the delegate
		// callbacks that are run on the main thread during spin.
		test_step_t step = TEST_STEP_STARTING;
		// The dimensions of the software rendering context.
		uint32_t expected_width = 0;
		uint32_t expected_height = 0;
	} test_state_t;

	class Browser {
	public:
		static std::expected<std::unique_ptr<Browser>, std::string> Create(uint32_t width, uint32_t height);
		~Browser();
		void LoadUrl(std::string_view url);
		test_state_t &GetState() { return m_state; }
		std::vector<std::array<uint8_t, 4>> &GetStagingData() { return m_stagingData; }
		std::shared_ptr<prosper::Texture> GetTexture() { return m_texture; }
		void *GetImageDataPointer() { return m_imgDataPtr; }
	private:
		Browser();
		void ClearTexture();
		void SetWebView(servolib::WebView *webView);
		bool Resize(uint32_t width, uint32_t height);
		void UpdateImage(prosper::ICommandBuffer &drawCmd);
		uint32_t m_width = 0;
		uint32_t m_height = 0;
		test_state_t m_state;
		servolib::WebView *m_webView = nullptr;
		std::shared_ptr<prosper::Texture> m_texture = nullptr;
		std::shared_ptr<prosper::IBuffer> m_stagingBuffer = nullptr;
		void *m_imgDataPtr = nullptr;
		std::vector<std::array<uint8_t, 4>> m_stagingData;
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

static void on_screenshot(const uint8_t* data, uint32_t width, uint32_t height,
	int32_t error, const void* user_data)
{
   auto* state = (servo::test_state_t*)user_data;
    if (data == nullptr) {
        state->step = servo::TEST_STEP_SCREENSHOT_FAILED;
        std::cout<<std::format("  [screenshot] failed with error=%d\n", error)<<std::endl;
        return;
    }
    if (width != state->expected_width || height != state->expected_height) {
        state->step = servo::TEST_STEP_SCREENSHOT_FAILED;
        std::cout<<std::format("  [screenshot] incorrect dimensions: expected %ux%u, got %ux%u\n",
            state->expected_width, state->expected_height, width, height)<<std::endl;
        return;
    }
    uint32_t pixel_count = width * height;
	//memcpy(state->browser->GetStagingData().data(), data, pixel_count *4);
	memcpy(state->browser->GetImageDataPointer(), data, pixel_count *4);

    state->step = servo::TEST_STEP_SCREENSHOT_DONE;
    std::cout<<std::format("  [screenshot] all %u pixels red (255,0,0,255)  size=%ux%u\n",
        pixel_count, width, height)<<std::endl;
}

static std::vector<servo::Browser *> g_webElements {};
static CallbackHandle g_preRecordGuiCb {};
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

	auto browser = std::unique_ptr<Browser> {new Browser{}};
	browser->Resize(width, height);

	auto &state = browser->GetState();
	state.browser = browser.get();
	servolib::ServoWebViewDelegate delegate {
		&state,
		+[](servolib::WebView *webView, int32_t status, const void *userData) {
			std::cout<<"Load status changed: "<<status<<std::endl;
		},
		+[](servolib::WebView *webView, const void *userData) {
			std::cout<<"New frame ready"<<std::endl;
			auto* state = (servo::test_state_t*)userData;
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

	browser->SetWebView(webView);


	if(g_webElements.empty()) {
		g_preRecordGuiCb = pragma::get_cengine()->AddCallback("PreRecordGUI", FunctionCallback<void>::Create([]() {
			auto &window = pragma::get_cengine()->GetWindow();
			auto &drawCmd = window.GetDrawCommandBuffer();
			servo::Instance::Get()->SpinEventLoop();
			for(auto *el : g_webElements)
				el->UpdateImage(*drawCmd);
		}));
	}
	g_webElements.push_back(browser.get());

	return std::move(browser);
}

servo::Browser::Browser() {}
servo::Browser::~Browser()
{
	ClearTexture();
	if(m_webView)
		servolib::servo_webview_free(m_webView);

	auto it = std::find(g_webElements.begin(), g_webElements.end(), this);
	if(it != g_webElements.end())
		g_webElements.erase(it);
	if(g_webElements.empty() && g_preRecordGuiCb.IsValid())
		g_preRecordGuiCb.Remove();
}

void servo::Browser::ClearTexture()
{
	if(m_imgDataPtr) {
		m_stagingBuffer->Unmap();
		m_imgDataPtr = nullptr;
	}
	m_texture = nullptr;
}

void servo::Browser::SetWebView(servolib::WebView *webView) { m_webView = webView; }

void servo::Browser::UpdateImage(prosper::ICommandBuffer &drawCmd)
{
	auto &img = m_texture->GetImage();
	drawCmd.RecordImageBarrier(img, prosper::ImageLayout::ShaderReadOnlyOptimal, prosper::ImageLayout::TransferDstOptimal);

	std::array<std::tuple<int, int, int, int>, 1> dirtyRects {
		std::tuple<int, int, int, int> {0, 0, m_width, m_height}
	};

	prosper::util::BufferImageCopyInfo copyInfo {};
	auto pixelSize = img.GetPixelSize();
	for(auto &rect : dirtyRects) {
		copyInfo.imageOffset = {std::get<0>(rect), std::get<1>(rect)};
		copyInfo.imageExtent = Vector2i {std::get<2>(rect), std::get<3>(rect)};
		copyInfo.bufferOffset = (copyInfo.imageOffset.y * img.GetWidth() + copyInfo.imageOffset.x) * pixelSize;
		copyInfo.bufferExtent = Vector2i {img.GetWidth(), img.GetHeight()};

		drawCmd.RecordCopyBufferToImage(copyInfo, *m_stagingBuffer, img);
	}

	drawCmd.RecordImageBarrier(img, prosper::ImageLayout::TransferDstOptimal, prosper::ImageLayout::ShaderReadOnlyOptimal);
}

bool servo::Browser::Resize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
	Vector2i m_browserViewSize {width, height};
	if(m_texture) {
		auto &img = m_texture->GetImage();
		if(img.GetWidth() == m_browserViewSize.x && img.GetHeight() == m_browserViewSize.y)
			return true;
	}
	ClearTexture();

	auto &context = pragma::gui::WGUI::GetInstance().GetContext();
	if(m_browserViewSize.x == 0 || m_browserViewSize.y == 0)
		return false;
	prosper::util::ImageCreateInfo imgCreateInfo {};
	imgCreateInfo.format = prosper::Format::R8G8B8A8_UNorm;
	imgCreateInfo.width = m_browserViewSize.x;
	imgCreateInfo.height = m_browserViewSize.y;
	imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
	imgCreateInfo.tiling = prosper::ImageTiling::Linear;
	imgCreateInfo.usage = prosper::ImageUsageFlags::SampledBit;
	imgCreateInfo.postCreateLayout = prosper::ImageLayout::ShaderReadOnlyOptimal;

	auto img = context.CreateImage(imgCreateInfo);
	prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
	imgViewCreateInfo.swizzleRed = prosper::ComponentSwizzle::B;
	imgViewCreateInfo.swizzleBlue = prosper::ComponentSwizzle::R;
	prosper::util::SamplerCreateInfo samplerCreateInfo {};
	auto tex = context.CreateTexture({}, *img, imgViewCreateInfo, samplerCreateInfo);
	if(tex == nullptr)
		return false;

	prosper::util::BufferCreateInfo bufCreateInfo {};
	bufCreateInfo.size = imgCreateInfo.width * imgCreateInfo.height * prosper::util::get_pixel_size(imgCreateInfo.format);
	bufCreateInfo.flags |= prosper::util::BufferCreateInfo::Flags::Persistent;
	bufCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::CPUToGPU | prosper::MemoryFeatureFlags::HostCoherent;

	// Clear to white color
	auto clearCol = colors::White;
	m_stagingData.clear();
	m_stagingData.resize(imgCreateInfo.width * imgCreateInfo.height, std::array<uint8_t, 4> {static_cast<uint8_t>(clearCol.r), static_cast<uint8_t>(clearCol.g), static_cast<uint8_t>(clearCol.b), static_cast<uint8_t>(clearCol.a)});

	auto stagingBuffer = context.CreateBuffer(bufCreateInfo, m_stagingData.data());
	if(!stagingBuffer)
		return false;

	m_texture = tex;
	m_stagingBuffer = stagingBuffer;
	stagingBuffer->Map(0ull, stagingBuffer->GetSize(), prosper::IBuffer::MapFlags::WriteBit | prosper::IBuffer::MapFlags::PersistentBit, &m_imgDataPtr);
	//cef::get_wrapper().render_handler_set_image_data(m_webRenderer.get(), m_imgDataPtr, img->GetWidth(), img->GetHeight());
	//cef::get_wrapper().browser_was_resized(GetBrowser());
	//cef::get_wrapper().render_handler_clear_dirty_rects(m_webRenderer.get());
	return true;
}

void servo::Browser::LoadUrl(std::string_view url)
{
    if (servolib::servo_webview_load(m_webView, std::string{url}.c_str()) != 0) {
    	std::cout<<"FAILED"<<std::endl;
        //log("  FAIL: servo_webview_builder_set_url failed for url '%s'\n", url);
        return;
    }
	std::cout<<"LOAD SUCCESS"<<std::endl;
}

extern "C"
{
    PR_EXPORT bool pragma_attach(std::string &outErr)
    {
    	std::string err;
    	std::string_view libPath;
#ifdef _WIN32
    	libPath = "servo/servo_capi";
#else
    	libPath = "servo/libservo_capi";
#endif
    	auto lib = pragma::util::load_library_module(std::string {libPath}, {}, {}, &err);
    	if (lib) {
    		lib->SetDontFreeLibraryOnDestruct();

    		try {
    			servolib::load_symbols(*lib);
    		}
    		catch(const std::runtime_error &e) {
    			std::cout<<"Error: "<<e.what()<<std::endl;
    		}

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

    PR_EXPORT void pragma_detach()
    {
    	g_servoInstance = nullptr;
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
        classDef.def("LoadUrl", &servo::Browser::LoadUrl);
        classDef.def("GetTexture", &servo::Browser::GetTexture);
        libDemo[classDef];
    }

    // Called when the Lua state is about to be closed.
    PR_EXPORT void pragma_terminate_lua(Lua::Interface &lua)
    {
        
    }
};
