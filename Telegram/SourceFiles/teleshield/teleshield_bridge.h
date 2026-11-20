#pragma once

#include <QtCore/QString>

#include <cstdint>

namespace TeleShield {

class Bridge final {
public:
	[[nodiscard]] static bool Init(const QString &configDir);
	[[nodiscard]] static int InspectText(const QString &text);
	[[nodiscard]] static int InspectPeer(
		std::int64_t peerId,
		const QString &username,
		const QString &title);
	[[nodiscard]] static bool IsSearchLocked();
	static void SetSearchLocked(bool locked);
	static void Shutdown();
};

} // namespace TeleShield
