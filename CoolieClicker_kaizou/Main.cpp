# include <Siv3D.hpp>

//ゲームのセーブデータ
struct SaveData
{
	double cookies;

	Array<int32> itemCounts;

	// シリアライズに対応させるためのメンバ関数を定義する
	template <class Archive>
	void SIV3D_SERIALIZE(Archive& archive)
	{
		archive(cookies, itemCounts);
	}
};

/// @brief アイテムのボタン
/// @param rect ボタンの領域
/// @param texture ボタンの絵文字
/// @param font 文字描画に使うフォント
/// @param name アイテムの名前
/// @param desc アイテムの説明
/// @param count アイテムの所持数
/// @param enabled ボタンを押せるか
/// @return ボタンが押された場合 true, それ以外の場合は false
bool Button(const Rect& rect, const Texture& texture, const Font& font, const String& name, const String& desc, int32 count, bool enabled)
{
	if (enabled)
	{
		rect.draw(ColorF{ 0.3, 0.5, 0.9, 0.8 });

		rect.drawFrame(2, 2, ColorF{ 0.5, 0.7, 1.0 });

		if (rect.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}
	}
	else
	{
		rect.draw(ColorF{ 0.0, 0.4 });

		rect.drawFrame(2, 2, ColorF{ 0.5 });
	}

	texture.scaled(0.5).drawAt(rect.x + 50, rect.y + 50);

	font(name).draw(30, rect.x + 100, rect.y + 15, Palette::White);

	font(desc).draw(18, rect.x + 102, rect.y + 60, Palette::White);

	font(count).draw(50, Arg::rightCenter((rect.x + rect.w - 20), (rect.y + 50)), Palette::White);

	return (enabled && rect.leftClicked());
}

// クッキーが降るエフェクト
struct CookieBackgroundEffect : IEffect
{
	// 初期座標
	Vec2 m_start;

	// 回転角度
	double m_angle;

	// テクスチャ
	Texture m_texture;

	CookieBackgroundEffect(const Vec2& start, const Texture& texture)
		: m_start{ start }
		, m_angle{ Random(2_pi) }
		, m_texture{ texture } {}

	bool update(double t) override
	{
		const Vec2 pos = m_start + 0.5 * t * t * Vec2{ 0, 120 };

		m_texture.scaled(0.3).rotated(m_angle).drawAt(pos, ColorF{ 1.0, (1.0 - t / 3.0) });

		return (t < 3.0);
	}
};

// クッキーが舞うエフェクト
struct CookieEffect : IEffect
{
	// 初期座標
	Vec2 m_start;

	// 初速
	Vec2 m_velocity;

	// 拡大倍率
	double m_scale;

	// 回転角度
	double m_angle;

	// テクスチャ
	Texture m_texture;

	CookieEffect(const Vec2& start, const Texture& texture)
		: m_start{ start }
		, m_velocity{ Circular{ 80, Random(-40_deg, 40_deg) } }
		, m_scale{ Random(0.5, 0.7) }
		, m_angle{ Random(2_pi) }
		, m_texture{ texture } {}

	bool update(double t) override
	{
		const Vec2 pos = m_start
			+ m_velocity * t + 0.5 * t * t * Vec2{ 0, 120 };

		m_texture.scaled(m_scale).rotated(m_angle).drawAt(pos, ColorF{ 1.0, (1.0 - t) });

		return (t < 1.0);
	}
};

// 「+1」が上昇するエフェクト
struct PlusOneEffect : IEffect
{
	// 初期座標
	Vec2 m_start;

	// フォント
	Font m_font;

	PlusOneEffect(const Vec2& start, const Font& font)
		: m_start{ start }
		, m_font{ font } {}

	bool update(double t) override
	{
		m_font(U"+1").drawAt(24, m_start.movedBy(0, t * -120), ColorF{ 1.0, (1.0 - t) });

		return (t < 1.0);
	}
};

// アイテムのデータ
struct Item
{
	// アイテムの絵文字
	Texture emoji;

	// アイテムの名前
	String name;

	// アイテムを初めて購入するときのコスト
	int32 initialCost;

	// アイテムの CPS
	int32 cps;

	// アイテムを count 個持っているときの購入コストを返す
	int32 getCost(int32 count) const
	{
		return initialCost * (count + 1);
	}
};

// クッキーのばね
class CookieSpring
{
public:

	void update(double deltaTime, bool pressed)
	{
		// ばねの蓄積時間を加算する
		m_accumulatedTime += deltaTime;

		while (0.005 <= m_accumulatedTime)
		{
			// ばねの力（変化を打ち消す方向）
			double force = (-0.02 * m_x);

			// 画面を押しているときに働く力
			if (pressed)
			{
				force += 0.004;
			}

			// 速度に力を適用（減衰もさせる）
			m_velocity = (m_velocity + force) * 0.92;

			// 位置に反映
			m_x += m_velocity;

			m_accumulatedTime -= 0.005;
		}
	}

	double get() const
	{
		return m_x;
	}

private:

	// ばねの伸び
	double m_x = 0.0;

	// ばねの速度
	double m_velocity = 0.0;

	// ばねの蓄積時間
	double m_accumulatedTime = 0.0;
};

// クッキーの後光を描く関数
void DrawHalo(const Vec2& center)
{
	for (int32 i = 0; i < 4; ++i)
	{
		double startAngle = Scene::Time() * 15_deg + i * 90_deg;
		Circle{ center, 180 }.drawPie(startAngle, 60_deg, ColorF{ 1.0, 0.3 }, ColorF{ 1.0, 0.0 });
	}

	for (int32 i = 0; i < 6; ++i)
	{
		double startAngle = Scene::Time() * -15_deg + i * 60_deg;
		Circle{ center, 180 }.drawPie(startAngle, 40_deg, ColorF{ 1.0, 0.3 }, ColorF{ 1.0, 0.0 });
	}
}

// アイテムの所有数をもとに CPS を計算する関数
int32 CalculateCPS(const Array<Item>& ItemTable, const Array<int32>& itemCounts)
{
	int32 cps = 0;

	for (size_t i = 0; i < ItemTable.size(); ++i)
	{
		cps += ItemTable[i].cps * itemCounts[i];
	}

	return cps;
}

void Main()
{
	// クッキーの絵文字
	const Texture texture{ U"💩"_emoji };

	// アイテムのデータ
	const Array<Item> ItemTable = {
		{ Texture{ U"🌾"_emoji }, U"うんち農場", 10, 1 },
		{ Texture{ U"🏭"_emoji }, U"うんち工場", 100, 10 },
		{ Texture{ U"⚓"_emoji }, U"うんち港", 1000, 100 },
	};

	// 各アイテムの所有数
	Array<int32> itemCounts(ItemTable.size()); // = { 0, 0, 0 }

	// フォント
	const Font font{ FontMethod::MSDF, 48, Typeface::Bold };

	// クッキーのクリック円
	constexpr Circle CookieCircle{ 170, 300, 100 };

	// エフェクト
	Effect effectBackground, effect;

	// クッキーのばね
	CookieSpring cookieSpring;

	// クッキーの個数
	double cookies = 0;

	// ゲームの経過時間の蓄積
	double accumulatedTime = 0.0;

	// 背景のクッキーの蓄積時間
	double cookieBackgroundAccumulatedTime = 0.0;

	// セーブデータが見つかればそれを読み込む
	{
		// バイナリファイルをオープン
		Deserializer<BinaryReader> reader{ U"game.save" };

		if (reader) // もしオープンに成功したら
		{
			SaveData saveData;

			reader(saveData);

			cookies = saveData.cookies;

			itemCounts = saveData.itemCounts;
		}
	}

	while (System::Update())
	{
		// クッキーの毎秒の生産量を計算する
		const int32 cps = CalculateCPS(ItemTable, itemCounts);

		// ゲームの経過時間を加算する
		accumulatedTime += Scene::DeltaTime();

		// 0.1 秒以上蓄積していたら
		if (0.1 <= accumulatedTime)
		{
			accumulatedTime -= 0.1;

			// 0.1 秒分のクッキー生産を加算する
			cookies += (cps * 0.1);
		}

		// 背景のクッキー
		{
			// 背景のクッキーが発生する適当な間隔を cps から計算（多くなりすぎないよう緩やかに小さくなり、下限も設ける）
			const double cookieBackgroundSpawnTime = cps ? Max(1.0 / Math::Log2(cps * 2), 0.03) : Math::Inf;

			if (cps)
			{
				cookieBackgroundAccumulatedTime += Scene::DeltaTime();
			}

			while (cookieBackgroundSpawnTime <= cookieBackgroundAccumulatedTime)
			{
				effectBackground.add<CookieBackgroundEffect>(RandomVec2(Rect{ 0, -150, 800, 100 }), texture);

				cookieBackgroundAccumulatedTime -= cookieBackgroundSpawnTime;
			}
		}

		// クッキーのばねを更新する
		cookieSpring.update(Scene::DeltaTime(), CookieCircle.leftPressed());

		// クッキー円上にマウスカーソルがあれば
		if (CookieCircle.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}

		// クッキー円が左クリックされたら
		if (CookieCircle.leftClicked())
		{
			++cookies;

			// クッキーが舞うエフェクトを追加する
			effect.add<CookieEffect>(Cursor::Pos().movedBy(Random(-5, 5), Random(-5, 5)), texture);

			// 「+1」が上昇するエフェクトを追加する
			effect.add<PlusOneEffect>(Cursor::Pos().movedBy(Random(-5, 5), Random(-15, -5)), font);

			// 背景のクッキーを追加する
			effectBackground.add<CookieBackgroundEffect>(RandomVec2(Rect{ 0, -150, 800, 100 }), texture);
		}

		// 背景を描く
		Rect{ 0, 0, 800, 600 }.draw(Arg::top = Palette::White, Arg::bottom = Palette::Darkgoldenrod);

		// 背景で降り注ぐクッキーを描画する
		effectBackground.update();

		// クッキーの後光を描く
		DrawHalo(CookieCircle.center);

		// クッキーの数を整数で表示する
		font(ThousandsSeparate((int32)cookies)).drawAt(60, 170, 100);

		// クッキーの生産量を表示する
		font(U"毎秒: {}"_fmt(cps)).drawAt(24, 170, 160);

		// クッキーを描画する
		texture.scaled(1.5 - cookieSpring.get()).drawAt(CookieCircle.center);

		// エフェクトを描画する
		effect.update();

		for (size_t i = 0; i < ItemTable.size(); ++i)
		{
			// アイテムの所有数
			const int32 itemCount = itemCounts[i];

			// アイテムの現在の価格
			const int32 itemCost = ItemTable[i].getCost(itemCount);

			// アイテム 1 つあたりの CPS
			const int32 itemCps = ItemTable[i].cps;

			// ボタン
			if (Button(Rect{ 340, (40 + 120 * i), 420, 100 }, ItemTable[i].emoji,
				font, ItemTable[i].name, U"C{} / {} CPS"_fmt(itemCost, itemCps), itemCount, (itemCost <= cookies)))
			{
				cookies -= itemCost;
				++itemCounts[i];
			}
		}
	}

	// メインループの後、終了時にゲームをセーブ
	{
		// バイナリファイルをオープン
		Serializer<BinaryWriter> writer{ U"game.save" };

		// シリアライズに対応したデータを書き出す
		writer(SaveData{ cookies, itemCounts });
	}
}
