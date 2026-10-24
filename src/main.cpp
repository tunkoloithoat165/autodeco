#include <Geode/Geode.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <set>
#include <vector>
#include <string>
#include <cstdint>

using namespace geode::prelude;

// ---------------------------------------------------------------------------
// Danh sách style. `ids` là object ID dùng để deco (block / slab / deco nhỏ).
// Các ID này là mặc định an toàn; hãy chỉnh lại theo ý thích trong editor.
// ---------------------------------------------------------------------------
struct DecoStyle {
	const char* name;
	const char* desc;
	ccColor3B base;
	ccColor3B accent;
	std::vector<int> ids;
};

static const std::vector<DecoStyle>& styles() {
	static const std::vector<DecoStyle> s = {
		{"Inferno",      "Cracked black rock glowing from inside.", {40,10,5},    {255,120,20},  {1, 40, 211}},
		{"Ocean",        "Cool blue waves and bubbles.",            {5,30,70},    {60,170,255},  {40, 211}},
		{"Neon City",    "Dark skyline with neon trim.",            {15,5,40},    {255,50,200},  {1, 40}},
		{"Forest",       "Mossy blocks and leafy edges.",           {10,40,15},   {90,200,80},   {40, 211}},
		{"Frost",        "Icy crystals on pale stone.",             {150,200,230},{240,250,255}, {1, 211}},
		{"Desert",       "Warm sand and sunbaked slabs.",           {120,80,30},  {240,200,110}, {40, 1}},
		{"Space",        "Deep space with glowing fragments.",      {5,5,25},     {140,120,255}, {211, 40}},
		{"Candy",        "Sugary bright pinks and blues.",          {255,150,200},{120,220,255}, {1, 40, 211}},
		{"Sakura",       "Soft pink petals, quiet mood.",           {90,40,60},   {255,180,210}, {40, 211}},
		{"Toxic",        "Radioactive green sludge.",               {15,30,10},   {150,255,30},  {1, 211}},
		{"Industrial",   "Metal plates and rivets.",                {50,50,55},   {200,170,60},  {1, 40}},
		{"Cyber",        "Circuit-board lines.",                    {5,20,30},    {0,255,220},   {40, 211}},
		{"Lava Cave",    "Hot cavern with molten edges.",           {35,15,10},   {255,80,10},   {1, 40}},
		{"Sunset",       "Orange to purple dusk.",                  {70,25,70},   {255,150,60},  {40, 211}},
		{"Midnight",     "Quiet dark blue night.",                  {5,10,35},    {90,110,200},  {211, 40}},
		{"Crystal",      "Faceted gem shards.",                     {40,20,80},   {200,150,255}, {1, 211}},
		{"Steampunk",    "Brass pipes and gears.",                  {70,45,25},   {210,160,70},  {1, 40}},
		{"Retro Arcade", "Pixel-style bright blocks.",              {20,20,20},   {255,220,0},   {1, 40, 211}},
		{"Haunted",      "Pale ghostly mist.",                      {30,25,40},   {170,255,200}, {40, 211}},
		{"Jungle",       "Dense vines and warm light.",             {15,50,20},   {255,220,80},  {40, 1}},
		{"Aurora",       "Shifting green and violet glow.",         {5,15,35},    {80,255,170},  {211, 40}},
		{"Volcano",      "Ash grey with red cracks.",               {45,40,40},   {255,60,30},   {1, 40}},
		{"Pastel",       "Light and airy colours.",                 {200,220,240},{255,200,220}, {1, 211}},
		{"Monochrome",   "Black, white and grey only.",             {30,30,30},   {230,230,230}, {1, 40}},
		{"Gold Temple",  "Ancient gold-trimmed stone.",             {60,45,20},   {255,210,80},  {1, 40}},
		{"Storm",        "Dark clouds with lightning accents.",     {25,30,45},   {200,220,255}, {40, 211}},
		{"Deep Sea",     "Abyssal dark with glowing life.",         {0,10,25},    {0,200,200},   {211, 40}},
		{"Clockwork",    "Interlocking mechanical pieces.",         {55,50,45},   {190,190,170}, {1, 40}},
		{"Glitch",       "Broken, offset coloured blocks.",         {10,10,10},   {255,0,80},    {1, 40, 211}},
		{"Dreamy",       "Soft pastel clouds.",                     {120,100,170},{255,230,250}, {40, 211}},
		{"Minimal",      "Clean, sparse trim only.",                {25,25,30},   {160,160,180}, {40}},
	};
	return s;
}

// ---------------------------------------------------------------------------
// "AI" gợi ý: phân tích layout (mật độ hazard, độ dài, độ dày block) rồi chọn style.
// Đây là heuristic chạy cục bộ (không cần mạng / API key).
// ---------------------------------------------------------------------------
struct LayoutStats {
	int blocks = 0;
	int hazards = 0;
	float length = 0.f;
	float maxY = 0.f;
};

static LayoutStats analyze(LevelEditorLayer* lel) {
	LayoutStats st;
	for (auto obj : CCArrayExt<GameObject*>(lel->m_objects)) {
		if (obj->m_objectType == GameObjectType::Solid) st.blocks++;
		else if (obj->m_objectType == GameObjectType::Hazard) st.hazards++;
		st.length = std::max(st.length, obj->getPositionX());
		st.maxY = std::max(st.maxY, obj->getPositionY());
	}
	return st;
}

static int suggestStyle(LevelEditorLayer* lel, int salt) {
	auto st = analyze(lel);
	int total = std::max(1, st.blocks + st.hazards);
	float hazardRatio = static_cast<float>(st.hazards) / total;
	float blockRatio = static_cast<float>(st.blocks) / total;

	std::vector<int> pool;
	if (hazardRatio > 0.35f)       pool = {0, 12, 21, 25, 28};      // dữ dội
	else if (blockRatio > 0.8f)    pool = {10, 16, 26, 27, 23};     // nhiều block, kiểu cơ khí
	else if (st.length > 9000.f)   pool = {6, 20, 14, 11, 2};       // level dài: ban đêm / không gian
	else if (st.maxY > 600.f)      pool = {1, 26, 14, 18, 29};      // level cao
	else                           pool = {7, 8, 22, 30, 3};        // nhẹ nhàng
	return pool[(static_cast<size_t>(salt) + st.blocks + st.hazards) % pool.size()];
}

// ---------------------------------------------------------------------------
// Decorator: đặt object deco lên cạnh trên của các block rắn.
// ---------------------------------------------------------------------------
static int64_t key(int gx, int gy) {
	return (static_cast<int64_t>(gx) << 32) ^ static_cast<uint32_t>(gy);
}

static std::vector<Ref<GameObject>> applyDeco(LevelEditorLayer* lel, DecoStyle const& style, bool undo) {
	std::vector<Ref<GameObject>> placed;
	auto maxObjs = static_cast<size_t>(Mod::get()->getSettingValue<int64_t>("max-objects"));
	float density = static_cast<float>(Mod::get()->getSettingValue<double>("density"));

	std::set<int64_t> solids;
	std::vector<CCPoint> tops;
	for (auto obj : CCArrayExt<GameObject*>(lel->m_objects)) {
		if (obj->m_objectType != GameObjectType::Solid) continue;
		auto p = obj->getPosition();
		solids.insert(key(static_cast<int>(std::floor(p.x / 30.f)), static_cast<int>(std::floor(p.y / 30.f))));
	}
	for (auto obj : CCArrayExt<GameObject*>(lel->m_objects)) {
		if (obj->m_objectType != GameObjectType::Solid) continue;
		auto p = obj->getPosition();
		int gx = static_cast<int>(std::floor(p.x / 30.f));
		int gy = static_cast<int>(std::floor(p.y / 30.f));
		if (!solids.count(key(gx, gy + 1))) tops.push_back(p);
	}

	for (auto const& p : tops) {
		if (placed.size() >= maxObjs) break;
		// giả ngẫu nhiên ổn định theo vị trí để preview và áp dụng cho kết quả giống nhau
		uint32_t h = static_cast<uint32_t>(p.x * 73856093.f) ^ static_cast<uint32_t>(p.y * 19349663.f);
		if ((h % 1000) / 1000.f > density) continue;
		int id = style.ids[h % style.ids.size()];
		auto obj = lel->createObject(id, ccp(p.x, p.y + 30.f), undo);
		if (obj) placed.push_back(obj);
	}
	return placed;
}

// ---------------------------------------------------------------------------
// Popup menu giống ảnh: tiêu đề, preview, nút < 15/31 >, Preview on layout, Decorate level.
// ---------------------------------------------------------------------------
class AutoDecoPopup : public Popup<LevelEditorLayer*> {
protected:
	LevelEditorLayer* m_lel = nullptr;
	int m_index = 0;
	CCLabelBIFont* m_nameLabel = nullptr;
	CCLabelBMFont* m_descLabel = nullptr;
	CCLabelBMFont* m_counter = nullptr;
	CCNode* m_previewBox = nullptr;
	std::vector<Ref<GameObject>> m_preview;
	bool m_committed = false;

	bool setup(LevelEditorLayer* lel) override {
		m_lel = lel;
		this->setTitle("Auto Deco");

		auto sub = CCLabelBMFont::create("Choose a style, then preview it on your layout", "chatFont.fnt");
		sub->setScale(0.7f);
		sub->setOpacity(180);
		m_mainLayer->addChildAtPosition(sub, Anchor::Top, {0, -32});

		// khung preview bên trái
		m_previewBox = CCLayerColor::create({0, 0, 0, 255}, 150.f, 100.f);
		m_previewBox->setPosition(ccp(20, 90));
		m_mainLayer->addChild(m_previewBox);

		// thông tin style bên phải
		m_nameLabel = CCLabelBIFont::create("", "goldFont.fnt");
		m_nameLabel->setScale(0.6f);
		m_nameLabel->setAnchorPoint({0.f, 0.5f});
		m_nameLabel->setPosition(ccp(190, 175));
		m_mainLayer->addChild(m_nameLabel);

		m_descLabel = CCLabelBMFont::create("", "chatFont.fnt");
		m_descLabel->setScale(0.6f);
		m_descLabel->setAnchorPoint({0.f, 1.f});
		m_descLabel->setPosition(ccp(190, 160));
		m_descLabel->setWidth(150.f / 0.6f);
		m_mainLayer->addChild(m_descLabel);

		// < 15/31 >
		auto left = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
		left->setScale(0.5f);
		auto right = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
		right->setScale(0.5f);
		right->setFlipX(true);
		auto leftBtn = CCMenuItemSpriteExtra::create(left, this, menu_selector(AutoDecoPopup::onPrev));
		auto rightBtn = CCMenuItemSpriteExtra::create(right, this, menu_selector(AutoDecoPopup::onNext));
		m_buttonMenu->addChildAtPosition(leftBtn, Anchor::BottomLeft, {200, 118});
		m_buttonMenu->addChildAtPosition(rightBtn, Anchor::BottomLeft, {330, 118});

		m_counter = CCLabelBMFont::create("", "bigFont.fnt");
		m_counter->setScale(0.4f);
		m_mainLayer->addChildAtPosition(m_counter, Anchor::BottomLeft, {265, 118});

		// nút hành động
		auto previewBtn = CCMenuItemSpriteExtra::create(
			ButtonSprite::create("Preview on layout", 140, true, "bigFont.fnt", "GJ_button_04.png", 30.f, 0.5f),
			this, menu_selector(AutoDecoPopup::onPreview));
		m_buttonMenu->addChildAtPosition(previewBtn, Anchor::BottomLeft, {265, 88});

		auto aiBtn = CCMenuItemSpriteExtra::create(
			ButtonSprite::create("AI Suggest", 90, true, "bigFont.fnt", "GJ_button_02.png", 30.f, 0.5f),
			this, menu_selector(AutoDecoPopup::onSuggest));
		m_buttonMenu->addChildAtPosition(aiBtn, Anchor::BottomLeft, {95, 55});

		auto decoBtn = CCMenuItemSpriteExtra::create(
			ButtonSprite::create("Decorate level", 120, true, "bigFont.fnt", "GJ_button_01.png", 30.f, 0.5f),
			this, menu_selector(AutoDecoPopup::onDecorate));
		m_buttonMenu->addChildAtPosition(decoBtn, Anchor::BottomLeft, {265, 55});

		auto undoBtn = CCMenuItemSpriteExtra::create(
			ButtonSprite::create("Clear preview", 100, true, "bigFont.fnt", "GJ_button_06.png", 30.f, 0.5f),
			this, menu_selector(AutoDecoPopup::onClear));
		m_buttonMenu->addChildAtPosition(undoBtn, Anchor::BottomLeft, {95, 88});

		this->refresh();
		return true;
	}

	void refresh() {
		auto const& st = styles()[m_index];
		m_nameLabel->setString(st.name);
		m_descLabel->setString(st.desc);
		m_counter->setString(fmt::format("{}/{}", m_index + 1, styles().size()).c_str());

		// vẽ preview đơn giản bằng màu của style
		m_previewBox->removeAllChildren();
		auto bg = CCLayerColor::create({st.base.r, st.base.g, st.base.b, 255}, 150.f, 100.f);
		m_previewBox->addChild(bg);
		for (int i = 0; i < 5; i++) {
			auto block = CCLayerColor::create({st.accent.r, st.accent.g, st.accent.b, 255}, 20.f, 8.f + 10.f * (i % 3));
			block->setPosition(ccp(10.f + 28.f * i, 0.f));
			m_previewBox->addChild(block);
		}
	}

	void onPrev(CCObject*) {
		m_index = (m_index + static_cast<int>(styles().size()) - 1) % static_cast<int>(styles().size());
		this->refresh();
	}
	void onNext(CCObject*) {
		m_index = (m_index + 1) % static_cast<int>(styles().size());
		this->refresh();
	}
	void onSuggest(CCObject*) {
		static int salt = 0;
		m_index = suggestStyle(m_lel, salt++);
		this->refresh();
		FLAlertLayer::create("AI Suggest",
			fmt::format("Style gợi ý cho layout này: <cy>{}</c>", styles()[m_index].name),
			"OK")->show();
	}
	void clearPreview() {
		for (auto& o : m_preview) {
			if (o) m_lel->removeObject(o, true);
		}
		m_preview.clear();
	}
	void onClear(CCObject*) { this->clearPreview(); }
	void onPreview(CCObject*) {
		this->clearPreview();
		m_preview = applyDeco(m_lel, styles()[m_index], false);
		Notification::create(fmt::format("Preview: {} object", m_preview.size()), NotificationIcon::Success)->show();
	}
	void onDecorate(CCObject*) {
		this->clearPreview();
		auto placed = applyDeco(m_lel, styles()[m_index], true);
		m_committed = true;
		Notification::create(fmt::format("Đã deco {} object", placed.size()), NotificationIcon::Success)->show();
		this->onClose(nullptr);
	}
	void onClose(CCObject* sender) override {
		if (!m_committed) this->clearPreview();
		Popup::onClose(sender);
	}

public:
	static AutoDecoPopup* create(LevelEditorLayer* lel) {
		auto ret = new AutoDecoPopup();
		if (ret && ret->initAnchored(360.f, 230.f, lel)) {
			ret->autorelease();
			return ret;
		}
		CC_SAFE_DELETE(ret);
		return nullptr;
	}
};

// ---------------------------------------------------------------------------
// Thêm nút "Auto Deco" vào menu pause của editor.
// ---------------------------------------------------------------------------
class $modify(AutoDecoPause, EditorPauseLayer) {
	bool init(LevelEditorLayer* lel) {
		if (!EditorPauseLayer::init(lel)) return false;

		auto menu = CCMenu::create();
		auto btn = CCMenuItemSpriteExtra::create(
			ButtonSprite::create("Auto Deco", 90, true, "bigFont.fnt", "GJ_button_01.png", 30.f, 0.6f),
			this, menu_selector(AutoDecoPause::onAutoDeco));
		menu->addChild(btn);
		auto win = CCDirector::get()->getWinSize();
		menu->setPosition(ccp(win.width - 70.f, 30.f));
		this->addChild(menu);
		return true;
	}

	void onAutoDeco(CCObject*) {
		AutoDecoPopup::create(m_editorLayer)->show();
	}
};
