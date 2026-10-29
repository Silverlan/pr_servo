// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.modules.servo:logging;

export import pragma.shared;

export namespace servo {
	spdlog::logger &get_logger() {
		static spdlog::logger &LOGGER = pragma::register_logger("servo");
		return LOGGER;
	}
}
