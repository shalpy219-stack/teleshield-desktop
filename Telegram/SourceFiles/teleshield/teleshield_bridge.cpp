#include "teleshield/teleshield_bridge.h"

#include "logs.h"
#include "teleshield/sdk/teleshield_core.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#endif // Q_OS_WIN

namespace TeleShield {
namespace {

#ifdef Q_OS_WIN

struct Api final {
	HMODULE module = nullptr;
	decltype(&teleshield_init) init = nullptr;
	decltype(&teleshield_inspect_text) inspectText = nullptr;
	decltype(&teleshield_inspect_peer) inspectPeer = nullptr;
	decltype(&teleshield_is_search_locked) isSearchLocked = nullptr;
	decltype(&teleshield_set_search_lock) setSearchLock = nullptr;
	decltype(&teleshield_version) version = nullptr;
	decltype(&teleshield_shutdown) shutdown = nullptr;
	bool initialized = false;
};

Api &LoadedApi() {
	static auto result = Api();
	return result;
}

template <typename Function>
[[nodiscard]] Function Resolve(HMODULE module, const char *name) {
	return reinterpret_cast<Function>(GetProcAddress(module, name));
}

void Unload(Api &api) {
	if (api.module) {
		FreeLibrary(api.module);
	}
	api = Api();
}

#endif // Q_OS_WIN

} // namespace

bool Bridge::Init(const QString &configDir) {
#ifdef Q_OS_WIN
	auto &api = LoadedApi();
	if (api.initialized) {
		return true;
	} else if (api.module) {
		Unload(api);
	}

	const auto path = QDir(QCoreApplication::applicationDirPath()).filePath(
		u"teleshield_core.dll"_q);
	api.module = LoadLibraryW(reinterpret_cast<LPCWSTR>(path.utf16()));
	if (!api.module) {
		LOG(("TeleShield Warning: could not load DLL from %1 (error %2)"
			).arg(path
			).arg(GetLastError()));
		return false;
	}

	api.init = Resolve<decltype(api.init)>(api.module, "teleshield_init");
	api.inspectText = Resolve<decltype(api.inspectText)>(
		api.module,
		"teleshield_inspect_text");
	api.inspectPeer = Resolve<decltype(api.inspectPeer)>(
		api.module,
		"teleshield_inspect_peer");
	api.isSearchLocked = Resolve<decltype(api.isSearchLocked)>(
		api.module,
		"teleshield_is_search_locked");
	api.setSearchLock = Resolve<decltype(api.setSearchLock)>(
		api.module,
		"teleshield_set_search_lock");
	api.version = Resolve<decltype(api.version)>(
		api.module,
		"teleshield_version");
	api.shutdown = Resolve<decltype(api.shutdown)>(
		api.module,
		"teleshield_shutdown");
	if (!api.init
		|| !api.inspectText
		|| !api.inspectPeer
		|| !api.isSearchLocked
		|| !api.setSearchLock
		|| !api.version
		|| !api.shutdown) {
		LOG(("TeleShield Warning: DLL does not expose the complete C ABI."));
		Unload(api);
		return false;
	}

	const auto config = QDir::toNativeSeparators(configDir).toUtf8();
	const auto result = api.init(config.constData());
	if (result != 0) {
		LOG(("TeleShield Warning: engine initialization failed with code %1."
			).arg(result));
		Unload(api);
		return false;
	}
	api.initialized = true;
	LOG(("TeleShield Info: core %1 initialized."
		).arg(QString::fromUtf8(api.version())));
	return true;
#else // Q_OS_WIN
	Q_UNUSED(configDir);
	return false;
#endif // Q_OS_WIN
}

int Bridge::InspectText(const QString &text) {
#ifdef Q_OS_WIN
	const auto &api = LoadedApi();
	if (!api.initialized || !api.inspectText) {
		return 0;
	}
	const auto utf8 = text.toUtf8();
	return api.inspectText(utf8.constData());
#else // Q_OS_WIN
	Q_UNUSED(text);
	return 0;
#endif // Q_OS_WIN
}

int Bridge::InspectPeer(
		std::int64_t peerId,
		const QString &username,
		const QString &title) {
#ifdef Q_OS_WIN
	const auto &api = LoadedApi();
	if (!api.initialized || !api.inspectPeer) {
		return 0;
	}
	const auto usernameUtf8 = username.toUtf8();
	const auto titleUtf8 = title.toUtf8();
	return api.inspectPeer(
		peerId,
		usernameUtf8.constData(),
		titleUtf8.constData());
#else // Q_OS_WIN
	Q_UNUSED(peerId);
	Q_UNUSED(username);
	Q_UNUSED(title);
	return 0;
#endif // Q_OS_WIN
}

bool Bridge::IsSearchLocked() {
#ifdef Q_OS_WIN
	const auto &api = LoadedApi();
	return api.initialized && api.isSearchLocked
		? api.isSearchLocked()
		: false;
#else // Q_OS_WIN
	return false;
#endif // Q_OS_WIN
}

void Bridge::SetSearchLocked(bool locked) {
#ifdef Q_OS_WIN
	const auto &api = LoadedApi();
	if (api.initialized && api.setSearchLock) {
		const auto result = api.setSearchLock(locked);
		if (result != 0) {
			LOG(("TeleShield Warning: search lock update failed with code %1."
				).arg(result));
		}
	}
#else // Q_OS_WIN
	Q_UNUSED(locked);
#endif // Q_OS_WIN
}

void Bridge::Shutdown() {
#ifdef Q_OS_WIN
	auto &api = LoadedApi();
	if (api.initialized && api.shutdown) {
		api.shutdown();
	}
	Unload(api);
#endif // Q_OS_WIN
}

} // namespace TeleShield
