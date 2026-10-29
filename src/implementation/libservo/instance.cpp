// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.modules.servo;

import :libservo.instance;

static std::atomic_int waker_triggered;
static void wake_callback(void) { std::atomic_store(&waker_triggered, 1); }

static std::unique_ptr<servo::capi::Instance> g_servoInstance {};
std::expected<void, std::string> servo::capi::Instance::Initialize()
{
	if(g_servoInstance)
		return {};
	auto *builder = servo_builder_create();
	if (builder == nullptr)
		return std::unexpected {"Failed to create servo builder."};
	auto *options = servo_options_create();
	if (options != nullptr) {
		servo_options_set_multiprocess(options, false);
		servo_builder_set_options(builder, options);
	}

	servo_builder_set_event_loop_waker(builder, wake_callback);
	auto *servo = servo_builder_build(builder);
	if(!servo) {
		servo_builder_free(builder);
		return std::unexpected {"Failed to build servo builder."};
	}
	servo_setup_logging(servo);

	auto instance = std::unique_ptr<Instance>{new Instance {servo}};
	g_servoInstance = std::move(instance);
	return {};
}
void servo::capi::Instance::Close() { g_servoInstance = nullptr; }
bool servo::capi::Instance::WasWakerTriggered() const { return atomic_load(&waker_triggered); }
void servo::capi::Instance::SpinEventLoop() { servo_spin_event_loop(m_instance); }
servo::capi::Instance *servo::capi::Instance::Get() { return g_servoInstance.get(); }

servo::capi::Instance::Instance(Servo *instance) : m_instance {instance} {}
servo::capi::Instance::~Instance() { servo_free(m_instance); }
