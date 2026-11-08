//==============================================================
// GameLoader.cpp
//  共通リソース(図柄・背景・サウンド・入力・ステージチェンジ・数字画像)
//  の読み込みと更新処理
//==============================================================
#include "DxLib.h"
#include "GameLoader.h"
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <algorithm>

// グローバルインスタンス
GameLoader  GL;
Button      BT;
BackGround  BL;
Sound       SOUND;
StageChange SC;
Text        TEXT;

//--------------------------------------------------------------
// 画像を読み込む。失敗したらファイル名を表示して終了する。

//--------------------------------------------------------------
int LoadGraphOrExit(const std::string& path)
{
	int handle = LoadGraph(path.c_str());
	if (handle == -1)
	{
		
		std::string msg = "Failed to load image:\n" + path;
		MessageBox(NULL, msg.c_str(), "Error", MB_ICONERROR);
		DxLib_End();
		exit(0);
	}
	return handle;
}

//==============================================================
// GameLoader
//==============================================================
void GameLoader::Read()
{
	CurrentMode = GameScene::NORMAL;

	// 図柄画像 1.png 〜 9.png を読み込む
	Symbol_GraphHandle.resize(SYMBOL_NUMBER);
	for (int j = 0; j < SYMBOL_NUMBER; j++)
	{
		std::string graphName = std::to_string(j + 1) + ".png";
		Symbol_GraphHandle[j] = LoadGraphOrExit(graphName);
	}
}

//==============================================================
// BackGround
//==============================================================
void BackGround::Read()
{
	x = 0;
	y = 0;
	add_x = 0;
	add_y = 0;
	a = 100;        // 揺れ幅
	speed = 0.005f; // 揺れの速さ
	time = 0;

	// 通常背景 background1.png 〜 を読み込む
	BackGroundHandle.resize(BACKGROUND_NUMBER);
	for (int i = 0; i < BACKGROUND_NUMBER; i++)
	{
		std::string graphName = "background" + std::to_string(i + 1) + ".png";
		BackGroundHandle[i] = LoadGraphOrExit(graphName);
	}

	ResultBack = LoadGraphOrExit("nightview.png");
	White_Back = LoadGraphOrExit("white.png");

	CurrentBack = 1;
	FieldBackRND = CurrentBack;
	BackGroundSelect = BackGroundHandle[CurrentBack];
}

void BackGround::Out()
{
	// 基準値 + sin(時間 * スピード) * 振幅 で左右にゆっくり揺らす
	time += 1;
	x = -150 + static_cast<int>(sin(time * speed) * a);
}

void BackGround::Draw()
{
	DrawGraph(x + add_x, y + add_y, BackGroundSelect, true);
}

//==============================================================
// Sound
//==============================================================
void Sound::Read()
{
	for (int i = 0; i < REACH_VOICE_NUMBER; i++)
	{
		Reach_Voice[i] = LoadSoundMem("act_rich.mp3");
	}
	for (int i = 0; i < HITTINGBGM_NUMBER; i++)
	{
		HittingBGM[i] = LoadSoundMem("zankokuna.mp3");
	}

	// 通常BGMはサイズが大きいので、メモリに展開せずファイルからストリーム再生する
	SetCreateSoundDataType(DX_SOUNDDATATYPE_FILE);
	Field[0] = -1; // 添字0は未使用
	for (int i = 1; i < FIELDBGM_NUMBER; i++)
	{
		std::string fieldBGM = "FieldBGM" + std::to_string(i) + ".wav";
		Field[i] = LoadSoundMem(fieldBGM.c_str());
	}
	SetCreateSoundDataType(DX_SOUNDDATATYPE_MEMNOPRESS); // 以降は通常の読み込みに戻す

	CurrentFieldBGM = 1;
	FieldBGMRND = CurrentFieldBGM;
	FieldBGMSelect = Field[CurrentFieldBGM];
	HittingBGMSelect = 0;

	// 効果音
	Reelfinish = LoadSoundMem("Punch.mp3");
	Reachstart = LoadSoundMem("yashima.wav");
	whiteout = LoadSoundMem("horror_9.mp3");
	ReachPut = LoadSoundMem("wadaiko.mp3");
	ReachThunder = LoadSoundMem("Thunder.mp3");
	ReachBlow = LoadSoundMem("shakiin2.mp3");
	Reach_EF1_1 = LoadSoundMem("Sword.mp3");
	Reach_EF1_2 = LoadSoundMem("Fight.mp3");
	Button_Text = LoadSoundMem("shakiin2.mp3");
	Pre_EF1 = LoadSoundMem("dosyakuzure.mp3");
	ShutterClose = LoadSoundMem("bakuha.mp3");
	ShutterOpen = LoadSoundMem("openshutter.mp3");
	Hit1 = LoadSoundMem("Hit2.mp3");
	LandSymbol = LoadSoundMem("foot.mp3");
	Roulette = LoadSoundMem("roulette.mp3");
}

//==============================================================
// Button
//==============================================================
void Button::Read()
{
	is_RMOUSE = false;
	RMOUSE_Counter = 0;
}

void Button::Out()
{
	if ((GetMouseInput() & MOUSE_INPUT_RIGHT) != 0)
	{
		// 押している間カウントし、押した最初の1フレームだけ true にする
		RMOUSE_Counter++;
		is_RMOUSE = (RMOUSE_Counter == 1);
	}
	else
	{
		// 離したらリセット
		
		RMOUSE_Counter = 0;
		is_RMOUSE = false;
	}
}

//==============================================================
// StageChange
//  Time の進行に合わせて
//   8   : シャッターが閉まる音
//   50〜100 : (画面が隠れている間に)次の背景・BGMを抽選して差し替え
//   120 : シャッターが開く音、BGMを頭から再生
//   130 : 演出終了
//==============================================================
void StageChange::Read()
{
	Time = 0;
	is_change = false;
	Count = 0;

	// シャッターの連番画像(130コマ)を非同期で読み込む
	SetUseASyncLoadFlag(true);
	LoadDivGraph("shutter.png", 130, 12, 11, 640, 480, Image_Shutter);
	SetUseASyncLoadFlag(false);
}

void StageChange::Out()
{
	if (is_change == false) return;

	Time += 0.30f;
	int t = static_cast<int>(Time); // 判定用に整数化したコマ番号

	if (t > 130)
	{
		// 演出終了
		is_change = false;
		Time = 0;
		Count = 0;
	}
	else if (t > 50 && t < 100 &&
		(BL.FieldBackRND == BL.CurrentBack ||
			SOUND.FieldBGMRND == SOUND.CurrentFieldBGM))
	{
		// 今と違う背景・BGMが出るまで抽選し直す
		BL.FieldBackRND = GetRand(BACKGROUND_NUMBER - 1);

		switch (GL.CurrentMode)
		{
		case GameScene::STAGECHANGE:  // 通常のステージチェンジ
			BL.BackGroundSelect = BL.BackGroundHandle[BL.FieldBackRND];
			break;
		case GameScene::KAKUHEN_END:  // 確変終了(リザルト画面へ)
			BL.add_x = 0;
			BL.x = 0;
			BL.BackGroundSelect = BL.ResultBack;
			break;
		default:
			break;
		}

		SOUND.FieldBGMRND = GetRand(FIELDBGM_NUMBER - 2) + 1; // 1 〜 FIELDBGM_NUMBER-1
		SOUND.FieldBGMSelect = SOUND.Field[SOUND.FieldBGMRND];
	}
	else if (t == 8 && CheckSoundMem(SOUND.ShutterClose) == 0)
	{
		PlaySoundMem(SOUND.ShutterClose, DX_PLAYTYPE_BACK);
	}
	else if (t == 120 && CheckSoundMem(SOUND.ShutterOpen) == 0)
	{
		PlaySoundMem(SOUND.ShutterOpen, DX_PLAYTYPE_BACK);
		BL.CurrentBack = BL.FieldBackRND;
		SOUND.CurrentFieldBGM = SOUND.FieldBGMRND;
		SetSoundCurrentTime(0, SOUND.FieldBGMSelect); // 新しいBGMを頭から
	}
}

void StageChange::Draw()
{
	if (is_change == false) return;

	int frame = std::min<int>(static_cast<int>(Time), 129); // 配列外を参照しないよう制限
	DrawRotaGraph(640 / 2, 480 / 2, 1.1f, 0, Image_Shutter[frame], 1);
}

//==============================================================
// Text
//==============================================================
void Text::Read()
{
	// NUM/num0.png 〜 num9.png
	for (int i = 0; i < 10; i++)
	{
		std::string name = "NUM/num" + std::to_string(i) + ".png";
		Number[i] = LoadGraphOrExit(name);
	}
}

void Text::DrawNumber(int x, int y, int value, int spacing) const
{
	if (value < 0) value = 0;
	std::string digits = std::to_string(value);
	for (int i = 0; i < static_cast<int>(digits.size()); i++)
	{
		int d = digits[i] - '0'; // 文字 → 数値 に変換
		DrawRotaGraph(x + spacing * i, y, 1.0f, 0, Number[d], true);
	}
}
