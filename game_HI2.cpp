#include "DxLib.h"
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <ctime>
#include <math.h>

#ifndef EXHEADER_LOADED_H
#include "./../noz_kingdom/env_value.h"
#endif

#ifndef EXHEADER_LOADED_H
#include "./../noz_kingdom/externalHeader.h"
#endif

#ifndef PLAYER_LOADED_H
#include "./../noz_kingdom/gamePlayer.h"
#endif

//==============================
// 1. 定数・グローバル変数の定義
//==============================

extern int GAME_MODE;


namespace HiGame2 {

	const int FPS = 60;
	const int WHITE = GetColor(255, 255, 255);
	const int BALL_MAX = 50;
	const int TIME_LIMIT = 40;
	const int I_0 = 3; //　初期のボール生成間隔(秒)(CallCreateBalls)
	const double Z_TOUCH_THRESHOLD = 0.90; // このz以上(=プレイヤー接触可能範囲)でボールの最大サイズ扱い
	const int BALL_CONTACT_RANGE = 20;// プレイヤーが接触するときの範囲
	const int DISPLAY_RADIUS_MAX = 32; // ボールの最大サイズ
	const double RADIUS_LERP_SPEED = 0.08; // 表示半径の補完速度
	const double GRAVITY = 0.2; // z　単位 / 秒^2 (落下加速度)

	int player_num = 0;

	int title_timer = 0;
	int timer = 0; //ゲームの経過時間をカウントするタイマー
	int ball_timer = 0; // ボールの生成時間をカウントするタイマー
	double ball_interval; // 現在のボール生成間隔(CallCreateBalls)
	double ball_elapsed_time; // 経過時間(CallCreateBalls)
	int ball_last_creation_time; // 最後にボールを生成した時刻(フレーム数)(CallCreateBalls)
	int count_balls = 0;
	int reverse_game_timer = 0; // 対戦時間を表示するためのタイマー
	int countdown_timer = 0; //カウントダウン用タイマー
	int display_timer = 0;  //メッセージ描画時間延長用タイマー
	bool is_counting_down = false; // カウントダウンフラグ


	// 勝者判定
	int winner_team = -1; // 0 or 1, 引き分けは　-1
	bool is_draw = false;

	// 各シーン関数プロトタイプ宣言
	int TitleScene(void);
	int PlayerCheckScene(void);
	int TutorialScene(void);
	int MainGameScene(void);
	int GameOverScene(void);

	//シーン定義
	enum Scene {
		TITLE,
		PLAYER_CHECK,
		TUTORIAL,
		GAME,
		GAME_OVER,
	};
	int scene = TITLE;

	//==============================
	// 2. 構造体定義とオブジェクト配列
	//==============================

	struct Player {
		int team;
		int x, y;
		int speed;
	};

	struct Ball {
		int state;
		int x, y;
		int radius;	// 接触半径(常時20)
		double display_radius; // 描画半径
		double target_radius; // そのフレームでの目標半径
		double movespeed_x;
		double movespeed_y;
		double z_index;
		double z_velocity; // z方向加速度 (負 = 上方向)
	};

	Player player[4];
	Ball ball[BALL_MAX];
	int teamscore[2] = { 0, 0 };
	int teamcolors[2] = { GetColor(95, 204, 235), GetColor(238, 47, 115) };
	int playerTeam[4] = { 1, 1, 1, 1 }; //チームメンバーを分けるための配列(初期化段階では全て1)


	

	//================================
	// 3. 関数群
	//================================

	//--------------------------------
	// 3.1 汎用関数
	//--------------------------------

	// 初期化関数 (プログラム最初で一度だけ呼出し)
	void InitRandom() {
		std::srand(static_cast<unsigned int>(std::time(nullptr))); // 乱数初期化
	}

	// 指定した範囲でdouble型乱数を返す変数 (GetRand拡張)
	double GetRandmInRange(double min, double max) {
		// min 以上 max 未満の乱数を生成
		double Value = min + (static_cast<double>(GetRand(10000) / (10000.0) * (max - min)));
		return Value;
	}

	//--------------------------------
	// 3.2 プレイヤー操作
	//--------------------------------

	// プレイヤー数判定
	void Check_Players(void) {
		if (Num_JoyPads == 0) {
			Num_JoyPads = 1;
		}
		if (Num_JoyPads > 4) {
			Num_JoyPads = 4;
		}
		player_num = Num_JoyPads;
	}

	// チーム振り分け
	void Team_Distribution(void) {
		if (player_num <= 0) return; //　基本的にCheck_Pkayers()後にチーム分けを行うため、不必要


		for (int i = 0; i < 4; i++) playerTeam[i] = 1; //初期化(周回プレイ時にチームリセット)

		// インデックス配列を作成してシャッフル
		int idx[4];
		for (int i = 0; i < player_num; i++) idx[i] = i;
		for (int i = player_num - 1; i > 0; i--) {
			int j = GetRand(i);
			int tmp = idx[i];
			idx[i] = idx[j];
			idx[j] = tmp;
		}

		int half = player_num / 2; // 先頭 half 人を team 0 に
		for (int k = 0; k < half; k++) {
			playerTeam[idx[k]] = 0;
		}
	}

	void InitPlayers(void) {
		for (int i = 0; i < Num_JoyPads; i++) {
			player[i].team = playerTeam[i];
			player[i].speed = 4;
		}
	}


	void move(void) {
		for (int i = 0; i < Num_JoyPads; i++) {
			if (Num_JoyPads == 1) {
				if (CheckHitKey(KEY_INPUT_RIGHT) == 1) player[i].x += player[i].speed;
				if (CheckHitKey(KEY_INPUT_LEFT) == 1) player[i].x -= player[i].speed;
				if (CheckHitKey(KEY_INPUT_UP) == 1) player[i].y -= player[i].speed;
				if (CheckHitKey(KEY_INPUT_DOWN) == 1) player[i].y += player[i].speed;
			}
			else {
				if (i == 0) {
					if (IsKey_PushedLong(DX_INPUT_PAD1, 2, 1) == TRUE) player[0].x += player[0].speed;
					if (IsKey_PushedLong(DX_INPUT_PAD1, 1, 1) == TRUE) player[0].x -= player[0].speed;
					if (IsKey_PushedLong(DX_INPUT_PAD1, 3, 1) == TRUE) player[0].y -= player[0].speed;
					if (IsKey_PushedLong(DX_INPUT_PAD1, 0, 1) == TRUE) player[0].y += player[0].speed;
				}
				if (i == 1) {
					if (IsKey_PushedLong(DX_INPUT_PAD2, 2, 1) == TRUE) player[1].x += player[1].speed;
					if (IsKey_PushedLong(DX_INPUT_PAD2, 1, 1) == TRUE) player[1].x -= player[1].speed;
					if (IsKey_PushedLong(DX_INPUT_PAD2, 3, 1) == TRUE) player[1].y -= player[1].speed;
					if (IsKey_PushedLong(DX_INPUT_PAD2, 0, 1) == TRUE) player[1].y += player[1].speed;
				}
				if (i == 2) {
					if (IsKey_PushedLong(DX_INPUT_PAD3, 2, 1) == TRUE) player[2].x += player[2].speed;
					if (IsKey_PushedLong(DX_INPUT_PAD3, 1, 1) == TRUE) player[2].x -= player[2].speed;
					if (IsKey_PushedLong(DX_INPUT_PAD3, 3, 1) == TRUE) player[2].y -= player[2].speed;
					if (IsKey_PushedLong(DX_INPUT_PAD3, 0, 1) == TRUE) player[2].y += player[2].speed;
				}
				if (i == 3) {
					if (IsKey_PushedLong(DX_INPUT_PAD4, 2, 1) == TRUE) player[3].x += player[3].speed;
					if (IsKey_PushedLong(DX_INPUT_PAD4, 1, 1) == TRUE) player[3].x -= player[3].speed;
					if (IsKey_PushedLong(DX_INPUT_PAD4, 3, 1) == TRUE) player[3].y -= player[3].speed;
					if (IsKey_PushedLong(DX_INPUT_PAD4, 0, 1) == TRUE) player[3].y += player[3].speed;
				}
			}
			// 移動座標の範囲制限
			if (player[i].x < 35) player[i].x = 35;
			else if (player[i].x > SCREEN_WIDTH - 35) player[i].x = SCREEN_WIDTH - 35;
			if (player[i].y < 35) player[i].y = 35;
			else if (player[i].y > SCREEN_HEIGHT - 35) player[i].y = SCREEN_HEIGHT - 35;

			// チームでの移動座標の範囲制限
			if (player[i].team == 0) {
				if (player[i].x < 55) player[i].x = 55; // 左方余白 + 自機幅
				else if (player[i].x > ScreenCenter_X - 45) player[i].x = ScreenCenter_X - 45; // 中央線余白 + 自機幅
				if (player[i].y < 103) player[i].y = 103; // 上方余白 + 自機幅
				else if (player[i].y > SCREEN_HEIGHT - 55) player[i].y = SCREEN_HEIGHT - 55;
			}
			else if (player[i].team == 1) {
				if (player[i].x < ScreenCenter_X + 45) player[i].x = ScreenCenter_X + 45;
				else if (player[i].x > SCREEN_WIDTH - 55) player[i].x = SCREEN_WIDTH - 55;
				if (player[i].y < 103) player[i].y = 103;
				else if (player[i].y > SCREEN_HEIGHT - 55) player[i].y = SCREEN_HEIGHT - 55;
			}
		}

	}


	// チーム振り分けに沿った開始位置の設定
	void SetStartPositions(void) {
		const int leftX = 100;
		const int rightX = SCREEN_WIDTH - 100;
		const int Offset = 75;

		if (player_num == 1) {
			player[0].x = (player[0].team == 0) ? leftX : rightX;
			player[0].y = ScreenCenter_Y;
		}
		else if (player_num == 2) {
			for (int i = 0; i < player_num; i++) {
				player[i].x = (player[i].team == 0) ? leftX : rightX;
				player[i].y = ScreenCenter_Y;
			}
		}
		else if (player_num == 4) {
			int positionIdx[2] = { 0, 0 }; // teamごとの割当インデックス（0->top, 1->bottom）
			for (int i = 0; i < 4; i++) {
				int idx = positionIdx[player[i].team]++;
				int y = (idx == 0) ? (ScreenCenter_Y - Offset) : (ScreenCenter_Y + Offset);
				int x = (player[i].team == 0) ? leftX : rightX;
				player[i].x = x;
				player[i].y = y;

			}
		}
		// 未接続スロット(4未満)は画面外へ
		for (int i = player_num; i < 4; i++) {
			player[i].x = -200;
			player[i].y = -200;
		}
	}
	//上の関数を使用すると操作不可となった

	// プレイヤー描画用(毎フレーム)
	void DrawPlayers(void) {
		for (int i = 0; i < player_num; i++) {
			DrawBox(player[i].x - 35, player[i].y - 35, player[i].x + 35, player[i].y + 35, teamcolors[player[i].team], TRUE);
		}
	}

	void DrawTeamScore(void) {
		char score_team0[10], score_team1[10], remaining_game_time[10];
		sprintf_s(score_team0, "%d", teamscore[0]);
		sprintf_s(score_team1, "%d", teamscore[1]);
		sprintf_s(remaining_game_time, "%d", reverse_game_timer / FPS);
		DrawStringToHandle(340, 10, score_team0, WHITE, Fonts[0]);
		DrawStringToHandle(ScreenCenter_X - 25, 10, remaining_game_time, WHITE, Fonts[0]);
		DrawStringToHandle(1020, 10, score_team1, WHITE, Fonts[0]);
	}

	void DrawField(void) {
		DrawLine(20, 68, SCREEN_WIDTH - 20, 68, WHITE); //上
		DrawLine(20, 68, 20, SCREEN_HEIGHT - 20, WHITE); //左
		DrawLine(20, SCREEN_HEIGHT - 20, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 20, WHITE); //下
		DrawLine(SCREEN_WIDTH - 20, 68, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 20, WHITE); //右
		DrawBox(ScreenCenter_X - 10, 68, ScreenCenter_X + 10, SCREEN_HEIGHT - 20, WHITE, TRUE); //中央線
	}

	// ゲーム開始時の各プレイヤー状のネーム表示
	void DispPlayer(void) {
		if (timer < 300) {
			for (int i = 0; i < player_num; i++) {
				DrawFormatString(player[i].x - 10, player[i].y - 10, WHITE, "P%d", i + 1);
			}
		}
	}



	//--------------------------------
	// 3.2 ボール処理関連
	//--------------------------------
	
	// ボールを生成する関数(ゲームシーンでは呼ばない)
	void CreateBalls(void) {
		for (int i = 0; i < BALL_MAX; i++) {
			if (ball[i].state == 0) {
				ball[i].x = ScreenCenter_X + GetRand(30) - 15;
				ball[i].y = SCREEN_HEIGHT - 30;
				float speed = sqrt(15);
				float angle = GetRandmInRange(10.0, 170.0);
				ball[i].movespeed_x = speed * cos(angle * PI / 180.0);
				ball[i].movespeed_y = speed * sin(angle * PI / 180.0);

				// 高度(z)の初期値と初速
				ball[i].z_index = 0.99;
				ball[i].z_velocity = -GetRandmInRange(0.45, 0.55);

				ball[i].radius = BALL_CONTACT_RANGE;
				ball[i].display_radius = (double)BALL_CONTACT_RANGE;
				ball[i].target_radius = (double)BALL_CONTACT_RANGE;
				ball[i].state = 1;
				break;
			}
		}
	}

	// ボールの生成秒数を管理する関数
	void CallCreateBalls(void) {
		const double I_min = 1.0; // 最小ボール生成間隔(秒)
		const double r = 0.1; // (ボール生成の加速率)(秒)

		// 経過時間を計算
		ball_elapsed_time += 1.0 / FPS; // 1フレームの時間を加算

		// ボール生成間隔の計算(I(t) = max(I_0 - r * t, I_min))
		ball_interval = I_0 - r * (ball_elapsed_time);
		if (ball_interval < I_min) {
			ball_interval = I_min; // 最小間隔以下になることを防ぐ
		}

		// ボール生成条件
		if (ball_last_creation_time + (int)(ball_interval * FPS) <= timer) {
			CreateBalls();
			ball_last_creation_time = timer; // 現在の時刻を記録
		}	
	}

	// ボールの影を描画する関数(MoveBallから呼出し)
	void DrawBallShadow(const Ball& b) {
		if (b.state == 0) return;

		// 地面に接地しているため、影の描画をスキップ
		if (b.z_index >= Z_TOUCH_THRESHOLD) return;

		// 計算用の"みなし"のボールの高さ
		// 接地(z_index >= 0.90)の時は高度なし、規定値を超えた高さは 0 と固定
		double deemed_z_index = b.z_index;
		if (b.z_index >= Z_TOUCH_THRESHOLD) deemed_z_index = 1.0;
		if (b.z_index <= 0.0) deemed_z_index = 0.0;


		// 空中に浮いているボールが、地面に近づくほど影がボールに近づくよう、オフセット計算
		// 光源は画面奥方向、上空から当たっているように、xの影のオフセットは計算しない
		double shadowY = b.y + (70.0 * (1.0 - deemed_z_index));

		//  影スケール設定(地面に近いほど、ボールと同サイズ、離れるほど小さく(描画サイズ*0.5))
		const double minScale = 0.3;
		double shadow_scale = b.display_radius * (minScale + 0.7 * deemed_z_index);

		// 影の不透明度（高さが高いほど薄く）
		const int maxAlpha = 140;
		const int minAlpha = 30;
		int alpha = (int)(maxAlpha * (1.0 - (-b.z_index) * 0.9));
		if (alpha < minAlpha) alpha = minAlpha;
		if (alpha > maxAlpha) alpha = maxAlpha;


		// 楕円描画
		SetDrawBlendMode(DX_BLENDGRAPHTYPE_ALPHA, alpha);
		DrawCircle(b.x, shadowY, shadow_scale, WHITE, TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	void MoveBalls(void) {
		double dt = 1.0 / FPS;

		for (int i = 0; i < BALL_MAX; i++) {
			if (ball[i].state == 0) continue;

			//　2D座標移動
			ball[i].x += ball[i].movespeed_x;
			ball[i].y -= ball[i].movespeed_y;

			// z方向の重力
			// z_velocityに重力を加算(下方向に加速のため符号は正)
			ball[i].z_velocity += GRAVITY * dt;
			// zに速度を積分
			ball[i].z_index += ball[i].z_velocity * dt;


			// 地面(z_index == 1.00)に到達で重力処理停止(state = 0 なので不要かも)
			if (ball[i].z_index >= 1.0) {
				ball[i].z_index = 1.0;
				ball[i].z_velocity = 0.0;
			}

			// zに応じた半径更新(描画)
			if (ball[i].z_index >= Z_TOUCH_THRESHOLD) {
				// 地面付近（最大サイズ）
				ball[i].target_radius = (double)BALL_CONTACT_RANGE;
			}
			else {
				// 空中
				double z_high_ratio = ball[i].z_index / Z_TOUCH_THRESHOLD; // 0...1(0が最高点)
				if (z_high_ratio < 0.0) z_high_ratio = 0.0;
				if (z_high_ratio > 1.0) z_high_ratio = 1.0;
				double interp = 1.0 - z_high_ratio;
				ball[i].target_radius = (double)BALL_CONTACT_RANGE + (DISPLAY_RADIUS_MAX - BALL_CONTACT_RANGE) * interp;
			}

			// --- display_radius を滑らかに補間（線形補間） ---
			ball[i].display_radius += (ball[i].target_radius - ball[i].display_radius) * RADIUS_LERP_SPEED;


			DrawBallShadow(ball[i]);

			// 描画（整数に丸めて渡す）
			int drawR = (int)(ball[i].display_radius + 0.5);
			DrawCircle(ball[i].x, ball[i].y, drawR, GetColor(95, 204, 235), TRUE);
			DrawFormatString(ball[i].x, ball[i].y - 20, WHITE, "%0.3f", ball[i].z_index);
		}
	}

	void CheckBallBounceWall(void) {
		for (int i = 0; i < BALL_MAX; i++) {
			if (ball[i].state == 0) continue;

			int drawR = (int)(ball[i].display_radius + 0.5);

			// 上の壁との衝突
			if (ball[i].y - drawR <= 68) {
				ball[i].movespeed_y = -ball[i].movespeed_y; // Y速度を反転
				ball[i].y = 68 + drawR; // ボールを壁の内側に移動
			}

			// 下の壁との衝突
			if (ball[i].y + drawR >= SCREEN_HEIGHT - 20) {
				ball[i].movespeed_y = -ball[i].movespeed_y; // Y速度を反転
				ball[i].y = SCREEN_HEIGHT - 20 - drawR; // ボールを壁の内側に移動
			}

			// 左の壁との衝突
			if (ball[i].x - drawR <= 20) {
				ball[i].movespeed_x = -ball[i].movespeed_x; // X速度を反転
				ball[i].x = 20 + drawR; // ボールを壁の内側に移動
			}

			// 右の壁との衝突
			if (ball[i].x + drawR >= SCREEN_WIDTH - 20) {
				ball[i].movespeed_x = -ball[i].movespeed_x; // X速度を反転
				ball[i].x = SCREEN_WIDTH - 20 - drawR; // ボールを壁の内側に移動
			}
		}
	}

	void CheckPlayerBounce(void) {
		const double PLAYER_HALF = 35.0; // プレイヤーの半幅
		const double HIT_MAX_RANGE = PLAYER_HALF + (double)BALL_CONTACT_RANGE; // 当たり判定の最大距離
		const double MIN_SPEED = 3.0; // ボールの画面移動(movespeed)の最小距離
		const double MAX_SPEED = 7.0; // ボールの画面移動(movespeed)の最大距離

		for (int i = 0; i < BALL_MAX; i++) {
			if (ball[i].state == 0) continue;

			// "z_index <  0.90"の時のみプレイヤーが触れる
			// 描画サイズが小さくても当たり判定には差異が出ないよう定数にする
			if (ball[i].z_index < Z_TOUCH_THRESHOLD) continue;

			for (int j = 0; j < Num_JoyPads; j++) {
				// プレイヤー中心とボール中心の差分
				double dx = (double)ball[i].x - (double)player[j].x;
				double dy = (double)ball[i].y - (double)player[j].y;
				double dist = sqrt(dx * dx + dy * dy);

				// 接触判定：中心間距離が HIT_MAX_RANGE 以下なら接触
				if (dist <= HIT_MAX_RANGE) {
					// 単位ベクトル（プレイヤー -> ボールの方向）
					double nx = 0.0, ny = -1.0;
					if (dist > 1e-6) {
						nx = dx / dist;
						ny = dy / dist;
					}

					// 中心からのずれが小さいほど強く飛ばす（近いほど強く）
					double dist_center = 1.0 - (dist / HIT_MAX_RANGE); // 0..1
					if (dist_center < 0.0) dist_center = 0.0;
					if (dist_center > 1.0) dist_center = 1.0;

					double speed = MIN_SPEED + (MAX_SPEED - MIN_SPEED) * dist_center;

					// -x 方向は movespeed_xにそのまま入れる
					// - y 方向は movespeed_y を正にすると画面上方向に移動する（MoveBalls で y -= movespeed_y）
					ball[i].movespeed_x = nx * speed;
					ball[i].movespeed_y = -ny * speed;

					// z（高さ）方向の初速を与えて上に打ち上げる（負 = 上方向）
					// 初速はランダムにしてバラつきを持たせる（必要なら固定値に）
					ball[i].z_velocity = -GetRandmInRange(0.45, 0.80);

					// 空中状態に戻す（地面フラグ解除）
					ball[i].z_index = 0.0;

					// 少し上にずらして即座に再接触しないようにする
					ball[i].y -= 4;

					// 一度当たったら他のプレイヤーで重複処理しない
					break;
				}
			}
		}
	}


	// ボールの落下時にスコア加算を行う関数
	// プレイヤーの跳ね返し処理の後に書いてプレイヤーがいるのに点数加算されることを防ぐ必要あり?
	void CheckPoints(void) {
		for (int i = 0; i < BALL_MAX; i++) {
			if (ball[i].state == 0) continue;
			for (int j = 0; j < Num_JoyPads; j++) {
				if (ball[i].state == 1 && ball[i].z_index == 1.00) {
					if (ball[i].x > 20 && ball[i].x < ScreenCenter_X - 10 && ball[i].y > 68 && ball[i].y < SCREEN_HEIGHT - 20) {
						ball[i].state = 0;
						teamscore[1] += 1;
					}
					else if (ball[i].x > ScreenCenter_X + 10 && ball[i].x < SCREEN_WIDTH - 20 && ball[i].y > 68 && ball[i].y < SCREEN_HEIGHT - 20) {
						ball[i].state = 0;
						teamscore[0] += 1;
					}
				}
			}
		}
	}

	// ゲーム開始時にボール生成関連のリセットを行う
	void ResetBallCreationState() {
		ball_interval = I_0;
		ball_elapsed_time = 0.0;
		ball_last_creation_time = 0;
		for (int i = 0; i < BALL_MAX; i++) {
			ball[i].state = 0;
		}
	}

	//--------------------------------
	// 3.4 ゲームシステム関連
	//--------------------------------

	// 現在の画面上のボール数を数える(デバッグ用)
	void CountExistanceBalls(void) {
		int ball_counter = 0;
		for (int i = 0; i < BALL_MAX; i++) {
			if (ball[i].state == 1)
				ball_counter += 1;
		}
		count_balls = ball_counter;
	}

	// ゲーム開始時にスコアリセットを行う
	void ResetScore(void) {
		teamscore[0] = 0;
		teamscore[1] = 0;
	}

	void GameOver_Check(void) {
		if (reverse_game_timer <= 0) {
			scene = GAME_OVER;
		}
	}

	void Check_Winner(void) {
		if (teamscore[0] > teamscore[1]) {
			winner_team = 0;
			is_draw = false;
		}
		else if (teamscore[0] < teamscore[1]) {
			winner_team = 1;
			is_draw = false;
		}
		else {
			winner_team = -1;
			is_draw = true;
		}
	}

	//--------------------------------
	// 3.5 シーン管理
	//^-------------------------------

	void GameScene(void) {
		switch (scene) {
		case TITLE:
			TitleScene();
			break;
		case PLAYER_CHECK:
			PlayerCheckScene();
			break;
		case TUTORIAL:
			TutorialScene();
			break;
		case GAME:
			MainGameScene();
			break;
		case GAME_OVER:
			GameOverScene();
			break;
		default:
			break;
		}
	}

	//================================
	// 4. 各シーン処理
	//================================

	//-----タイトルシーン-----
	int TitleScene(void) {
		const int blink_interval = 30;
		bool string_view = true;
		title_timer++;
		Check_Players();
		DrawStringToHandle(ScreenCenter_X - 150, ScreenCenter_Y - 200, "タイトル", WHITE, Fonts[2]);
		DrawFormatString(10, 10, WHITE, "Connected Players: %d", Num_JoyPads);
		if (title_timer >= blink_interval) {
			DrawStringToHandle(ScreenCenter_X - 250, 380, "PRESS Z KEY", WHITE, Fonts[0]);
			if (title_timer >= 60) {
				title_timer = 0; //カウントリセット
			}
		}
		if (player_num < 2 || player_num == 3) {
			if (IsKey_PushedNow(DX_INPUT_KEY_PAD1, 4) == TRUE) {
				scene = PLAYER_CHECK;
			}
		}
		else {
			if (IsKey_PushedNow(DX_INPUT_KEY_PAD1, 4) == TRUE) {
				scene = TUTORIAL;
			}
		}
		return 0;
	}

	int PlayerCheckScene(void) {
		DrawFormatString(0, 0, WHITE, "Player %d", player_num);
		if (player_num < 2) {
			DrawStringToHandle(ScreenCenter_X, ScreenCenter_Y - 200, "このゲームは2人以上専用ゲームだよ", WHITE, Fonts[1]);
			DrawStringToHandle(ScreenCenter_X, ScreenCenter_Y - 150, "友達を作って出直してきてね", WHITE, Fonts[1]);
			if (IsKey_PushedNow(DX_INPUT_KEY_PAD1, 4) == TRUE) {
				scene = TITLE;
			}
			else if (IsKey_PushedNow(DX_INPUT_KEY_PAD1, 5) == TRUE) { //なぜか無反応
				scene = TUTORIAL;
			}
		}
		else if (player_num == 3) {
			DrawStringToHandle(ScreenCenter_X, ScreenCenter_Y - 200, "このゲームは2人か4人でプレイ可能です", WHITE, Fonts[1]);
			DrawStringToHandle(ScreenCenter_X, ScreenCenter_Y - 150, "プレイヤー人数を変更してください", WHITE, Fonts[1]);
			if (IsKey_PushedNow(DX_INPUT_KEY_PAD1, 4) == TRUE) {
				scene = TITLE;
			}
		}
		return 0;
	}

	//------チュートリアルシーン-----
	int TutorialScene(void) {
		DrawStringToHandle(4, 4, "チュートリアル", WHITE, Fonts[0]);
		DrawStringToHandle(10, 100, "飛んでくる弾を", WHITE, Fonts[0]);
		DrawStringToHandle(10, 150, "相手のフィールドに跳ね返そう!", WHITE, Fonts[0]);
		DrawStringToHandle(310, 600, "PRESS Z KEY", WHITE, Fonts[0]);
		if (IsKey_PushedNow(DX_INPUT_KEY_PAD1, 4) == TRUE) {			
			Team_Distribution();
			InitPlayers();
			SetStartPositions();
			ResetBallCreationState(); 
			ResetScore();
			scene = GAME;
			// カウントダウン開始 (初期化)
			countdown_timer = 3 * FPS;
			display_timer = 0;
			is_counting_down = true;
			//　ゲーム時間タイマー初期化
			timer = 0;
			ball_timer = 0;
			reverse_game_timer = TIME_LIMIT * FPS;
		}
		return 0;
	}

	int MainGameScene(void) {
		DrawFormatString(0, 0, WHITE, "%d", timer);
		DrawFormatString(0, 20, WHITE, "%d", count_balls);
		DrawTeamScore();
		DrawField();
		DrawPlayers();
		DispPlayer();
		//-----カウントダウン処理-----
		if (is_counting_down) {
			int seconds_left = (countdown_timer + FPS - 1) / FPS; // 残り秒数の切り上げ計算
			if (seconds_left > 0) {
				char disp[10];
				sprintf_s(disp, "%d", seconds_left);
				DrawStringToHandle(ScreenCenter_X - 50, ScreenCenter_Y - 60, disp, WHITE, Fonts[2]);
			}
			else {
				DrawStringToHandle(ScreenCenter_X - 50, ScreenCenter_Y - 60, "Go!", WHITE, Fonts[2]);
				display_timer++;
			}
			countdown_timer--;
			if (countdown_timer < 0 && display_timer == FPS) {
				is_counting_down = false;
			}
			return 0;
		}
		//-----------------------------

		timer++;
		reverse_game_timer--;
		
		
		CallCreateBalls();
		MoveBalls();
		CheckBallBounceWall();
		CountExistanceBalls();
		CheckPlayerBounce();
		CheckPoints();
		GameOver_Check();
		move();
		return 0;
	}

	int GameOverScene(void) {
		Check_Winner();
		DrawStringToHandle(ScreenCenter_X - 155, 100, "GAME OVER", WHITE, Fonts[2]);
		
		//　勝者表示
		if (is_draw) {
			DrawStringToHandle(ScreenCenter_X - 60, 220, "引き分け!", WHITE, Fonts[2]);
		}
		else {
			char buf[64];
			sprintf_s(buf, "チーム %d の勝利!", winner_team + 1);
			DrawStringToHandle(ScreenCenter_X - 120, 190, buf, WHITE, Fonts[2]);
		}
		
		DrawStringToHandle(ScreenCenter_X - 270, 400, "PRESS Z to TITLE", WHITE, Fonts[2]);
		DrawStringToHandle(ScreenCenter_X - 270, 500, "Xでゲーム選択に戻る", WHITE, Fonts[2]);
		if (IsKey_PushedNow(DX_INPUT_KEY_PAD1, 4) == TRUE) {
			scene = TITLE;
		}
		if (IsKey_PushedNow(DX_INPUT_KEY_PAD1, 5) == TRUE) {
			scene = TITLE;
			GAME_MODE = 21;
		}
		return 0;
	}

}

/*
☆メモ
ボール生成時の速度計算
ボールの速度は関数作成時 movespeed_x = 5, movespeed_y = -5としていた。
しかしランダムにしつつ、ボールの速度を一定にするために計算が必要
条件
1. フィールド内にボールを飛ばすためにmovespeed_yは負である必要がある
2. movespeed_xは正又は負の任意の値を取ることができるが、速さ維持のためには次の式が成り立つ必要がある
	sqrt(movespeed_x^2 + movespeed_y^2)

実際の値で条件2の式を計算すると
	sqrt(movespeed_x^2 + movespeed_y^2) = sqrt(50)	となる
平方して
	movespeed_x^2 + movespeed_y^2 = 50

movespeed_yは負の値なので、movespeed_y = -k　と置くと
	movespeed_x^2 + k^2 = 50
ここで、kは正の値となる。したがって、次のように表せる。
	movespeed_x^2 = 50 - k^2

具体的な範囲で
・kは 0 < K < sqrt(50) ≈ 7.07 の範囲で取ることができる
・それによりmovespeed_xの範囲は次のようになる
	-sqrt(50 - k^2) < movespeed_x < sqrt(50 - k^2) 
よって
・movespped_y： -7.07 < movespeed_y < 0
・modepeed_x： -sqrt(50 - k^2) < movespeed_x < sqrt(50 - k^2) (kは負の値)
となる
*/

/*
ボールのz-index(高度)移動の重力処理
ボール生成から5秒で戻って地面に到達(1.00)する処理を目的に設定
フレームレートは FPS = 60 => つまり、1フレーム当たりの時間 dt = 1 / 60 秒で積分を行う
目的の総飛行時間は
	T = 2 * V0 / g (上向き初速 V0 の絶対値と重力 g)
約5秒にするため、 V0 = 0.5, g = 0.2 ( 2 * 0.5 / 0.2 = 5s)
*/