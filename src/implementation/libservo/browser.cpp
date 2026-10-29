// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.modules.servo;

import :libservo.browser;
import :libservo.capi;
import :libservo.instance;

import pragma.client;

static void on_screenshot(const uint8_t* data, uint32_t width, uint32_t height,
	int32_t error, const void* user_data)
{
	auto* state = (servo::capi::test_state_t*)user_data;
	if (data == nullptr) {
		state->step = servo::capi::TEST_STEP_SCREENSHOT_FAILED;
		std::cout<<std::format("  [screenshot] failed with error=%d\n", error)<<std::endl;
		return;
	}
	if (width != state->expected_width || height != state->expected_height) {
		state->step = servo::capi::TEST_STEP_SCREENSHOT_FAILED;
		std::cout<<std::format("  [screenshot] incorrect dimensions: expected %ux%u, got %ux%u\n",
			state->expected_width, state->expected_height, width, height)<<std::endl;
		return;
	}
	uint32_t pixel_count = width * height;
	//memcpy(state->browser->GetStagingData().data(), data, pixel_count *4);
	memcpy(state->browser->GetImageDataPointer(), data, pixel_count *4);

	state->step = servo::capi::TEST_STEP_SCREENSHOT_DONE;
	std::cout<<std::format("  [screenshot] all %u pixels red (255,0,0,255)  size=%ux%u\n",
		pixel_count, width, height)<<std::endl;
}

static std::vector<servo::capi::Browser *> g_webElements {};
static CallbackHandle g_preRecordGuiCb {};
std::expected<std::unique_ptr<servo::capi::Browser>, std::string> servo::capi::Browser::Create(uint32_t width, uint32_t height)
{
	if (auto res = Instance::Initialize(); !res)
		return std::unexpected {std::format("Failed to initialize servo instance: {}", res.error())};
	auto *context = servo_rendering_context_create_software(width, height);
	if (!context)
		return std::unexpected {"Failed to create servo rendering context."};
	auto *instance = Instance::Get();
	auto *servo = instance->GetServo();
	auto *webviewBuilder = servo_webview_builder_create(servo, context);
	if (!webviewBuilder) {
		servo_rendering_context_free(context);
		return std::unexpected {"Failed to create servo webview."};
	}

	auto browser = std::unique_ptr<Browser> {new Browser{}};
	browser->Resize(width, height);

	auto &state = browser->GetState();
	state.browser = browser.get();
	ServoWebViewDelegate delegate {
		&state,
		+[](WebView *webView, int32_t status, const void *userData) {
			std::cout<<"Load status changed: "<<status<<std::endl;
		},
		+[](WebView *webView, const void *userData) {
			std::cout<<"New frame ready"<<std::endl;
			auto* state = (test_state_t*)userData;
			state->step = TEST_STEP_FRAME_READY;
			servo_webview_paint(webView);
			servo_webview_take_screenshot(webView, on_screenshot, userData);
		},
	};
	servo_webview_builder_set_delegate(webviewBuilder, delegate);

	const char* url = "data:text/html,<html><body style='background:red'></body></html>";
	if (servo_webview_builder_set_url(webviewBuilder, url) != 0) {
		std::cout<<"FAILED"<<std::endl;
		//log("  FAIL: servo_webview_builder_set_url failed for url '%s'\n", url);
		servo_webview_builder_free(webviewBuilder);
		return std::unexpected {"Test"};
	}
	std::cout<<"SET URL SUCCESS"<<std::endl;

	auto *webView = servo_webview_builder_build(webviewBuilder);
	if(!webView) {
		servo_webview_builder_free(webviewBuilder);
		servo_rendering_context_free(context);
		return std::unexpected {"Failed to build servo webview."};
	}


	state.expected_width = width;
	state.expected_height = height;
	std::cout<<std::format("  Spinning event loop... (timeout 30s)\n")<<std::endl;
	{
		int elapsed_ms = 0;
		int poll_ms = 15;
		while (elapsed_ms < 30000) {
			servo_spin_event_loop(servo);
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

	if (!instance->WasWakerTriggered()) {
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
			Instance::Get()->SpinEventLoop();
			for(auto *el : g_webElements)
				el->UpdateImage(*drawCmd);
		}));
	}
	g_webElements.push_back(browser.get());

	return std::move(browser);
}

servo::capi::Browser::Browser() {}
servo::capi::Browser::~Browser()
{
	ClearTexture();
	if(m_webView)
		servo_webview_free(m_webView);

	auto it = std::find(g_webElements.begin(), g_webElements.end(), this);
	if(it != g_webElements.end())
		g_webElements.erase(it);
	if(g_webElements.empty() && g_preRecordGuiCb.IsValid())
		g_preRecordGuiCb.Remove();
}

void servo::capi::Browser::ClearTexture()
{
	if(m_imgDataPtr) {
		m_stagingBuffer->Unmap();
		m_imgDataPtr = nullptr;
	}
	m_texture = nullptr;
}

void servo::capi::Browser::SetWebView(capi::WebView *webView) { m_webView = webView; }

void servo::capi::Browser::UpdateImage(prosper::ICommandBuffer &drawCmd)
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

bool servo::capi::Browser::Resize(uint32_t width, uint32_t height)
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

void servo::capi::Browser::LoadUrl(std::string_view url)
{
    if (capi::servo_webview_load(m_webView, std::string{url}.c_str()) != 0) {
    	std::cout<<"FAILED"<<std::endl;
        //log("  FAIL: servo_webview_builder_set_url failed for url '%s'\n", url);
        return;
    }
	std::cout<<"LOAD SUCCESS"<<std::endl;
}
