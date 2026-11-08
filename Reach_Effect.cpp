//==============================================================
// Reach_Effect.cpp
//  リーチ発展演出(数字同士のバトル)の処理
//==============================================================
#include "Reach_Effect.h"
#include "Slot.h"
#include "DxLib.h"
#include "GameLoader.h"
#include <format>
#include <string>
#include <algorithm>

Reach_Effect1 Reach_EF1;

// 当たり図柄(NM.Number_Hit[0])をもとに、バトルする2つの図柄を決める
void Reach_Effect1::SetSymbols()
{
	int hit = NM.Number_Hit[0];
	Gh0 = (hit + 1) % SYMBOL_NUMBER;  // はずれ図柄
	Gh1 = hit;                        // 当たり図柄
}

void Reach_Effect1::Read()
{
	SetSymbols();

	// 「押せ」の連番画像を非同期で読み込む
	SetUseASyncLoadFlag(true);
	for (int i = 0; i < 21; i++)
	{
		std::string filepath = std::format("OSE_TEXT/{:04d}.png", i + 1);
		Ose_Image[i] = LoadGraph(filepath.c_str());
	}
	SetUseASyncLoadFlag(false);

	X0 = 400;
	X1 = 200;
	X_increment = 5.0f;

	Y0 = 240;
	Y1 = 240;

	S0 = 0;
	S1 = 0;
	S_increment = 0.010f;

	R0 = 0;
	R1 = 0;

	Sta = 0;
	Time = 0;
	StateTimer = 60;

	Conflict_Count = 0;

	is_white = false;
	white_alpha = 0;

	is_Ose = false;
	Ose_Time = 0;

	is_WinorLose = false;
}

void Reach_Effect1::Out()
{
	Time++;

	//---------------- 状態を順番に進める ----------------
	// StateTimer フレーム経過したら次の状態へ
	for (int i = 1; i <= 10; i++)
	{
		if (Time > StateTimer && Sta == i - 1)
		{
			Sta = i;
			// ぶつかり合いの時だけ効果音を鳴らす
			if (i == STATE::Conflict || i == STATE::ConflictLose)
			{
				PlaySoundMem(SOUND.Reach_EF1_2, DX_PLAYTYPE_BACK);
			}
			else
			{
				StopSoundMem(SOUND.Reach_EF1_2);
			}
			Time = 0;
		}
	}

	//---------------- 状態ごとの処理 ----------------
	switch (Sta)
	{
	case STATE::Expansion:  // 拡大
		S0 += S_increment;
		S1 += S_increment;
		break;

	case STATE::Reduction:  // 縮小(0.5倍まで)
		S0 -= S_increment;
		S1 -= S_increment;
		if (S0 < 0.5f) S0 = 0.5f;
		if (S1 < 0.5f) S1 = 0.5f;
		break;

	case STATE::Stop:  // 初期位置に戻す
		X0 = 400;
		X1 = 200;
		R0 = 0;
		R1 = 0;
		break;

	case STATE::Repulsion:  // 離れる(のけぞる)
		X0 += X_increment;
		X1 -= X_increment;
		R0 = 20;
		R1 = -20;
		break;

	case STATE::Suction:  // 引き寄せられる
		StateTimer = 180;
		X_increment = 10.0f;
		R0 = -20;
		R1 = 20;

		// 2つの図柄が接触したか(図柄の幅 363px × 拡大率)
		if (X0 - X1 <= SYMBOL_WIDTH * S0)
		{
			if (Conflict_Count < 2)
			{
				// 1〜2回目:ぶつかり合いへ
				StateTimer = 60;
				Sta = STATE::Conflict;
			}
			else if (Conflict_Count == 2)
			{
				// 3回目:「押せ」ボタン演出
				StateTimer = 60000;   // ボタンが押されるまで自動では進めない
				is_Ose = true;
				X_increment = 0.010f; // ほぼ止める
				Ose_Time += 0.5f;
				if (Ose_Time == 0.50f)
				{
					WaitTimer(200);   // 表示した瞬間に一瞬止めて溜めを作る(演出上の意図的な停止)
					PlaySoundMem(SOUND.Button_Text, DX_PLAYTYPE_BACK);
				}

				// ENTERキー、または一定時間経過で決着へ
				if (Ose_Time > 600 || CheckHitKey(KEY_INPUT_RETURN))
				{
					is_Ose = false;
					X_increment = 0.0f;
					Ose_Time = 0;
					Sta = STATE::ConflictWin;
					break;
				}
			}
		}

		X0 -= X_increment * 1.5;
		X1 += X_increment * 1.5;
		break;

	case STATE::Conflict:  // 押し合い(1フレームごとに前後に揺らす)
		if (Time % 2 == 0)
		{
			X0 += 20;  // 図柄0を右へ
			X1 -= 20;  // 図柄1を左へ(お互い押し込む)
		}
		else
		{
			X0 -= 20;  // 図柄0を左へ戻す
			X1 += 20;  // 図柄1を右へ戻す(反発する)
		}
		break;

	case STATE::ConflictLose:  // 左に押し込まれる
		if (Time % 6 >= 3)
		{
			X0 += 20;
			X1 += 20;
		}
		else
		{
			X0 -= 30;
			X1 -= 30;
		}
		break;

	case STATE::ConflictWin:
		if (Conflict_Count == 2)
		{
			//---------- 決着 ----------
			is_white = true;
			white_alpha = 255;  // 画面を真っ白にする
			Sta = STATE::Lose;
			Time = 0;
			StateTimer = 400;

			// 勝敗の抽選
			// (GetRand(99) は 0〜99 の100通りなので、ちょうど REACH_WIN_PROBABILITY % になる)
			is_WinorLose = (GetRand(99) < NM.Reach_Win_Probability);
			if (DEMO_FORCE_WIN) is_WinorLose = true;

			// 勝った方の図柄を中央に残し、負けた方を消す
			if (is_WinorLose == true)
			{
				X1 = 320;
				S0 = 0;
			}
			else
			{
				X0 = 320;
				S1 = 0;
			}

			PlaySoundMem(SOUND.Reach_EF1_1, DX_PLAYTYPE_BACK);
			break;
		}

		// 右に押し込む
		if (Time % 6 >= 3)
		{
			X0 += 30;
			X1 += 30;
		}
		else
		{
			X0 -= 20;
			X1 -= 20;
		}
		break;

	case STATE::Result:  // 1セット終了 → もう一度
		Time = 0;
		Conflict_Count++;
		Sta = STATE::Stop;
		break;

	case STATE::Lose:  // 決着後:少し待ってから白を明けていく
		R0 = 0;
		R1 = 0;
		if (Time > 60) white_alpha = std::max<int>(white_alpha - 1, 0);
		break;

	case STATE::Finish:  // 白フェードアウト
		is_white = true;
		white_alpha = std::min<int>(white_alpha + 1, 255);
		if (white_alpha == 250)
		{
			Sta = STATE::Reset;
		}
		break;

	case STATE::Reset:  // 次のリーチのためにパラメータを戻す
		SetSymbols();

		X0 = 400;
		X1 = 200;
		X_increment = 5.0f;
		Y0 = 240;
		Y1 = 240;

		S0 = 0.0f;
		S1 = 0.0f;
		R0 = 0;
		R1 = 0;

		Time = 0;
		StateTimer = 60;
		Conflict_Count = 0;

		is_Ose = false;
		Ose_Time = 0;

		// 白フェードは main.cpp 側でシーンを戻すまで維持する
		Sta = STATE::Finish;
		break;
	}

	
	//   main.cpp の描画と合わせて1フレームに2回描画されていた
}

void Reach_Effect1::Draw()
{
	// 2つの図柄(角度は度 → ラジアンに変換)
	DrawRotaGraph(static_cast<int>(X0), static_cast<int>(Y0), S0, R0 * DX_PI_F / 180.0f, GL.Symbol_GraphHandle[Gh0], TRUE);
	DrawRotaGraph(static_cast<int>(X1), static_cast<int>(Y1), S1, R1 * DX_PI_F / 180.0f, GL.Symbol_GraphHandle[Gh1], TRUE);

	// 「押せ」の表示
	if (!is_Ose) return;
	int frame = std::min<int>(20, static_cast<int>(Ose_Time));
	DrawRotaGraph(640 / 2, 320 / 2, 1.0, 0, Ose_Image[frame], true);
}

void Reach_Effect1::White_Draw()
{
	if (!is_white) return;

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, white_alpha);
	DrawGraph(0, 0, BL.White_Back, 0);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
}
