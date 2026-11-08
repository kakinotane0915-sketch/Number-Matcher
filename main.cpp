//==============================================================
// main.cpp
//  初期化とメインループ
//
//  シーンの流れ
//   NORMAL ──(リーチ → ホワイトアウト)──→ REACH ──(勝ち)──→ HIT ──→ KAKUHEN
//     ↑  │                                  │(負け)               ↑      │
//     │  └─(ステージチェンジ)→ STAGECHANGE   ↓                      └(当たり)┤
//     └──────────────────────────────── NORMAL                            │
//     └────────────────── KAKUHEN_END(リザルト) ←──(残り回数0)────────────┘
//==============================================================
#include "DxLib.h"
#include "GameLoader.h"
#include "Slot.h"
#include "Reach_Effect.h"
#include "kakuhen.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//---------------- 画面設定 ----------------
	SetWindowSize(1280, 720);
	SetFullScreenResolutionMode(DX_FSRESOLUTIONMODE_DESKTOP);  // デスクトップ解像度で全画面
	SetFullScreenScalingMode(DX_FSSCALINGMODE_BILINEAR);       // 640x480 を滑らかに拡大

	if (DxLib_Init() == -1)
	{
		return -1;
	}

	SetDrawScreen(DX_SCREEN_BACK);  // ダブルバッファ

	// 乱数の種を起動時刻から決める
	// (以前の SRand(GetRand(100)) だと101通りしかなかった)
	SRand(GetNowCount());

	//---------------- リソース読み込み・初期化 ----------------
	// 
	GL.Read();
	BL.Read();
	TEXT.Read();
	SLOT.Read();
	NM.Read();
	SOUND.Read();
	BT.Read();
	RT.Read();
	Reach_EF1.Read();
	SC.Read();
	HE.Read();
	KAKUHEN.Read();

	GameScene Current_GameScene = GameScene::NORMAL;

	//---------------- メインループ ----------------
	while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
	{
		ClearDrawScreen();
		GL.CurrentMode = Current_GameScene;

		// 入力はどのシーンでも毎フレーム更新する
		
		BT.Out();

		switch (Current_GameScene)
		{
			//======================================================
			// 通常回転
			//======================================================
		case GameScene::NORMAL:
			BL.Out();
			SLOT.Out();
			RT.Out();
			SC.Out();

			BL.Draw();
			SLOT.Draw();
			RT.Draw();
			SC.Draw();

			if (SLOT.is_white == true && SLOT.white_Alpha > 253)
			{
				// ホワイトアウトしきったらリーチ演出(バトル)へ
				Reach_EF1.SetSymbols();
				Current_GameScene = GameScene::REACH;
			}
			else if (SC.is_change == true)
			{
				// ステージチェンジへ
				Current_GameScene = GameScene::STAGECHANGE;
				StopSoundMem(SOUND.FieldBGMSelect);
			}
			break;

			//======================================================
			// リーチ演出(数字同士のバトル)
			//======================================================
		case GameScene::REACH:
			BL.Out();
			Reach_EF1.Out();

			BL.Draw();
			Reach_EF1.Draw();
			Reach_EF1.White_Draw();

			// 演出が終わり、画面が真っ白になったら結果に応じてシーンを切り替える
			if (Reach_EF1.Sta == Reach_Effect1::STATE::Finish &&
				Reach_EF1.Time > 254 && Reach_EF1.white_alpha > 254)
			{
				Reach_EF1.Sta = Reach_Effect1::STATE::Expansion;

				// 通常画面に戻った時のため、全リールをリセットしておく
				for (int i = 0; i < REEL_NUMBER; i++) SLOT.Reel_State[i] = Slot::STATE::RESET;

				// 中リールの図柄を結果に合わせる
				// 勝ち → 当たり図柄で揃える / 負け → 1つ後ろ(はずれ)
				
				int hit = NM.Number_Hit[0];
				if (Reach_EF1.is_WinorLose == true) SLOT.Current_Number[1] = hit;
				else                                SLOT.Current_Number[1] = (hit + 1) % SYMBOL_NUMBER;

				if (Reach_EF1.is_WinorLose == true)
				{
					// 大当たりへ
					Current_GameScene = GameScene::HIT;
					SOUND.HittingBGMSelect = GetRand(HITTINGBGM_NUMBER - 1);
					HE.Load();  // 当たった時だけ大当たり画像を読み込む
				}
				else
				{
					Current_GameScene = GameScene::NORMAL;
				}

				// ホワイトアウト関連のリセット
				SLOT.is_white = false;
				SLOT.white_Alpha = 0;
				Reach_EF1.is_white = false;
				StopSoundMem(SOUND.Reachstart);

				WaitTimer(300);  // 結果が出る前に一瞬間を置く(演出上の意図的な停止)
			}
			break;

			//======================================================
			// ステージチェンジ(シャッター演出)
			//======================================================
		case GameScene::STAGECHANGE:
			SC.Out();

			BL.Draw();
			SLOT.Draw();
			SC.Draw();

			if (SC.is_change == false) Current_GameScene = GameScene::NORMAL;
			break;

			//======================================================
			// 大当たり演出
			//======================================================
		case GameScene::HIT:
			HE.Out();
			HE.Draw();

			if (HE.Time < 1 && CheckSoundMem(SOUND.Hit1) == 0)
			{
				// 演出開始:BGMを止めて大当たり音を鳴らす
				StopSoundMem(SOUND.HittingBGM[SOUND.HittingBGMSelect]);
				PlaySoundMem(SOUND.Hit1, DX_PLAYTYPE_BACK, TRUE);
				KAKUHEN.HittingTimes++;
			}
			else if (HE.Time >= 300)
			{
				// 演出終了 → 確変へ
				HE.Time = 0;
				HE.is_swi = false;
				Reach_EF1.is_WinorLose = false;
				StopSoundMem(SOUND.Hit1);
				
				PlaySoundMem(SOUND.HittingBGM[SOUND.HittingBGMSelect], DX_PLAYTYPE_LOOP, FALSE);
				Current_GameScene = GameScene::KAKUHEN;
			}
			break;

			//======================================================
			// 確変
			//======================================================
		case GameScene::KAKUHEN:
			BL.Out();
			BL.Draw();
			KAKUHEN.Out();
			KAKUHEN.DrawTimes();

			if (KAKUHEN.is_Hit == true && KAKUHEN.is_Finish == true)
			{
				// 確変中に当たり → 大当たり演出へ
				NM.Number_Hit[0] = KAKUHEN.Number_Hit[0];
				KAKUHEN.is_Hit = false;
				Current_GameScene = GameScene::HIT;
			}
			else if (KAKUHEN.RemainingTimes <= 0)
			{
				// 残り回数0 → リザルトへ(シャッター演出で背景をリザルト用に変える)
				Current_GameScene = GameScene::KAKUHEN_END;
				SC.is_change = true;
				StopSoundMem(SOUND.HittingBGM[SOUND.HittingBGMSelect]);
			}
			break;

			//======================================================
			// 確変終了(リザルト画面)
			//======================================================
		case GameScene::KAKUHEN_END:
			BL.Draw();

			// シャッターが閉じきるまでは確変の画面を残しておく
			if (SC.Time < 50 && SC.is_change)
			{
				for (int i = 0; i < REEL_NUMBER; i++) KAKUHEN.Draw(i);
				KAKUHEN.DrawTimes();
			}

			SC.Out();
			KAKUHEN.EndOut();
			SC.Draw();
			KAKUHEN.EndDraw();

			if (KAKUHEN.EndTimes > 460)
			{
				// 通常に戻す
				KAKUHEN.EndTimes = 0;
				KAKUHEN.RemainingTimes = KAKUHEN_TIMES;
				KAKUHEN.HittingTimes = 0;  // 次の大当たりのためにリセット
				BL.FieldBackRND = GetRand(BACKGROUND_NUMBER - 1);
				BL.BackGroundSelect = BL.BackGroundHandle[BL.FieldBackRND];
				Current_GameScene = GameScene::NORMAL;
			}
			break;
		}

		//---------------- 保留(右クリックで追加) ----------------
		if (BT.is_RMOUSE == true && NM.Stock < STOCK_MAX)
		{
			NM.Stock++;
		}

		ScreenFlip();

		//---------------- フレーム待ち ----------------
		if (DEBUG_MODE && CheckHitKey(KEY_INPUT_SPACE))
		{
			WaitTimer(60);  // デバッグ用:SPACE押下中はスロー
		}
		else
		{
			WaitTimer(12);  // 約0.012秒待つ
		}
	}

	DxLib_End();
	return 0;
}
