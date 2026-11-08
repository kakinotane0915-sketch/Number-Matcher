#pragma once
//==============================================================
// Reach_Effect.h
//  リーチ発展演出:2つの数字(図柄)同士のバトル
//
//  Gh0 = はずれ図柄(当たり図柄の次の番号)
//  Gh1 = 当たり図柄
//  2つが「拡大→縮小→ぶつかり合い」を3回繰り返し、
//  最後に「押せ」ボタン演出 → 勝敗が決まる
//==============================================================

class Reach_Effect1
{
public:
	// 演出の状態(Out() の中で StateTimer ごとに順番に進む)
	enum STATE
	{
		Expansion,     // 0: 拡大して登場
		Reduction,     // 1: 縮小
		Stop,          // 2: 位置・角度をリセット
		Repulsion,     // 3: お互い離れる(溜め)
		Suction,       // 4: お互い引き寄せられる
		Conflict,      // 5: ぶつかり合い(押し合い)
		ConflictWin,   // 6: 押し勝つ(3回目は決着)
		ConflictLose,  // 7: 押し負ける
		Result,        // 8: 1セット終了 → Stop に戻る
		Lose,          // 9: 決着後、白フェードが明ける
		Finish,        // 10: 白フェードして演出終了
		Reset,         // 11: パラメータを初期状態に戻す
	};

	int Gh0;  // はずれ図柄の番号(Gh = グラフハンドル配列の添字)
	int Gh1;  // 当たり図柄の番号

	double X0, Y0;  // 図柄0の座標
	double X1, Y1;  // 図柄1の座標

	double X_increment;  // 横移動の速さ

	double S0, S1;       // 拡大率
	double S_increment;  // 拡大率の変化量

	int R0, R1;  // 回転角度(度)

	int Sta;         // 現在の状態(STATE)
	int Time;        // 現在の状態に入ってからの経過フレーム
	int StateTimer;  // 次の状態に進むまでのフレーム数

	int Conflict_Count;  // ぶつかり合いの回数(2回目の次で決着)

	bool is_white;    // 白フェード中か
	int  white_alpha; // 白フェードの濃さ(0〜255)

	int    Ose_Image[21];  // 「押せ」の連番画像
	double Ose_Time;       // 「押せ」表示の経過時間
	bool   is_Ose;         // 「押せ」表示中か

	bool is_WinorLose;  // true = 勝ち(大当たり) / false = 負け

	void Read();
	void Out();         // 更新
	void Draw();        // 図柄と「押せ」の描画
	void White_Draw();  // 白フェードの描画

	void SetSymbols();  // NM.Number_Hit[0] から Gh0 / Gh1 を決める
};

extern Reach_Effect1 Reach_EF1;
