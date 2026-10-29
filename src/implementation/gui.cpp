// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.modules.servo;

import :gui;
import pragma.client;

pragma::gui::types::ServoBrowser::ServoBrowser() : WITexturedRect{} {}
pragma::gui::types::ServoBrowser::~ServoBrowser() {}
void pragma::gui::types::ServoBrowser::Initialize() {
	WITexturedRect::Initialize();
}
void pragma::gui::types::ServoBrowser::Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd)
{
	WITexturedRect::Think(drawCmd);
}

void pragma::gui::types::ServoBrowser::OnCursorEntered()
{
	WITexturedRect::OnCursorEntered();
}
void pragma::gui::types::ServoBrowser::OnCursorExited()
{
	WITexturedRect::OnCursorExited();
}
void pragma::gui::types::ServoBrowser::OnCursorMoved(int x, int y)
{
	WITexturedRect::OnCursorMoved(x, y);
}
pragma::util::EventReply pragma::gui::types::ServoBrowser::MouseCallback(platform::MouseButton button, platform::KeyState state, platform::Modifier mods)
{
	return WITexturedRect::MouseCallback(button, state, mods);
}
pragma::util::EventReply pragma::gui::types::ServoBrowser::OnDoubleClick()
{
	return WITexturedRect::OnDoubleClick();
}
pragma::util::EventReply pragma::gui::types::ServoBrowser::KeyboardCallback(platform::Key key, int scanCode, platform::KeyState state, platform::Modifier mods)
{
	return WITexturedRect::KeyboardCallback(key, scanCode, state, mods);
}
pragma::util::EventReply pragma::gui::types::ServoBrowser::CharCallback(unsigned int c, platform::Modifier mods)
{
	return WITexturedRect::CharCallback(c, mods);
}
pragma::util::EventReply pragma::gui::types::ServoBrowser::ScrollCallback(Vector2 offset, bool offsetAsPixels)
{
	return WITexturedRect::ScrollCallback(offset, offsetAsPixels);
}
void pragma::gui::types::ServoBrowser::OnFocusGained()
{
	WITexturedRect::OnFocusGained();
}
void pragma::gui::types::ServoBrowser::OnFocusKilled()
{
	WITexturedRect::OnFocusKilled();
}
void pragma::gui::types::ServoBrowser::DoUpdate()
{
	WITexturedRect::DoUpdate();
}
