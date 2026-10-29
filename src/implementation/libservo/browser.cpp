// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.modules.servo;

import :libservo.browser;
import :libservo.capi;
import :libservo.instance;
import :logging;

import pragma.client;

static void on_screenshot(const uint8_t* data, uint32_t width, uint32_t height,
	int32_t error, void* user_data)
{
	auto* state = static_cast<servo::capi::test_state_t*>(user_data);
	if (data == nullptr) {
		state->step = servo::capi::LoadStage::ScreenshotFailed;
		servo::get_logger().trace("Screenshot failed.");
		return;
	}
	if (width != state->expected_width || height != state->expected_height) {
		state->step = servo::capi::LoadStage::ScreenshotFailed;
		servo::get_logger().trace("Incorrect screenshot dimensions: expected {}x{}, got {}x{}", state->expected_width, state->expected_height, width, height);
		return;
	}
	uint32_t pixel_count = width * height;
	memcpy(state->browser->GetImageDataPointer(), data, pixel_count *4);

	state->step = servo::capi::LoadStage::ScreenshotDone;
}

static std::vector<servo::capi::Browser *> g_webElements {};
static CallbackHandle g_preRecordGuiCb {};
std::expected<std::unique_ptr<servo::capi::Browser>, std::string> servo::capi::Browser::Create(uint32_t width, uint32_t height, std::optional<std::string_view> initialUrl)
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
	if(auto res = browser->Resize(width, height); !res) {
		servo_rendering_context_free(context);
		return std::unexpected(std::format("Failed to resize browser: {}", res.error()));
	}

	auto &state = browser->GetState();
	state.browser = browser.get();
	ServoWebViewDelegate delegate {
		&state,
		+[](WebView *webView, int32_t status, void *userData) {
			get_logger().info("Load status changed: {}", status);
		},
		+[](WebView *webView, void *userData) {
			// get_logger().trace("New frame is ready.");
			auto* state = static_cast<test_state_t*>(userData);
			state->step = LoadStage::FrameReady;
			servo_webview_paint(webView);
			servo_webview_take_screenshot(webView, on_screenshot, userData);
		},
	};
	servo_webview_builder_set_delegate(webviewBuilder, delegate);

	if(!initialUrl)
		initialUrl = "data:text/html,<html><body style='background:white'></body></html>";
	if (servo_webview_builder_set_url(webviewBuilder, std::string{*initialUrl}.c_str()) != 0) {
		servo_webview_builder_free(webviewBuilder);
		return std::unexpected {std::format("Failed to load initial url '{}'.", *initialUrl)};
	}

	auto *webView = servo_webview_builder_build(webviewBuilder);
	if(!webView) {
		servo_webview_builder_free(webviewBuilder);
		servo_rendering_context_free(context);
		return std::unexpected {"Failed to build servo webview."};
	}

	state.expected_width = width;
	state.expected_height = height;
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

std::expected<void, std::string> servo::capi::Browser::Resize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
	if(m_texture) {
		auto &img = m_texture->GetImage();
		if(img.GetWidth() == m_width && img.GetHeight() == m_height)
			return {};
	}
	ClearTexture();
	if (m_webView)
		capi::servo_webview_resize(m_webView, width, height);

	auto &context = pragma::gui::WGUI::GetInstance().GetContext();
	if(m_width == 0 || m_height == 0)
		return std::unexpected(std::format("Invalid resolution {}x{}", width, height));
	prosper::util::ImageCreateInfo imgCreateInfo {};
	imgCreateInfo.format = prosper::Format::R8G8B8A8_UNorm;
	imgCreateInfo.width = m_width;
	imgCreateInfo.height = m_height;
	imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
	imgCreateInfo.tiling = prosper::ImageTiling::Linear;
	imgCreateInfo.usage = prosper::ImageUsageFlags::SampledBit;
	imgCreateInfo.postCreateLayout = prosper::ImageLayout::ShaderReadOnlyOptimal;

	auto img = context.CreateImage(imgCreateInfo);
	prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
	prosper::util::SamplerCreateInfo samplerCreateInfo {};
	auto tex = context.CreateTexture({}, *img, imgViewCreateInfo, samplerCreateInfo);
	if(tex == nullptr)
		return std::unexpected("Failed to create prosper texture.");

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
		return std::unexpected("Failed to create prosper staging buffer.");

	m_texture = tex;
	m_stagingBuffer = stagingBuffer;
	m_state.expected_width = m_width;
	m_state.expected_height = m_height;
	stagingBuffer->Map(0ull, stagingBuffer->GetSize(), prosper::IBuffer::MapFlags::WriteBit | prosper::IBuffer::MapFlags::PersistentBit, &m_imgDataPtr);
	return {};
}

void servo::capi::Browser::LoadUrl(std::string_view url)
{
    if (capi::servo_webview_load(m_webView, std::string{url}.c_str()) == 0)
		return;
	get_logger().warn("Failed to load url '{}'.", url);
}

void servo::capi::Browser::NotifyMouseMove(float x, float y)
{
	servo_webview_notify_mouse_move(m_webView, x, y);
}
void servo::capi::Browser::NotifyMouseButton(MouseButtonAction action, MouseButton button, float x, float y)
{
	servo_webview_notify_mouse_button(m_webView, action, button, x, y);
}
void servo::capi::Browser::NotifyWheel(double deltaX, double deltaY, double deltaZ, WheelMode mode, float x, float y)
{
	servo_webview_notify_wheel(m_webView, deltaX, deltaY, deltaZ, mode, x, y);
}
void servo::capi::Browser::NotifyKeyboard(KeyState state, const char *key, const char *code, KeyModifier modifiers)
{
	servo_webview_notify_keyboard(m_webView, state, key, code, modifiers);
}
