// SPDX-License-Identifier: GPL-2.0-or-later
#include "visual/visual-engine.hpp"
#include <QGuiApplication>
#include <QPainter>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace oto::visual;
namespace {
void require(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
template<class F> void rejected(F fn) { bool failed = false; try { fn(); } catch (const std::invalid_argument &) { failed = true; } require(failed, "Invalid request accepted"); }
int ink(const QImage &image) {
	int count = 0;
	for (int y = 0; y < image.height(); ++y) for (int x = 0; x < image.width(); ++x) if (image.pixelColor(x,y).alpha()) ++count;
	return count;
}
}
int main(int argc, char **argv)
{
	QGuiApplication app(argc, argv);
	try {
		TextLayoutEngine layout;
		TextNode node; node.box = QRect(10, 10, 100, 50); node.text = QStringLiteral("Very long display name with spaces");
		const auto clipped = layout.prepare(node);
		require(clipped.metrics.overflow && clipped.metrics.lines == 1, "Clip must retain full single line");
		node.overflow = Overflow::Ellipsis;
		const auto ellipsis = layout.prepare(node);
		require(ellipsis.metrics.elided && ellipsis.metrics.width <= 100 && ellipsis.displayedText.endsWith(QChar(0x2026)), "Ellipsis does not fit");
		node.overflow = Overflow::Wrap; node.box.setHeight(500);
		const auto wrapped = layout.prepare(node);
		require(wrapped.metrics.lines > 1 && !wrapped.metrics.overflow, "Wrap failed");
		node.text = QStringLiteral("First\r\nSecond\nThird"); node.box.setWidth(300);
		require(layout.prepare(node).metrics.lines == 3, "Explicit line breaks lost");
		node.text = QStringLiteral("Fit this name"); node.box = QRect(10,10,120,50); node.pixelSize = 40;
		node.minimumPixelSize = 8; node.overflow = Overflow::AutoFit;
		const auto fitted = layout.prepare(node);
		require(fitted.metrics.pixelSize < 40 && fitted.metrics.pixelSize >= 8 && !fitted.metrics.overflow, "Autofit failed");
		node.box.setWidth(1);
		require(layout.prepare(node).metrics.pixelSize == 8 && layout.prepare(node).metrics.overflow, "Minimum font fallback failed");
		node.text.clear(); node.box = QRect(10,10,160,80);
		require(!layout.prepare(node).metrics.overflow, "Empty text overflow");
		node.text = QString::fromUtf8(u8"José e\u0301 日本語 مرحبا 👩‍💻"); node.pixelSize = 24; node.overflow = Overflow::Wrap;
		const auto unicode = layout.prepare(node);
		require(unicode.displayedText == node.text && unicode.metrics.lines > 0, "Unicode text changed");
		VisualEngine engine;
		auto atlas=std::make_shared<ImageData>();atlas->image=QImage(32,32,QImage::Format_RGBA8888_Premultiplied);atlas->image.fill(QColor(20,20,20,240));
		{QPainter paint(&atlas->image);paint.fillRect(QRect(13,13,6,6),Qt::cyan);}
		Scene darkScene{QSize(32,32),{ImageNode{QRect(0,0,32,32),atlas,{},0,QRect(4,4,24,24),100}}};
		const auto darkOriginal=engine.prepare(darkScene);
		for(int opacity:{0,50}) {
			std::get<ImageNode>(darkScene.nodes[0]).darkOpacity=opacity;const auto faded=engine.prepare(darkScene);
			require(faded->image.pixelColor(8,8).alpha()==240*opacity/100 && faded->image.pixelColor(2,2)==darkOriginal->image.pixelColor(2,2) && faded->image.pixelColor(15,15)==QColor(Qt::cyan),"Dark-fill control faded decoration or failed alpha");
			require(engine.prepare(darkScene)==faded && atlas->image.pixelColor(8,8).alpha()==240,"Opacity bypassed cache or mutated shared atlas");
		}
		std::get<ImageNode>(darkScene.nodes[0]).darkOpacity=-1;rejected([&]{engine.prepare(darkScene);});
		std::get<ImageNode>(darkScene.nodes[0]).darkOpacity=101;rejected([&]{engine.prepare(darkScene);});
		std::get<ImageNode>(darkScene.nodes[0]).darkOpacity=100;require(engine.prepare(darkScene)==darkOriginal,"Restoring opacity changed original pixels");engine.clear();
		Scene scene{QSize(200,120), {node}};
		const auto original = engine.prepare(scene);
		require(ink(original->image) > 0 && original->image.pixelColor(0,0).alpha() == 0, "Transparent raster is empty/opaque");
		for (int y = 0; y < 120; ++y) for (int x = 0; x < 200; ++x)
			if (!node.box.contains(x,y)) require(original->image.pixelColor(x,y).alpha() == 0, "Text leaked beyond clip");
		for (int i = 0; i < 100; ++i) require(engine.prepare(scene) == original, "Unchanged scene not shared");
		require(engine.stats().renders == 1 && engine.stats().hits == 100, "Idle/repeated preparation rerasterized");
		std::get<TextNode>(scene.nodes[0]).color = Qt::red;
		require(engine.prepare(scene) != original, "Color did not invalidate");
		std::get<TextNode>(scene.nodes[0]).horizontal = Horizontal::Right;
		require(engine.prepare(scene)->image != original->image, "Changed scene unchanged");
		Scene rectangles{QSize(40,40), {RectangleNode{QRect(5,5,20,20), QColor(255,0,0,128)}, RectangleNode{QRect(10,10,10,10), Qt::blue}}};
		const auto colors = engine.prepare(rectangles);
		require(colors->image.pixelColor(6,6) == QColor(255,0,0,128) && colors->image.pixelColor(12,12) == QColor(Qt::blue), "Premultiplied alpha or order failed");
		VisualEngine lru(2, 1000000);
		const auto a = lru.prepare(scene); const auto b = lru.prepare(rectangles); lru.prepare(scene);
		Scene blank{QSize(4,4), {}}; lru.prepare(blank);
		require(lru.stats().entries == 2 && lru.stats().evictions == 1 && lru.prepare(scene) == a, "LRU failed");
		require(lru.prepare(rectangles) != b && b->image == colors->image, "Evicted handle invalid");
		VisualEngine bounded(32, 200); bounded.prepare(blank); bounded.prepare(scene);
		require(bounded.stats().entries == 1 && bounded.stats().bytes <= 200, "Oversize frame evicted cache");
		engine.clear(); require(engine.stats().entries == 0 && ink(original->image) > 0, "Clear invalidated retained frame");
		TextNode aligned; aligned.box = QRect(0,0,200,100); aligned.text = QStringLiteral("Hello");
		Scene alignment{QSize(200,100), {aligned}};
		const auto left = engine.prepare(alignment);
		std::get<TextNode>(alignment.nodes[0]).horizontal = Horizontal::Right;
		std::get<TextNode>(alignment.nodes[0]).vertical = Vertical::Bottom;
		const auto rightBottom = engine.prepare(alignment);
		require(ink(left->image.copy(0,0,100,50)) > 0 && ink(left->image.copy(100,50,100,50)) == 0 &&
			ink(rightBottom->image.copy(0,0,100,50)) == 0 && ink(rightBottom->image.copy(100,50,100,50)) > 0,
			"Horizontal/vertical alignment failed");
		aligned.overflow = Overflow::AutoFit;
		TextNode stripNode;stripNode.box=QRect(0,0,80,32);stripNode.pixelSize=18;stripNode.text=QString::fromUtf8(u8"José_日本語_مرحبا_Community_Supporter");
		const auto strip=engine.prepareTextStrip(stripNode);
		require(strip->image.width()>80 && strip->image.width()<=2048 && strip->image.height()==32 && !strip->textMetrics.front().elided,"Overflow strip lost full text or exceeded bounds");
		require(ink(strip->image.copy(strip->image.width()-80,0,80,32))>0,"Tail of overflow strip is empty");
		require(engine.prepareTextStrip(stripNode)==strip,"Text strips not cached/shared");
		stripNode.text="DJ";require(engine.prepareTextStrip(stripNode)->image.size()==stripNode.box.size(),"Fitting name expanded unnecessarily");
		stripNode.text=QString(4096,QChar('W'));const auto capped=engine.prepareTextStrip(stripNode);
		require(capped->image.width()<=2048 && capped->textMetrics.front().elided,"Pathological strip escaped pixel budget");
		stripNode.box.moveLeft(1);rejected([&]{engine.prepareTextStrip(stripNode);});
		require(layout.prepare(aligned).metrics.pixelSize == 24, "Autofit enlarged fitting text");
		rejected([&] { engine.prepare(Scene{QSize(0,1), {}}); });
		rejected([&] { engine.prepare(Scene{QSize(2049,1), {}}); });
		rejected([&] { engine.prepare(Scene{QSize(32,32), std::vector<Node>(65, RectangleNode{QRect(0,0,1,1), Qt::white})}); });
		rejected([&] { engine.prepare(Scene{QSize(32,32), {RectangleNode{QRect(0,0,0,1), Qt::white}}}); });
		TextNode excessive; excessive.box = QRect(0,0,32,32); excessive.text = QString(4096, QChar('x'));
		rejected([&] { engine.prepare(Scene{QSize(32,32), std::vector<Node>(5, excessive)}); });
		node.text = QString(4097, QChar('x')); rejected([&] { layout.prepare(node); });
		node.text.clear(); node.minimumPixelSize = 128; rejected([&] { layout.prepare(node); });
		bool wrongThreadRejected = false;
		std::thread other([&] { try { engine.prepare(blank); } catch (const std::logic_error &) { wrongThreadRejected = true; } }); other.join();
		require(wrongThreadRejected, "Wrong thread accepted");
		std::cout << "PASS: Unicode, overflow modes, clipping, transparency, shared frames, limits, LRU and thread contract\n";
		return 0;
	} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
