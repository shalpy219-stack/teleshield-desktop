#include "teleshield/teleshield_studio.h"

#include "base/qt/qt_common_adapters.h"
#include "base/unique_qptr.h"
#include "logs.h"
#include "styles/style_iv.h"
#include "ui/rp_widget.h"
#include "ui/widgets/separate_panel.h"
#include "webview/webview_embed.h"
#include "webview/webview_interface.h"

#include <crl/crl_on_main.h>
#include <QtCore/QPointer>
#include <QtCore/QUrl>

#include <memory>

namespace TeleShield {
namespace {

class StudioWindow final {
public:
	explicit StudioWindow(const QString &url);

	[[nodiscard]] bool valid() const;
	void show(const QString &url);

private:
	std::unique_ptr<Ui::SeparatePanel> _panel;
	std::unique_ptr<Webview::Window> _webview;
};

std::unique_ptr<StudioWindow> &Instance() {
	static auto result = std::unique_ptr<StudioWindow>();
	return result;
}

[[nodiscard]] QString StudioUrl(const QString &route) {
	auto base = QUrl(qEnvironmentVariable(
		"TELESHIELD_STUDIO_URL",
		u"https://studio.teleshield.app"_q));
	if (!base.isValid()
		|| (base.scheme() != u"https"_q && base.scheme() != u"http"_q)) {
		base = QUrl(u"https://studio.teleshield.app"_q);
	}
	const auto normalized = route.startsWith('/') ? route : (u"/"_q + route);
	return base.resolved(QUrl(normalized)).toString();
}

void ReleaseOnMain() {
	crl::on_main([] {
		Instance().reset();
	});
}

StudioWindow::StudioWindow(const QString &url)
: _panel(std::make_unique<Ui::SeparatePanel>()) {
	_panel->setTitle(rpl::single(u"TeleShield Store & Studio"_q));
	_panel->setInnerSize(st::ivEditorWindowDefaultSize, true);

	auto container = base::make_unique_q<Ui::RpWidget>(_panel.get());
	const auto rawContainer = container.get();
	_panel->showInner(std::move(container));

	_webview = std::make_unique<Webview::Window>(
		rawContainer,
		Webview::WindowConfig{
			.safe = true,
			.allowThirdPartyCookies = false,
		});
	const auto widget = QPointer<QWidget>(_webview->widget());
	if (!widget) {
		LOG(("TeleShield Warning: Studio WebView could not be created."));
		return;
	}

	rawContainer->sizeValue(
	) | rpl::on_next([=](QSize size) {
		if (widget) {
			widget->resize(size);
		}
	}, rawContainer->lifetime());

	_webview->setNavigationPolicyHandler([](const QString &target, bool) {
		const auto url = QUrl(target);
		return url.scheme() == u"https"_q || url.scheme() == u"http"_q;
	});
	_panel->closeRequests(
	) | rpl::on_next([] {
		ReleaseOnMain();
	}, _panel->lifetime());
	_panel->closeEvents(
	) | rpl::on_next([] {
		ReleaseOnMain();
	}, _panel->lifetime());

	_webview->navigate(url);
	_panel->showAndActivate();
}

bool StudioWindow::valid() const {
	return _panel && _webview && _webview->valid() && _webview->widget();
}

void StudioWindow::show(const QString &url) {
	_webview->navigate(url);
	_panel->showAndActivate();
	_webview->focus();
}

} // namespace

void OpenStudioWindow(const QString &route) {
	const auto url = StudioUrl(route);
	auto &instance = Instance();
	if (instance && instance->valid()) {
		instance->show(url);
		return;
	}
	instance = std::make_unique<StudioWindow>(url);
	if (!instance->valid()) {
		instance.reset();
	}
}

void CloseStudioWindow() {
	Instance().reset();
}

} // namespace TeleShield
