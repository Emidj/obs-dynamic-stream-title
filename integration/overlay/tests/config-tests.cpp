// SPDX-License-Identifier: GPL-2.0-or-later
#include "adapters/json-config-store.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char *message)
{
	if (!condition) throw std::runtime_error(message);
}
QByteArray read(const QString &path)
{
	QFile file(path);
	require(file.open(QIODevice::ReadOnly), "test read failed");
	return file.readAll();
}
void write(const QString &path, const QByteArray &bytes)
{
	QFile file(path);
	require(file.open(QIODevice::WriteOnly), "test write failed");
	require(file.write(bytes) == bytes.size(), "test write incomplete");
}
}
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	try {
		QTemporaryDir temporary;
		require(temporary.isValid(), "temporary directory unavailable");
		const auto path = temporary.filePath(QString::fromUtf8("configuración/settings.json"));
		oto::JsonConfigStore store(path);
		require(!store.save({true}), "store writes before load");
		require(store.load().status == oto::ConfigStatus::Missing, "missing config not detected");
		require(store.save({false}) && store.save({true}), "atomic save failed");
		oto::JsonConfigStore reloaded(path);
		const auto loaded = reloaded.load();
		require(loaded.status == oto::ConfigStatus::Loaded && loaded.settings.diagnosticsEnabled, "round trip failed");
		for (const auto &invalid : {QByteArray("broken json"), QByteArray("[]"),
			QByteArray("{\"schemaVersion\":1,\"diagnosticsEnabled\":\"true\"}"),
			QByteArray("{\"schemaVersion\":0,\"diagnosticsEnabled\":false}"), QByteArray(65537, 'x')}) {
			write(path, invalid);
			require(store.load().status == oto::ConfigStatus::Invalid, "invalid document accepted");
			require(!store.save({true}) && read(path) == invalid, "invalid document overwritten");
		}
		const QByteArray future("{\"schemaVersion\":2,\"diagnosticsEnabled\":true}");
		write(path, future);
		require(store.load().status == oto::ConfigStatus::UnsupportedVersion, "future version accepted");
		require(!store.save({false}) && read(path) == future, "future config overwritten");
		write(path, "{\"schemaVersion\":1,\"diagnosticsEnabled\":false,\"extra\":42}");
		require(store.load().status == oto::ConfigStatus::Loaded && store.save({true}), "valid config rejected");
		require(QJsonDocument::fromJson(read(path)).object().value("extra").toInt() == 42, "unknown field dropped");
		write(path, future);
		require(!store.save({false}) && read(path) == future, "external edit overwritten");
		const auto blocker = temporary.filePath("blocker");
		write(blocker, "not a directory");
		oto::JsonConfigStore blocked(blocker + "/settings.json");
		blocked.load();
		require(!blocked.save({true}) && read(blocker) == "not a directory", "write failure not handled");
		oto::JsonConfigStore noPath(QString{});
		require(noPath.load().status == oto::ConfigStatus::Unavailable && !noPath.save({}), "empty path accepted");
		std::cout << "PASS: JSON round trip, Unicode paths, malformed/future/oversized config, failed writes, external edits\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
