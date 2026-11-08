#pragma once
//==============================================================
// GameLoader.h
//  ゲーム全体で使う定数・シーン定義と、
//  画像/サウンド/背景/入力/ステージチェンジ/数字画像 などの
//  「共通リソース管理クラス」をまとめたヘッダー
//==============================================================

#include <vector>
#include <string>

//--------------------------------------------------------------
// リール・図柄関連の定数
//--------------------------------------------------------------
constexpr int REEL_COUNT = 3;    // リールの数
constexpr int SYMBOL_HEIGHT = 312;  // 図柄画像の高さ(px)
constexpr int SYMBOL_WIDTH = 363;  // 図柄画像の幅(px)
constexpr int REEL_NUMBER = 3;    // リールの数(ループ用)
constexpr int SYMBOL_NUMBER = 9;    // 図柄の種類(1〜9)
constexpr int REEL_MARGIN = 30;
constexpr int REEL_GAP = 50;
constexpr int SYMBOL_GAP = 20;
constexpr int REEL_INITIALPOSITION_X = 0;
constexpr int REEL_INITIALPOSITION_Y = -600;

//--------------------------------------------------------------
// リソース数の定数
//--------------------------------------------------------------
constexpr int BACKGROUND_NUMBER = 2;  // 通常背景の枚数
constexpr int FIELDBGM_NUMBER = 4;  // 通常BGM配列のサイズ(添字1〜3を使用、0は未使用)
constexpr int REACHTYPE_NUMBER = 4;  // リーチの種類数(NONEを除く)
constexpr int REACH_VOICE_NUMBER = 1;  // リーチボイスの数
constexpr int PREVIOUS_EF_NUMBER = 1;  // 前兆演出の数
constexpr int HITTINGBGM_NUMBER = 1;  // 大当たり中BGMの数

//--------------------------------------------------------------
// ゲームバランス / デモ用設定
//  ※提出用の演出デモとして、リーチと勝利が必ず発生する設定にしている。
//    通常の確率で遊ばせたい場合はここの値を変更する。
//--------------------------------------------------------------
constexpr int  REACH_PROBABILITY = 100; // リーチ発生率(%)
constexpr int  REACH_ACCEL_WHITEOUT_PROBABILITY = 100; // リーチ時に「加速→ホワイトアウト」になる率(%)
constexpr int  REACH_WIN_PROBABILITY = 50;  // バトル演出の勝利率(%)
constexpr bool DEMO_FORCE_WIN = true;// true: バトル演出で必ず勝利する(演出確認用)
constexpr int  KAKUHEN_TIMES = 10;  // 確変(高確率)状態の回転数
constexpr int  KAKUHEN_HIT_RANGE = 10;  // 確変中の当選確率 = 1 / (KAKUHEN_HIT_RANGE + 1)
constexpr int  STOCK_MAX = 5;   // 保留の最大数

//--------------------------------------------------------------
// デバッグ設定
//  true にすると画面左上にデバッグ情報を表示し、
//  SPACEキー押下中はスロー再生になる
//--------------------------------------------------------------
constexpr bool DEBUG_MODE = false;

//--------------------------------------------------------------
// ゲームシーン
//  main.cpp のメインループで、この値によって処理を切り替える
//--------------------------------------------------------------
enum class GameScene
{
	NORMAL,       // 通常の回転中(リール上でのリーチも含む)
	REACH,        // リーチ発展演出中(数字同士のバトル)
	STAGECHANGE,  // ステージ(背景・BGM)切り替え中
	HIT,          // 大当たり演出中
	KAKUHEN,      // 確変中
	KAKUHEN_END,  // 確変終了(リザルト画面)
};

//--------------------------------------------------------------
// 画像読み込み(失敗したらエラー表示して終了する)
//--------------------------------------------------------------
int LoadGraphOrExit(const std::string& path);

//==============================================================
// GameLoader : 図柄画像と現在のシーンを管理
//==============================================================
class GameLoader
{
public:
	GameScene CurrentMode;                // 現在のシーン(他クラスから参照する用)
	std::vector<int> Symbol_GraphHandle;  // 図柄 1〜9 の画像ハンドル(添字 0〜8)

	void Read();  // 初期化・画像読み込み
};

extern GameLoader GL;

//==============================================================
// BackGround : 背景の描画と、左右にゆらゆら揺らす処理
//==============================================================
class BackGround
{
public:
	std::vector<int> BackGroundHandle;  // 通常背景の画像ハンドル
	int BackGroundSelect;  // 現在描画している背景
	int CurrentBack;       // 現在の背景番号
	int FieldBackRND;      // 次に切り替える背景番号(抽選結果)
	int White_Back;        // ホワイトアウト用の白画像
	int ResultBack;        // 確変終了時(リザルト)の背景

	// 揺れ(サイン波)用パラメータ
	float time;   // 経過フレーム
	float speed;  // 揺れの速さ
	int a;        // 揺れ幅(振幅)

	int x;
	int y;
	int add_x;    // 画面シェイクなど、外部から加えるズレ量
	int add_y;

	void Read();
	void Out();   // 更新
	void Draw();  // 描画
};

extern BackGround BL;

//==============================================================
// Sound : 効果音・BGMのハンドルをまとめて管理
//==============================================================
class Sound
{
public:
	void Read();

	int Field[FIELDBGM_NUMBER];          // 通常BGM(添字1〜を使用)
	int HittingBGM[HITTINGBGM_NUMBER];   // 大当たり・確変中BGM

	int Reelfinish;      // リール停止音
	int Reachstart;      // リーチ中BGM
	int ReachThunder;    // リーチ文字の雷
	int whiteout;        // ホワイトアウト
	int ReachPut;        // 和太鼓(図柄着地・リーチ文字)
	int ReachBlow;       // リーチ文字の斬撃音
	int Reach_Voice[REACH_VOICE_NUMBER];
	int Reach_EF1_1;     // バトル決着の斬撃
	int Reach_EF1_2;     // バトルのぶつかり合い
	int Button_Text;     // 「押せ」表示
	int Pre_EF1;         // 前兆演出(画面揺れ)
	int ShutterClose;    // ステージチェンジ(シャッター閉)
	int ShutterOpen;     // ステージチェンジ(シャッター開)
	int LandSymbol;      // 図柄着地
	int Roulette;        // リザルトのルーレット
	int Hit1;            // 大当たり

	int FieldBGMSelect;    // 現在の通常BGMハンドル
	int HittingBGMSelect;  // 大当たりBGMの番号(HittingBGMの添字)
	int CurrentFieldBGM;   // 現在の通常BGM番号
	int FieldBGMRND;       // 次に切り替える通常BGM番号(抽選結果)
};

extern Sound SOUND;

//==============================================================
// Button : マウス右クリックの「押した瞬間」を判定
//==============================================================
class Button
{
public:
	bool is_RMOUSE;      // 右クリックが押された瞬間のフレームだけ true
	int  RMOUSE_Counter; // 右クリックを押し続けているフレーム数

	void Read();
	void Out();
};

extern Button BT;

//==============================================================
// StageChange : シャッター演出で背景とBGMを切り替える
//==============================================================
class StageChange
{
public:
	bool   is_change;          // ステージチェンジ演出中なら true
	int    Count;              // ステージチェンジまでの回転数カウント
	double Time;               // 演出の経過時間(= 再生中のコマ番号)
	int    Image_Shutter[130]; // シャッターの連番画像

	void Read();
	void Out();
	void Draw();
};

extern StageChange SC;

//==============================================================
// Text : 数字(0〜9)の画像
//==============================================================
class Text
{
public:
	int Number[10];  // 0〜9の数字画像
	void Read();

	// 数値を数字画像で描画する(x,y は先頭の桁の中心、spacing は桁の間隔)
	void DrawNumber(int x, int y, int value, int spacing) const;
};

extern Text TEXT;
