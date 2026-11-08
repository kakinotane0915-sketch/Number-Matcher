#pragma once
//==============================================================
// Slot.h
//  通常時のスロット(3リール)と、それに付随する
//   ・Number_Management : 停止図柄・リーチの抽選
//   ・Reach_Text        : 「リーチ」文字と雷の演出
//   ・Hit_Effect        : 大当たり演出
//==============================================================
#include <vector>
#include "GameLoader.h"

//==============================================================
// Slot : リールの回転・停止・リーチ・ホワイトアウト
//
//  このスロットは図柄を縦に流すのではなく、
//  「1つの図柄が拡大していき、一定サイズで次の図柄に切り替わる」
//  ことで回転を表現している(Symbol_Scale で管理)
//==============================================================
class Slot
{
public:
	// リールの状態
	enum class STATE
	{
		STOPPED,  // 停止中
		ACCEL,    // 回転開始
		CONSTANT, // 回転中
		DECEL,    // 停止動作中
		RESET,    // 次の回転の準備
		REACH     // リーチ成立(中リールのみ)
	};

	std::vector<int> Symbol_Center_X;               // 各リールの表示位置
	std::vector<int> Symbol_Center_Y;
	std::vector<std::vector<double>> Symbol_Scale;  // [リール][図柄] の拡大率(0なら非表示)
	std::vector<STATE> Reel_State;                  // 各リールの状態
	std::vector<double> Reel_GlobalTime;            // 各リールの経過時間
	std::vector<int> Reel_AlphaValue;               // 各リールの透明度
	std::vector<double> Constant_Count;             // 回転中のカウンタ
	std::vector<double> Decel_Count;                // 停止動作のカウンタ

	std::vector<int> Decel_State;     // 停止動作の段階(0:待機 1:縮小 2:拡大 3:確定)
	std::vector<int> Current_Number;  // 各リールに今表示している図柄

	double Reel_GlobalTime_Increment;  // 経過時間の増加量
	double Constant_Count_Increment;   // 回転カウンタの増加量
	double Decel_Count_Increment;      // 停止カウンタの増加量

	double Reel_Accel_Constant;
	double Symbol_ScaleIncrement;      // 図柄の拡大速度(= 回転の速さ)

	int Symbol_IntervalTime;
	int Accel_inTime;     // 回転開始するフレーム
	int Constant_inTime;  // 等速回転に入るフレーム
	int Decel_inTime;     // 停止動作に入る基準フレーム
	int Reset_inTime;     // リセットするフレーム
	int Reel_Alpha;
	int offset_time_swi;  // 停止動作の各段階の長さ

	// 各リールの停止順(大きいほど遅く止まる)
	// 左 → 右 → 中 の順に止まる
	float order[3] = { 1.5f, 3.5f, 2.5f };

	double Pace_Speed;  // 全体の速さ(保留が多いと早くなる)

	bool is_white;    // ホワイトアウト中か
	int  white_Alpha; // ホワイトアウトの濃さ
	int  TotalSpins;  // 総回転数

	// 前兆演出(画面揺れ)
	int  Previous_EF_Time[PREVIOUS_EF_NUMBER];
	bool is_Previous_EF_Swi[PREVIOUS_EF_NUMBER];

	void Read();
	void Out();

	void Stopped_Out(int i);
	void Accel_Out(int i);
	void Constant_Out(int i);
	void Decel_Out(int i);
	void Reset_Out(int i);
	void Reach_Out(int i);

	void EF1_Out();  // 前兆演出:画面シェイク

	void Draw();
};

extern Slot SLOT;

//==============================================================
// Number_Management : 停止図柄とリーチの種類を抽選する
//==============================================================
class Number_Management
{
public:
	enum class REACH_TYPE
	{
		NONE,              // リーチなし
		NORMAL_MISS,       // 挙動1: 最終リールはランダムの図柄で停止
		CLOSE_MISS,        // 挙動2: 揃う図柄の1つ後ろで停止(惜しいハズレ)
		ACCEL_WHITEOUT,    // 挙動3: 通り過ぎて加速 → ホワイトアウト → バトル演出へ
		LONGREACH_NOWHITE  // 挙動4: ホワイトアウトせず、1つ後ろで停止
	};

	std::vector<int> Number_Hit;  // 各リールの停止図柄(抽選結果)

	bool   is_Reach;  // リーチが成立するか
	double Stock;     // 保留の数

	int Reach_Probability;                 // リーチ発生率(%)
	int Reach_ACCEL_WHITEOUT_Probability;  // ホワイトアウト演出になる率(%)
	int Reach_Win_Probability;             // バトル演出の勝利率(%)

	REACH_TYPE Current_ReachType;  // 今回のリーチの種類

	void Read();
	void Out();  // 次の回転の抽選

private:
	void DecideReach();  // 左右の図柄からリーチの種類を決める
};

extern Number_Management NM;

//==============================================================
// Reach_Text : 「リーチ」の文字と雷のアニメーション
//==============================================================
class Reach_Text
{
public:
	int image[50];              // 「リーチ」文字の連番画像
	int ReachThunder_Image[30]; // 雷の連番画像

	float t;    // 経過時間(= コマ番号)
	bool  swi;  // 表示中か

	void Read();
	void Out();
	void Draw();
};

extern Reach_Text RT;

//==============================================================
// Hit_Effect : 大当たり演出(当たり図柄ごとのアニメーション)
//==============================================================
class Hit_Effect
{
public:
	bool   is_swi;          // 演出中か
	double Time;            // 経過時間(= コマ番号)
	int    HitImage[100];   // 当たり図柄のアニメーション(図柄ごとに読み替える)
	bool   is_HitImageLoaded;
	int    Hit_Back[40];    // 背景のアニメーション(添字1〜39を使用)

	void Read();
	void Out();
	void Draw();
	void Load();  // 当たり図柄に合わせて HitImage を読み込む
};

extern Hit_Effect HE;
