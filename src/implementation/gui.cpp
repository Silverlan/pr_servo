// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.modules.servo;

import :gui;
import :logging;
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

void pragma::gui::types::ServoBrowser::LoadUrl(std::string_view url)
{
	if(!m_browser)
		return;
	m_browser->LoadUrl(url);
	m_url = url;
}

const std::string &pragma::gui::types::ServoBrowser::GetUrl() const { return m_url; }

void pragma::gui::types::ServoBrowser::SetInitialUrl(std::string_view url) { m_initialUrl = std::string{url}; }

void pragma::gui::types::ServoBrowser::SetBrowserViewSize(Vector2i size)
{
	size.x = pragma::math::max(size.x, 1);
	size.y = pragma::math::max(size.y, 1);
	m_browserViewSize = size;
}
const Vector2i &pragma::gui::types::ServoBrowser::GetBrowserViewSize() const { return m_browserViewSize; }

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
	InitializeBrowser();
	Resize();
}
bool pragma::gui::types::ServoBrowser::Resize()
{
	// TODO
	return true;
}
void pragma::gui::types::ServoBrowser::InitializeBrowser()
{
	if (m_browserInitialized)
		return;
	m_browserInitialized = true;
	uint32_t width = GetWidth();
	uint32_t height = GetHeight();
	if (width == 0)
		width = 128;
	if (height == 0)
		height = 128;
	std::optional<std::string_view> initialUrl {};
	if(!m_initialUrl.empty())
		initialUrl = m_initialUrl;
	auto browser = servo::capi::Browser::Create(width, height, initialUrl);
	if (!browser) {
		servo::get_logger().error("Failed to create servo browser: {}", browser.error());
		return;
	}
	if(initialUrl)
		m_url = *initialUrl;
	m_browser = std::move(*browser);
	SetTexture(*m_browser->GetTexture());
}
