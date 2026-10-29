// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.modules.servo:gui;

export import pragma.gui;

export namespace pragma::gui::types {
	class ServoBrowser : public WITexturedRect {
	public:
		ServoBrowser();
		~ServoBrowser() override;
		void Initialize() override;
		void Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd) override;

		void OnCursorEntered() override;
		void OnCursorExited() override;
		void OnCursorMoved(int x, int y) override;
		util::EventReply MouseCallback(platform::MouseButton button, platform::KeyState state, platform::Modifier mods) override;util::EventReply OnDoubleClick() override;
		util::EventReply KeyboardCallback(platform::Key key, int scanCode, platform::KeyState state, platform::Modifier mods) override;
		util::EventReply CharCallback(unsigned int c, platform::Modifier mods = platform::Modifier::None) override;
		util::EventReply ScrollCallback(Vector2 offset, bool offsetAsPixels = false) override;
		void OnFocusGained() override;
		void OnFocusKilled() override;
		void DoUpdate() override;
	private:
	};
}
