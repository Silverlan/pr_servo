// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.modules.servo;

import :gui;
import :logging;
import pragma.client;

pragma::gui::types::ServoBrowser::ServoBrowser() : WITexturedRect{} {}
pragma::gui::types::ServoBrowser::~ServoBrowser() {}
void pragma::gui::types::ServoBrowser::Initialize()
{
	WITexturedRect::Initialize();
	SetKeyboardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetMouseMovementCheckEnabled(true);
	SetScrollInputEnabled(true);
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
	size.x = math::max(size.x, 1);
	size.y = math::max(size.y, 1);
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
	m_mousePos = {x, y};
	WIBase::OnCursorMoved(x, y);
	if(!m_browser)
		return;
	auto brMousePos = GetBrowserMousePos();
	m_browser->NotifyMouseMove(brMousePos.x, brMousePos.y);
}
pragma::util::EventReply pragma::gui::types::ServoBrowser::MouseCallback(platform::MouseButton button, platform::KeyState state, platform::Modifier mods)
{
	if (button == platform::MouseButton::Left)
		RequestFocus();

	auto result = WIBase::MouseCallback(button, state, mods);
	if(result == util::EventReply::Handled)
		return result;
	if(!m_browser || (state != platform::KeyState::Press && state != platform::KeyState::Release))
		return util::EventReply::Unhandled;
	auto brMousePos = GetBrowserMousePos();
	auto btType = servo::capi::MouseButton::Primary;
	switch(button) {
	case platform::MouseButton::Left:
		btType = servo::capi::MouseButton::Primary;
		break;
	case platform::MouseButton::Right:
		btType = servo::capi::MouseButton::Secondary;
		break;
	case platform::MouseButton::Middle:
		btType = servo::capi::MouseButton::Auxiliary;
		break;
	default:
		return util::EventReply::Unhandled;
	}
	auto action = (state == platform::KeyState::Press) ? servo::capi::MouseButtonAction::Down : servo::capi::MouseButtonAction::Up;
	m_browser->NotifyMouseButton(action, btType, brMousePos.x, brMousePos.y);
	return util::EventReply::Handled;
}
pragma::util::EventReply pragma::gui::types::ServoBrowser::OnDoubleClick()
{
	return WITexturedRect::OnDoubleClick();
}
static servo::capi::KeyModifier get_servo_modifiers(pragma::platform::Modifier mods)
{
	auto servoMods = servo::capi::KeyModifier::None;
	if(pragma::math::is_flag_set(mods, pragma::platform::Modifier::Shift))
		servoMods |= servo::capi::KeyModifier::Shift;
	if(pragma::math::is_flag_set(mods, pragma::platform::Modifier::Alt))
		servoMods |= servo::capi::KeyModifier::Alt;
	if(pragma::math::is_flag_set(mods, pragma::platform::Modifier::Control))
		servoMods |= servo::capi::KeyModifier::Control;
	if(pragma::math::is_flag_set(mods, pragma::platform::Modifier::Super))
		servoMods |= servo::capi::KeyModifier::Meta;
	return servoMods;
}
pragma::util::EventReply pragma::gui::types::ServoBrowser::KeyboardCallback(platform::Key key, int scanCode, platform::KeyState state, platform::Modifier mods)
{
	WIBase::KeyboardCallback(key, scanCode, state, mods);
	if(!m_browser)
		return util::EventReply::Unhandled;
	auto systemKey = platform::to_system_key(key);
	if(systemKey == -1) {
		if((mods & (platform::Modifier::Alt | platform::Modifier::Control | platform::Modifier::Super)) == platform::Modifier::None) {
			// Already handled by CharCallback
			return util::EventReply::Handled;
		}
	}
	auto servoState = (state == platform::KeyState::Press || state == platform::KeyState::Repeat) ? servo::capi::KeyState::Down : servo::capi::KeyState::Up;
	auto servoMods = get_servo_modifiers(mods) | m_buttonMods;
	auto keyboardEventCode = std::string {pragma::platform::glfw_to_w3c_keyboard_event_code(key)};
	auto keyboardEventKey = std::string {pragma::platform::glfw_to_w3c_keyboard_event_key(key)};
	m_browser->NotifyKeyboard(servoState, keyboardEventCode.c_str(), keyboardEventKey.c_str(), servoMods);
	return util::EventReply::Handled;
}
pragma::util::EventReply pragma::gui::types::ServoBrowser::CharCallback(unsigned int c, platform::Modifier mods)
{
	WIBase::CharCallback(c, mods);
	if(!m_browser)
		return util::EventReply::Unhandled;
	std::string key {};
	key += static_cast<char>(c);
	m_browser->NotifyKeyboard(servo::capi::KeyState::Down, key.c_str(), key.c_str(), get_servo_modifiers(mods) | m_buttonMods);
	return util::EventReply::Handled;
}
Vector2i pragma::gui::types::ServoBrowser::GetBrowserMousePos() const
{
	auto *elRoot = GetBaseRootElement();
	if(elRoot && elRoot->GetCursorPosOverride()) {
		int x, y;
		GetMousePos(&x, &y);
		return {x, y};
	}
	return {m_mousePos.x / static_cast<float>(GetWidth()) * m_browserViewSize.x, m_mousePos.y / static_cast<float>(GetHeight()) * m_browserViewSize.y};
}
pragma::util::EventReply pragma::gui::types::ServoBrowser::ScrollCallback(Vector2 offset, bool offsetAsPixels)
{
	WITexturedRect::ScrollCallback(offset, offsetAsPixels);
	if(!m_browser)
		return util::EventReply::Unhandled;
	auto brMousePos = GetBrowserMousePos();
	if(!offsetAsPixels)
		offset *= 50.f;
	else
		offset /= 10.f;
	m_browser->NotifyWheel(offset.x, offset.y, 0.0, servo::capi::WheelMode::Pixel, brMousePos.x, brMousePos.y);
	return util::EventReply::Handled;
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
	if(!m_browser)
		return false;
	if (m_browser->Resize(m_browserViewSize.x, m_browserViewSize.y))
		SetTexture(*m_browser->GetTexture());
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
