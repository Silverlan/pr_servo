// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.modules.servo:libservo.instance;

export import :libservo.capi;

export namespace servo::capi {
	class Instance {
	public:
		static std::expected<void, std::string> Initialize();
		static void Close();
		static Instance *Get();
		Servo *GetServo() { return m_instance; }
		bool WasWakerTriggered() const;
		void SpinEventLoop();
		~Instance();
	private:
		Instance(Servo *instance);
		Servo *m_instance = nullptr;
	};
}
