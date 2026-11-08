//==============================================================
// kakuhen.cpp
//  確変モードの処理
//
//  1回転の流れ(各リール共通、Reel_Time で管理)
//   WAITING → STARTING(上へ飛ぶ) → CONSTANTING(画面外)
//   → DECELING(左から順に落下・着地) → FINISHING(3つ着地)
//   → RESETING(残り回数を減らして次の回転へ)
//==============================================================
#include "DxLib.h"
#include "GameLoader.h"
#include "kakuhen.h"
#include "Slot.h"
#include <string>

Kakuhen KAKUHEN;

namespace
{
	constexpr int   LAND_Y = 240;    // 図柄の着地位置
	constexpr int   DROP_START_Y = -240;   // 落下開始位置(画面の上)
	constexpr float START_SPEED_Y = 20.0f;  // 初速
	constexpr float JUMP_SPEED_Y = -25.0f; // 飛び上がる時の初速(マイナス=上)
	constexpr float JUMP_GRAVITY = 15.5f;  // 飛び上がり中の重力
	constexpr float DROP_GRAVITY = 2.5f;   // 落下中の重力
}

void Kakuhen::Read()
{
	pace = 5;
	OffsetTime = 20 / pace;
	RemainingTimes = KAKUHEN_TIMES;
	HittingTimes = 0;
	EndTimes = 0;
	is_Hit = false;
	is_Finish = false;

	for (int i = 0; i < REEL_NUMBER; i++)
	{
		Reel_Time[i] = 0;
		Sta[i] = STATE::WAITING;
		Number_Hit[i] = i;

		X[i] = 120 + (i * 200);
		Y[i] = LAND_Y;
		gY[i] = START_SPEED_Y;
	}
	SetTimers();

	RemainingTimes_BackImage = LoadGraphOrExit("NUM/futuresign.png");
	BONUS_Image = LoadGraphOrExit("BONUS.png");
}

// pace(速さ)に合わせて各状態に入るフレームを決める
void Kakuhen::SetTimers()
{
	for (int i = 0; i < REEL_NUMBER; i++)
	{
		StartingTimer[i] = 100 / pace;
		ConstantingTimer[i] = 300 / pace;
		DecelingTimer[i] = 400 / pace;
	}
}

void Kakuhen::Out()
{
	for (int i = 0; i < REEL_NUMBER; i++)
	{
		Reel_Time[i]++;

		//---------------- 状態の切り替え ----------------
		if (Reel_Time[i] == 2)
		{
			Sta[i] = STATE::WAITING;
		}
		else if (Reel_Time[i] == StartingTimer[i])
		{
			gY[i] = JUMP_SPEED_Y;  // 上に飛ばす
			Sta[i] = STATE::STARTING;
		}
		else if (Reel_Time[i] == ConstantingTimer[i])
		{
			Sta[i] = STATE::CONSTANTING;
		}
		else if (Reel_Time[i] == DecelingTimer[i] + (OffsetTime * i))
		{
			// 左のリールから順番に、少しずつずらして落とす
			Update(i);           // 落とす前に抽選(i==0 の時だけ実行される)
			Y[i] = DROP_START_Y;
			Sta[i] = STATE::DECELING;
		}
		else if (Y[0] == LAND_Y && Y[1] == LAND_Y && Y[2] == LAND_Y &&
			Sta[i] == STATE::DECELING)
		{
			// 3つとも着地した
			Sta[i] = STATE::FINISHING;
		}

		//---------------- 状態ごとの処理 ----------------
		switch (Sta[i])
		{
		case STATE::WAITING:     Waiting(i);     break;
		case STATE::STARTING:    Starting(i);    break;
		case STATE::CONSTANTING: Constanting(i); break;
		case STATE::DECELING:    Deceling(i);    break;
		case STATE::FINISHING:   Finishing(i);   break;
		case STATE::RESETING:    Reseting();     break;
		}

		Draw(i);
	}
}

// 抽選:当たりなら3つ同じ図柄、はずれなら揃わない図柄にする
void Kakuhen::Update(int i)
{
	if (i > 0) return;  // 抽選は1回転に1回だけ(左リールの時)

	is_Finish = false;
	is_Hit = (GetRand(KAKUHEN_HIT_RANGE) == 0);

	if (is_Hit)
	{
		int n = GetRand(SYMBOL_NUMBER - 1);  // 当たり図柄
		for (int k = 0; k < REEL_NUMBER; k++) Number_Hit[k] = n;

		NM.Number_Hit[0] = n;
		HE.Load();  // 着地するまでの間に大当たり演出の画像を先読みしておく
	}
	else
	{
		for (int k = 0; k < REEL_NUMBER; k++) Number_Hit[k] = GetRand(SYMBOL_NUMBER - 1);

		// はずれなのに偶然揃ってしまった場合は右の図柄をずらす
		if (Number_Hit[0] == Number_Hit[1] && Number_Hit[1] == Number_Hit[2])
		{
			Number_Hit[2] = (Number_Hit[2] + 1) % SYMBOL_NUMBER;
		}
	}
}

void Kakuhen::Waiting(int i)
{
	// 何もしない(待機)
}

// 上に飛んでいく(画面外まで)
void Kakuhen::Starting(int i)
{
	if (Y[i] < 1000)
	{
		Y[i] = static_cast<int>(Y[i] + gY[i]);
		gY[i] += JUMP_GRAVITY;
	}
}

void Kakuhen::Constanting(int i)
{
	// 画面外で待機
}

// 上から落ちてきて着地する
void Kakuhen::Deceling(int i)
{
	if (Y[i] < LAND_Y)
	{
		gY[i] += DROP_GRAVITY;
		Y[i] = static_cast<int>(Y[i] + gY[i]);
		if (Y[i] >= LAND_Y)
		{
			// このフレームで着地した
			Y[i] = LAND_Y;
			PlaySoundMem(SOUND.ReachPut, DX_PLAYTYPE_BACK);
		}
	}
}

// 3つとも着地したら「回転終了」にする
void Kakuhen::Finishing(int i)
{
	if (Sta[0] == STATE::FINISHING &&
		Sta[1] == STATE::FINISHING &&
		Sta[2] == STATE::FINISHING)
	{
		is_Finish = true;  // main.cpp 側で当たり判定に使う
		Sta[i] = STATE::RESETING;
	}
}

// 次の回転の準備
void Kakuhen::Reseting()
{
	RemainingTimes--;

	// 次の回転の速さを抽選(1/3でゆっくり)
	pace = (GetRand(2) == 0) ? 1 : 5;
	
	//   1回前の pace のずれ幅になっていた
	OffsetTime = 20 / pace;
	SetTimers();

	is_Finish = false;
	for (int i = 0; i < REEL_NUMBER; i++)
	{
		Reel_Time[i] = 0;
		gY[i] = START_SPEED_Y;
		Sta[i] = STATE::WAITING;
	}
}

void Kakuhen::Draw(int i)
{
	DrawRotaGraph(X[i], Y[i], 0.8f, 0, GL.Symbol_GraphHandle[Number_Hit[i]], true);
}

// 残り回数を左下に表示
void Kakuhen::DrawTimes()
{
	DrawRotaGraph(60, 400, 1.0f, 0, RemainingTimes_BackImage, true);
	TEXT.DrawNumber(20, 400, RemainingTimes, 38);
}

// リザルト画面:ルーレット音を鳴らし、240フレームで止める
void Kakuhen::EndOut()
{
	if (SC.is_change == true) return;  // シャッター演出が終わるまで待つ

	EndTimes++;
	if (EndTimes == 1)   PlaySoundMem(SOUND.Roulette, DX_PLAYTYPE_LOOP);
	if (EndTimes == 240) StopSoundMem(SOUND.Roulette);
}

// リザルト画面:「BONUS ○回」を表示(240フレームまではルーレット風に数字が回る)
void Kakuhen::EndDraw()
{
	if (SC.is_change == true) return;

	DrawRotaGraph(250, 200, 1.0f, 0, BONUS_Image, true);

	if (EndTimes <= 240)
	{
		// ルーレット中:数字をぐるぐる回す
		DrawRotaGraph(500, 200, 1.0f, 0, TEXT.Number[EndTimes % 10], true);
	}
	else
	{
		// 確定:確変中に当たった回数(最初の当たりを除く)
		
		TEXT.DrawNumber(500, 200, HittingTimes - 1, 38);
	}
}
