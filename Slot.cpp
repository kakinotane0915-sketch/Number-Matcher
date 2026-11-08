//==============================================================
// Slot.cpp
//  通常時のスロット処理
//
//  1回転の流れ(リールごとに Reel_GlobalTime で管理)
//   STOPPED → ACCEL → CONSTANT(回転) → DECEL(停止)
//   → RESET(次の抽選)
//  左右の図柄が揃うと中リールが REACH になり、リーチ演出が入る
//==============================================================
#include <vector>
#include <algorithm>
#include <format>
#include <string>
#include "DxLib.h"
#include "Slot.h"
#include "GameLoader.h"
#include "Reach_Effect.h"

Slot SLOT;
Number_Management NM;
Reach_Text RT;
Hit_Effect HE;

//==============================================================
// Slot
//==============================================================
void Slot::Read()
{
	TotalSpins = 0;

	Pace_Speed = 1.0f;
	for (int i = 0; i < REEL_NUMBER; i++) order[i] = order[i] / static_cast<float>(Pace_Speed);

	Reel_GlobalTime_Increment = 1.0f * Pace_Speed;
	Constant_Count_Increment = 1.0f * Pace_Speed;
	Decel_Count_Increment = 1.0f * Pace_Speed;

	Symbol_IntervalTime = 20;
	Symbol_ScaleIncrement = 0.02f;
	Reel_Accel_Constant = 0.0010f;

	Symbol_Center_X.resize(REEL_NUMBER);
	Symbol_Center_Y.resize(REEL_NUMBER);
	Symbol_Scale.resize(REEL_NUMBER);
	Reel_State.resize(REEL_NUMBER);
	Reel_GlobalTime.resize(REEL_NUMBER);
	Reel_AlphaValue.resize(REEL_NUMBER);
	Constant_Count.resize(REEL_NUMBER);
	Decel_Count.resize(REEL_NUMBER);
	Decel_State.resize(REEL_NUMBER);
	Current_Number.resize(REEL_NUMBER);

	// リールの配置と初期図柄
	for (int i = 0; i < REEL_NUMBER; i++)
	{
		Symbol_Center_X[i] = 120 + (i * 200);
		Symbol_Center_Y[i] = 240;
		Reel_AlphaValue[i] = 255;
		Symbol_Scale[i].resize(SYMBOL_NUMBER);
		Reel_GlobalTime[i] = 0;
		Reel_State[i] = STATE::STOPPED;
		Constant_Count[i] = 0;
		Decel_Count[i] = 0;
		Decel_State[i] = 0;

		Current_Number[i] = GetRand(SYMBOL_NUMBER - 1);

		// 今の図柄だけ表示(拡大率0.4)、他は非表示(0)
		for (int j = 0; j < SYMBOL_NUMBER; j++)
		{
			Symbol_Scale[i][j] = (Current_Number[i] == j) ? 0.40f : 0;
		}
	}

	Accel_inTime = 20;
	Constant_inTime = 120;
	Decel_inTime = 300;
	offset_time_swi = 10;

	// リセットのタイミング
	// = (最後のリールが停止動作に入るタイミング) + (停止動作が完了するまでの時間) + 余韻
	Reset_inTime = static_cast<int>((Decel_inTime + 100 * order[1]) + (1 + offset_time_swi * 3) + 50);

	Reel_Alpha = 5;
	is_white = false;
	white_Alpha = 0;

	for (int i = 0; i < PREVIOUS_EF_NUMBER; i++)
	{
		Previous_EF_Time[i] = 0;
		is_Previous_EF_Swi[i] = false;
	}
}

// 前兆演出:効果音を鳴らして背景を左右に揺らす
void Slot::EF1_Out()
{
	if (Previous_EF_Time[0] == 0) PlaySoundMem(SOUND.Pre_EF1, DX_PLAYTYPE_BACK);

	Previous_EF_Time[0]++;

	BL.add_x = (Previous_EF_Time[0] % 6 <= 3) ? 6 : -6;
}

void Slot::Out()
{
	//---------------- 前兆演出 ----------------
	for (int i = 0; i < PREVIOUS_EF_NUMBER; i++)
	{
		if (!is_Previous_EF_Swi[i]) continue;

		switch (i)
		{
		case 0:
			EF1_Out();
			if (Previous_EF_Time[0] > 300)
			{
				// 300フレームで終了
				Previous_EF_Time[0] = 0;
				is_Previous_EF_Swi[0] = false;
				BL.add_x = 0;  // 揺れを元に戻す
				StopSoundMem(SOUND.Pre_EF1);
			}
			break;
		}
	}

	if (Reel_GlobalTime[0] == 0) TotalSpins++;

	//---------------- デバッグ表示 ----------------
	if (DEBUG_MODE)
	{
		clsDx();
		printfDx("TotalSpins=%d\n", TotalSpins);
		printfDx("Stock=%d\n", static_cast<int>(NM.Stock));
		printfDx("Number_Hit=%d %d %d\n", NM.Number_Hit[0] + 1, NM.Number_Hit[1] + 1, NM.Number_Hit[2] + 1);
		printfDx("ReachType=%d\n", static_cast<int>(NM.Current_ReachType));
	}

	//---------------- 各リールの状態遷移 ----------------
	for (int i = 0; i < REEL_NUMBER; i++)
	{
		Reel_GlobalTime[i] += Pace_Speed;

		if (Reel_GlobalTime[i] > Reset_inTime + order[1] + 100 &&
			Reel_State[1] != STATE::CONSTANT)
		{
			// 全リール停止後 → リセット
			Reel_State[i] = STATE::RESET;
		}
		else if (Reel_GlobalTime[i] > Decel_inTime + 100 * order[1] &&
			NM.is_Reach == true &&
			i == 1 &&
			order[1] < 4.0f &&
			Reel_State[i] == STATE::CONSTANT)
		{
			// 中リールが止まるタイミングでリーチ成立 → リーチ演出へ
			Reel_State[i] = STATE::REACH;
		}
		else if (Reel_GlobalTime[i] > Decel_inTime + 100 * order[i] &&
			Current_Number[i] == NM.Number_Hit[i] &&
			order[1] < 20.0f)
		{
			// 停止タイミングを過ぎ、停止図柄が来たら止める
			Reel_State[i] = STATE::DECEL;
		}
		else if (Reel_GlobalTime[i] > Constant_inTime &&
			Reel_GlobalTime[i] <= Decel_inTime + 100 * order[i] &&
			Reel_State[i] == STATE::ACCEL)
		{
			// 等速回転に入る(左リールのタイミングでBGM再生)
			if (i == 0) PlaySoundMem(SOUND.FieldBGMSelect, DX_PLAYTYPE_LOOP, FALSE);
			Reel_State[i] = STATE::CONSTANT;
		}
		else if (Reel_GlobalTime[i] > Accel_inTime &&
			Reel_GlobalTime[i] <= Constant_inTime &&
			Reel_State[i] == STATE::STOPPED)
		{
			// 回転開始
			Reel_State[i] = STATE::ACCEL;
		}

		//---------------- 状態ごとの処理 ----------------
		switch (Reel_State[i])
		{
		case STATE::STOPPED:  Stopped_Out(i);  break;
		case STATE::ACCEL:    Accel_Out(i);    break;
		case STATE::CONSTANT: Constant_Out(i); break;
		case STATE::DECEL:    Decel_Out(i);    break;
		case STATE::RESET:    Reset_Out(i);    break;
		case STATE::REACH:    Reach_Out(i);    break;
		}
	}
}

void Slot::Stopped_Out(int i)
{
	// 停止中は何もしない
}

void Slot::Accel_Out(int i)
{
	// 加速演出を入れる場合はここに追加する(現在は何もしない)
}

// 回転中
void Slot::Constant_Out(int i)
{
	//---------------- 長いリーチの演出 ----------------
	if (NM.Current_ReachType == Number_Management::REACH_TYPE::ACCEL_WHITEOUT)
	{
		// 中リールがリーチ時間を過ぎたあたりから、ゆっくり回してくっきり見せる
		if (Reel_GlobalTime[1] > Decel_inTime + 100 * 4.0f &&
			Reel_GlobalTime[1] < Decel_inTime + 100 * 6.0f)
		{
			Reel_Alpha = 255;
			Constant_Count_Increment = 0.0020f * Pace_Speed;
			Symbol_ScaleIncrement = 0.0020f * Pace_Speed;
		}

		// ① 当たり図柄を通り過ぎた瞬間に「ホワイトアウト開始フラグ」を立てる
		if (Reel_GlobalTime[1] > Decel_inTime + 100 * 6.0f &&
			Current_Number[1] == (NM.Number_Hit[0] + 1) % SYMBOL_NUMBER)
		{
			is_white = true;
		}

		// ② フラグが立っていたら、図柄に関係なく毎フレーム白くしていく
		if (is_white == true)
		{
			if (white_Alpha == 0) PlaySoundMem(SOUND.whiteout, DX_PLAYTYPE_BACK); // 白くなり始めに1回だけ

			white_Alpha = std::min<int>(white_Alpha + 2, 255);

			Constant_Count_Increment += 0.01f;  // 図柄を通り過ぎたところで一気に加速
			Symbol_ScaleIncrement = 0.05f;
			StopSoundMem(SOUND.FieldBGMSelect);
		}
		// white_Alpha が255に近づくと main.cpp でリーチ演出シーンへ移る
	}
	else if (NM.Current_ReachType == Number_Management::REACH_TYPE::LONGREACH_NOWHITE)
	{
		if (Reel_GlobalTime[1] > Decel_inTime + 100 * 4.0f &&
			Reel_GlobalTime[1] < Decel_inTime + 100 * 6.0f)
		{
			Reel_Alpha = 255;
			Constant_Count_Increment = 0.0020f * Pace_Speed;
			Symbol_ScaleIncrement = 0.0020f * Pace_Speed;
		}

		// 当たり図柄の1つ後ろで止める(惜しいハズレ)
		if (Reel_GlobalTime[1] > Decel_inTime + 100 * 6.0f &&
			Current_Number[1] == (NM.Number_Hit[0] + 1) % SYMBOL_NUMBER)
		{
			StopSoundMem(SOUND.FieldBGMSelect);
			order[1] = 4.0f;
			Reel_State[1] = STATE::DECEL;
			Reset_inTime = static_cast<int>(Reel_GlobalTime[1] + (1 + offset_time_swi * 3) + 50);
			NM.Number_Hit[1] = Current_Number[1];
		}
	}

	//---------------- 回転カウンタ ----------------
	Constant_Count[i] += Constant_Count_Increment;
	if (Constant_Count[i] > SYMBOL_NUMBER * Symbol_IntervalTime)
	{
		Constant_Count[i] = 0;
	}

	//---------------- 透明度 ----------------
	// 右リールが止まるまでは回転中のリールを薄くしていく
	if (Decel_State[2] < 1)
	{
		Reel_AlphaValue[i] = std::max<int>(0, Reel_AlphaValue[i] - 10);
	}
	else
	{
		Reel_AlphaValue[i] = Reel_Alpha;
	}

	//---------------- 図柄の切り替え(回転の表現) ----------------
	// 今の図柄を少しずつ拡大し、0.65倍になったら次の図柄(0.5倍)に切り替える
	for (int j = 0; j < SYMBOL_NUMBER; j++)
	{
		if (Symbol_Scale[i][j] >= 0.65)
		{
			int next = (j + 1) % SYMBOL_NUMBER;  // 9の次は1に戻る
			Symbol_Scale[i][next] = 0.5f;
			Symbol_Scale[i][j] = 0;
			Current_Number[i] = next;
		}

		if (Current_Number[i] == j && Symbol_Scale[i][j] < 0.65)
		{
			Symbol_Scale[i][j] += Symbol_ScaleIncrement;
		}
	}
}

// 停止動作
//  段階0: 待機 → 段階1: 縮小 → 段階2: 少し拡大 → 段階3: 確定
void Slot::Decel_Out(int i)
{
	const double prevCount = Decel_Count[i];
	Decel_Count[i] += Decel_Count_Increment;

	// 停止音(確定する瞬間に1回だけ)
	
	const double finishTime = 10 + offset_time_swi * 3;
	if (prevCount < finishTime && Decel_Count[i] >= finishTime)
	{
		PlaySoundMem(SOUND.Reelfinish, DX_PLAYTYPE_BACK);
	}

	// 経過時間から段階を決める
	for (int swi = 0; swi < 4; swi++)
	{
		if (Decel_Count[i] > 10 + offset_time_swi * swi)
		{
			Decel_State[i] = swi;
		}
	}

	for (int j = 0; j < SYMBOL_NUMBER; j++)
	{
		// 停止図柄以外は隠す
		if (Decel_Count[i] > 1 && NM.Number_Hit[i] != j)
		{
			Symbol_Scale[i][j] = 0;
		}

		// 停止図柄だけ動かす
		if (NM.Number_Hit[i] == j)
		{
			switch (Decel_State[i])
			{
			case 0:  // 待機
				Reel_AlphaValue[i] = 5;
				break;
			case 1:  // 縮小
				Symbol_Scale[i][j] = std::max<double>(Symbol_Scale[i][j] - 0.05f * Pace_Speed, 0.3);
				Reel_AlphaValue[i] = std::min<int>(Reel_AlphaValue[i] + 15, 255);
				break;
			case 2:  // 少し拡大
				Symbol_Scale[i][j] = std::min<double>(Symbol_Scale[i][j] + 0.01f * Pace_Speed, 0.4);
				Reel_AlphaValue[i] = std::min<int>(Reel_AlphaValue[i] + 15, 255);
				break;
			case 3:  // 確定
				Symbol_Scale[i][j] = 0.4f;
				Reel_AlphaValue[i] = 255;
				if (i == 1) StopSoundMem(SOUND.FieldBGMSelect);
				break;
			}
		}
	}
}

// 次の回転の準備
void Slot::Reset_Out(int i)
{
	Reel_GlobalTime_Increment = 1.0f * Pace_Speed;
	Constant_Count_Increment = 1.0f * Pace_Speed;
	Symbol_ScaleIncrement = 0.02f * Pace_Speed;

	Reel_GlobalTime[i] = 0;
	Reel_State[i] = STATE::STOPPED;
	Decel_State[i] = 0;
	Reel_AlphaValue[i] = 255;
	Constant_Count[i] = Symbol_IntervalTime * (NM.Number_Hit[i] + 1);
	Decel_Count[i] = 0;

	// 止まっている図柄だけ表示
	for (int j = 0; j < SYMBOL_NUMBER; j++)
	{
		Symbol_Scale[i][j] = (Current_Number[i] == j) ? 0.40f : 0;
	}

	// 停止順を通常に戻す(リーチで変更されている場合があるため)
	order[0] = 0.0f;
	order[1] = 2.5f;
	order[2] = 1.0f;

	Reset_inTime = static_cast<int>((Decel_inTime + 100 * order[1]) + (1 + offset_time_swi * 3) + 50);

	if (i == 1)
	{
		StopSoundMem(SOUND.Reachstart);

		// 回転数に応じてランダムでステージチェンジ
		SC.Count = std::min<int>(SC.Count + 1, 10);
		if (SC.Count == GetRand(10))
		{
			SC.is_change = true;
			SC.Count = 0;
		}

		NM.Out();  // 次の回転の抽選
	}

	RT.swi = false;
}

// リーチ成立(中リール)
void Slot::Reach_Out(int i)
{
	if (i == 1)
	{
		StopSoundMem(SOUND.FieldBGMSelect);
		PlaySoundMem(SOUND.Reachstart, DX_PLAYTYPE_BACK);
		PlaySoundMem(SOUND.Reach_Voice[0], DX_PLAYTYPE_BACK);

		RT.swi = true;  // 「リーチ」文字の演出開始
	}

	// リーチ中は中リールの停止を遅らせる
	if (NM.Current_ReachType != Number_Management::REACH_TYPE::ACCEL_WHITEOUT) order[1] = 4.0f;

	// リーチの種類ごとの設定
	switch (NM.Current_ReachType)
	{
	case Number_Management::REACH_TYPE::NORMAL_MISS:
		break;
	case Number_Management::REACH_TYPE::CLOSE_MISS:
		NM.Number_Hit[1] = (NM.Number_Hit[0] + 1) % SYMBOL_NUMBER;  // 1つ後ろで止める
		break;
	case Number_Management::REACH_TYPE::ACCEL_WHITEOUT:
	case Number_Management::REACH_TYPE::LONGREACH_NOWHITE:
		order[1] = 20.0f;  // 通常の停止をさせず、Constant_Out 側の演出に任せる
		break;
	default:
		break;
	}

	order[1] = order[1] / static_cast<float>(Pace_Speed);
	Reset_inTime = static_cast<int>((Decel_inTime + 100 * order[1]) + (1 + offset_time_swi * 3) + 50);
	Reel_State[i] = STATE::CONSTANT;  // 回転を続ける
}

void Slot::Draw()
{
	// 各リールの図柄(拡大率0のものは見えない)
	for (int i = 0; i < REEL_NUMBER; i++)
	{
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, Reel_AlphaValue[i]);
		for (int j = 0; j < SYMBOL_NUMBER; j++)
		{
			DrawRotaGraph(Symbol_Center_X[i], Symbol_Center_Y[i], Symbol_Scale[i][j], 0,
				GL.Symbol_GraphHandle[j], true);
		}
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
	}

	// ホワイトアウト
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, white_Alpha);
	DrawGraph(0, 0, BL.White_Back, 0);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
}

//==============================================================
// Number_Management
//==============================================================
void Number_Management::Read()
{
	Number_Hit.resize(REEL_NUMBER);

	Reach_Probability = REACH_PROBABILITY;
	Reach_ACCEL_WHITEOUT_Probability = REACH_ACCEL_WHITEOUT_PROBABILITY;
	Reach_Win_Probability = REACH_WIN_PROBABILITY;

	for (int i = 0; i < REEL_NUMBER; i++)
	{
		Number_Hit[i] = GetRand(SYMBOL_NUMBER - 1);
	}

	// 初回の回転は抽選結果そのままでリーチ判定する
	if (Number_Hit[0] == Number_Hit[2])
	{
		is_Reach = true;
		Current_ReachType = static_cast<REACH_TYPE>(GetRand(REACHTYPE_NUMBER - 1) + 1);
	}
	else
	{
		is_Reach = false;
		Current_ReachType = REACH_TYPE::NONE;
	}

	Stock = 0;
	SLOT.Pace_Speed = 1 + Stock / 10;
}

// 左右の図柄が揃っていればリーチ。種類はランダムに決める
void Number_Management::DecideReach()
{
	if (Number_Hit[0] == Number_Hit[2])
	{
		is_Reach = true;
		Current_ReachType = static_cast<REACH_TYPE>(GetRand(REACHTYPE_NUMBER - 1) + 1);
		SLOT.is_Previous_EF_Swi[0] = true;  // リーチの時は前兆演出(画面揺れ)を出す
	}
	else
	{
		is_Reach = false;
		Current_ReachType = REACH_TYPE::NONE;
	}

	// 一定確率で「ホワイトアウト → バトル」演出に格上げ
	
	if (is_Reach && GetRand(99) < Reach_ACCEL_WHITEOUT_Probability)
	{
		Current_ReachType = REACH_TYPE::ACCEL_WHITEOUT;
	}
}

// 次の回転の抽選
void Number_Management::Out()
{
	for (int i = 0; i < REEL_NUMBER; i++)
	{
		Number_Hit[i] = GetRand(SYMBOL_NUMBER - 1);
	}

	// リーチ抽選に当たったら左右の図柄を揃える
	if (GetRand(99) < Reach_Probability)
	{
		Number_Hit[0] = Number_Hit[2];
	}

	DecideReach();

	// 保留を1つ消化。保留が多いほど回転を速くする
	Stock = std::max<double>(0, Stock - 1);
	SLOT.Pace_Speed = 1 + (Stock * 2) / 10;
}

//==============================================================
// Reach_Text
//==============================================================
void Reach_Text::Read()
{
	t = 0;
	swi = false;
	LoadDivGraph("REACH_THUNDER_add.png", 30, 4, 8, 640, 480, ReachThunder_Image, 0);

	for (int i = 0; i < 50; i++)
	{
		std::string filepath = std::format("REACH_TEXT/{:04d}.png", i + 1);
		image[i] = LoadGraph(filepath.c_str());
	}
}

void Reach_Text::Out()
{
	if (swi)
	{
		t += 0.50f;
		if (t == 20)
		{
			// 文字が出るタイミングで太鼓と雷
			PlaySoundMem(SOUND.ReachPut, DX_PLAYTYPE_BACK);
			ChangeVolumeSoundMem(255, SOUND.ReachThunder);
			PlaySoundMem(SOUND.ReachThunder, DX_PLAYTYPE_BACK);
		}
		else if (t == 22)
		{
			PlaySoundMem(SOUND.ReachBlow, DX_PLAYTYPE_BACK);
		}
	}
	else
	{
		t = 0;
	}
}

void Reach_Text::Draw()
{
	if (!swi || t > 50) return;

	int textFrame = std::min<int>(static_cast<int>(t), 49);
	
	int thunderFrame = std::min<int>(static_cast<int>(t), 29);

	// 雷(加算合成で光らせる)
	SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
	DrawExtendGraph(0, 100, 640, 380, ReachThunder_Image[thunderFrame], 1);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);

	// 「リーチ」文字
	DrawGraph(0, 0, image[textFrame], 1);
}

//==============================================================
// Hit_Effect
//==============================================================
void Hit_Effect::Read()
{
	Time = 0;
	is_swi = false;
	is_HitImageLoaded = false;

	// 背景アニメーションは図柄に関係なく共通なので、最初に1回だけ読み込む
	
	SetUseASyncLoadFlag(true);
	Hit_Back[0] = -1;  // 添字0は未使用
	for (int i = 1; i < 40; i++)
	{
		std::string name = "HIT_BACK/HitBack (" + std::to_string(i) + ").bmp";
		Hit_Back[i] = LoadGraph(name.c_str());
	}
	SetUseASyncLoadFlag(false);
}

void Hit_Effect::Out()
{
	if (is_swi == false) return;
	Time += 0.50f;
}

void Hit_Effect::Draw()
{
	int frame = std::min<int>(99, static_cast<int>(Time));  // 当たりアニメ(最後のコマで止める)
	int backFrame = (static_cast<int>(Time) % 39) + 1;           // 背景アニメ(1〜39をループ)

	DrawGraph(0, 0, Hit_Back[backFrame], 1);
	DrawGraph(0, 0, HitImage[frame], 1);
}

// 当たり図柄のアニメーション(例: 7なら Hit777.png)を読み込む
void Hit_Effect::Load()
{
	is_swi = true;

	// 前回の画像を解放してから読み込む(メモリリーク防止)
	if (is_HitImageLoaded)
	{
		for (int i = 0; i < 100; i++) DeleteGraph(HitImage[i]);
	}

	std::string num = std::to_string(NM.Number_Hit[0] + 1);
	std::string name = "Hit" + num + num + num + ".png";

	SetUseASyncLoadFlag(true);
	LoadDivGraph(name.c_str(), 100, 10, 10, 640, 480, HitImage);
	SetUseASyncLoadFlag(false);

	is_HitImageLoaded = true;
}
