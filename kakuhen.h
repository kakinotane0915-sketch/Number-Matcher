#pragma once
//==============================================================
// kakuhen.h
//  確変(高確率)モード
//  3つの図柄が「上に飛んでいく → 上から落ちてくる」を繰り返し、
//  KAKUHEN_TIMES 回転のうちに揃えば再び大当たり
//==============================================================
#include "GameLoader.h"

class Kakuhen
{
public:
	// 1回転の中での各リールの状態
	enum STATE
	{
		WAITING,      // 待機
		STARTING,     // 上へ飛んでいく
		CONSTANTING,  // 画面外で待機(回転中)
		DECELING,     // 上から落ちてくる
		FINISHING,    // 着地完了
		RESETING,     // 次の回転の準備
	};

	int pace;  // 回転の速さ(大きいほど早い。1回転ごとに抽選で変化)

	int Reel_Time[REEL_NUMBER];   // 各リールの経過フレーム
	int Sta[REEL_NUMBER];         // 各リールの状態(STATE)
	int Number_Hit[REEL_NUMBER];  // 各リールの停止図柄

	// 各状態に入るフレーム(pace で割って速さを変える)
	int StartingTimer[REEL_NUMBER];
	int ConstantingTimer[REEL_NUMBER];
	int DecelingTimer[REEL_NUMBER];

	int X[REEL_NUMBER];     // 図柄の表示座標
	int Y[REEL_NUMBER];
	double gY[REEL_NUMBER]; // 縦方向の速度(重力で加速)

	int OffsetTime;  // 左→中→右と順番に落とすためのずらし時間

	int RemainingTimes;  // 確変の残り回数
	int HittingTimes;    // 大当たり回数(最初の当たりも含む)

	int RemainingTimes_BackImage; // 残り回数の背景画像
	int BONUS_Image;              // リザルトの「BONUS」画像

	bool is_Hit;     // この回転が当たりか
	bool is_Finish;  // 3つとも着地したか

	int EndTimes;    // リザルト画面の経過フレーム

	void Read();
	void Out();

	void Update(int i);  // 抽選(当たり/はずれと停止図柄を決める)

	void Waiting(int i);
	void Starting(int i);
	void Constanting(int i);
	void Deceling(int i);
	void Finishing(int i);
	void Reseting();

	void Draw(int i);
	void DrawTimes();  // 残り回数の表示

	void EndOut();     // リザルト画面の更新
	void EndDraw();    // リザルト画面の描画

private:
	void SetTimers();  // pace に合わせて各タイマーを設定する
};

extern Kakuhen KAKUHEN;
