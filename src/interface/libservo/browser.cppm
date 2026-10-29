// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.modules.servo:libservo.browser;

export import :libservo.capi;
export import pragma.prosper;

export namespace servo::capi {
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
		void SetWebView(capi::WebView *webView);
		bool Resize(uint32_t width, uint32_t height);
		void UpdateImage(prosper::ICommandBuffer &drawCmd);
		uint32_t m_width = 0;
		uint32_t m_height = 0;
		test_state_t m_state;
		capi::WebView *m_webView = nullptr;
		std::shared_ptr<prosper::Texture> m_texture = nullptr;
		std::shared_ptr<prosper::IBuffer> m_stagingBuffer = nullptr;
		void *m_imgDataPtr = nullptr;
		std::vector<std::array<uint8_t, 4>> m_stagingData;
	};
}
