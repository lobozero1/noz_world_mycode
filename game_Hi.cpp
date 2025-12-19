#include "DxLib.h"
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <cstdio>


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

// メインゲームループ
extern int GAME_MODE;

namespace HiGame {

    //  画面サイズ、FPS、各種定数
    const int FPS = 60;
    const int ITEM_MAX = 10;
    const int BLOCK_MAX_HIGH = 40; //配列の要素数
    const int BLOCK_MAX_LOW = 40;
    const int BLOCK_MAX_SIN = 40;
    const int MESSAGE_MAX = 10;
    const int WHITE = GetColor(255, 255, 255); // よく使う色(白)を定義

    //  プレイヤー初期位置・速度
    int hi_player_num = 0;
    int circle1_x = 111;
    int circle1_y = 222;
    const int default_speed = 4; // スピードの初期値を保存

    //  スコア、ライフ、タイマー等の管理変数
    int Score = 0;
    int life = 3;
    int timer = 0; // ゲーム全体の経過時間を数える変数
    int countdown_timer = 0; //カウントダウン用カウンタ
    int display_timer = 0; // メッセージ描画時間延長用タイマー
    int title_timer_hi = 0; //タイトル画面の点滅文字用タイマー
    int block_timer = 0; //ブロック生成用タイマー
    int noDamage = 0; // 無敵時間のカウント
    int item_speed_up_buff_time = 0; // スピードアップアイテム効果時間のカウント
    int item_bounce_buff_time = 0; // バウンドアイテム効果時間のカウント
    bool is_counting_down = false; //カウントダウン中フラグ

    //  色の定義
    int block_color = GetColor(238, 47, 115); // ブロックの色(ピンク)

    // アイテムの最大効果時間の管理(バフインジケーター用)
    const int SPEED_UP_ITEM_MAX_BUFF_TIME = 60; // スピードアップアイテムの効果時間
    const int BOUNCE_ITEM_MAX_BUFF_TIME = 300; // バウンドアイテムの効果時間

    // シーンごとの関数宣言
    int TitleScene(void);
    int TutorialScene(void);
    int MainGameScene(void);
    int GameOverScene(void);

    //シーン定義
    enum Scene {
        TITLE,
        TUTORIAL,
        GAME,
        GAME_OVER,
    };
    int scene = TITLE;

    // アイテムの種類の定義
    enum ItemType {
        ITEM_SPEED_UP,
        ITEM_HEAL,
        ITEM_BOUNCE,
    };

    //==============================
    // 2. 構造体定義とオブジェクト配列
    //==============================

    struct Player {
        int x, y;
        int speed;
        int score;
        int life;
        int item_speed_up_buff_time;
        int item_bounce_buff_time;
        int noDamage;
        bool state;
        float vx; // アイテムバウンス用のx,y方向の速度
        float vy;
    };

    Player player[4];
    int playercolors[4] = { GetColor(95, 204, 235), GetColor(243, 236, 97), GetColor(251, 171, 76), GetColor(188, 216, 112) };

    struct OBJECT_HIGH {
        int state;
        int x;
        int y;
        int movespeed_x;
        int movespeed_y;
    };

    struct OBJECT_LOW {
        int state;
        int x;
        int y;
        int movespeed_x;
        int movespeed_y;
    };

    struct OBJECT_RANDOM {
        int state;
        int x;
        int y;
        int movespeed_x;
        int movespeed_y;
    };

    struct MESSAGE {
        int state;
        int x;
        int y;
        int movespeed_x;
        int playerIndex;
    };

    struct ITEM {
        int state;
        int x, y;
        int buff_time;
        int type;
    };

    //block配列の定義
    OBJECT_HIGH block_high[BLOCK_MAX_HIGH];
    OBJECT_LOW block_low[BLOCK_MAX_LOW];
    OBJECT_RANDOM block_sin_curve[BLOCK_MAX_SIN];
    MESSAGE message[MESSAGE_MAX];
    ITEM items[ITEM_MAX];

    //==============================
    // 3.関数群
    //==============================

    //------------------------------
    // 3-1. プレイヤー操作
    //------------------------------

    //  プレイヤー数判定
    void Check_Players(void) {
        if (Num_JoyPads == 0) {
            Num_JoyPads = 1;
        }
        if (Num_JoyPads > 4) {
            Num_JoyPads = 4;
        }
        hi_player_num = Num_JoyPads;
    }

    // プレイヤーの初期化
    void InitPlayers(void) {
        for (int i = 0; i < Num_JoyPads; i++) {
            player[i].x = 100;
            player[i].y = 200 + i * 92;
            player[i].speed = 4;
            player[i].score = 0;
            player[i].life = 3;
            player[i].item_speed_up_buff_time = 0;
            player[i].item_bounce_buff_time = 0;
            player[i].noDamage = 0;
            player[i].state = 1; // プレイヤーの状態を初期化
        }
    }


    // プレイヤーの移動と状態更新
    void move(void) {
        //無敵判定
        for (int i = 0; i < Num_JoyPads; i++) {
            if (player[i].noDamage > 0) {
                player[i].noDamage--; //無敵時間のカウント
            }
            if (player[i].item_speed_up_buff_time > 0) {
                player[i].item_speed_up_buff_time--; //アイテム効果時間のカウント
                if (player[i].item_speed_up_buff_time == 0) {   // バフの効果を解除
                    player[i].speed = default_speed; // スピードバフを元に戻す
                }
            }
            if (player[i].item_bounce_buff_time > 0) {
                player[i].item_bounce_buff_time--;
            }

            // 移動処理(コントローラー入力は修正予定)
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
            if (player[i].x < 0) player[i].x = 0;
            else if (player[i].x > SCREEN_WIDTH) player[i].x = SCREEN_WIDTH;
            if (player[i].y < 0) player[i].y = 0;
            else if (player[i].y > SCREEN_HEIGHT) player[i].y = SCREEN_HEIGHT;
        }
    }

    // プレイヤーの描画
    void DrawPlayers(void) {
        for (int i = 0; i < Num_JoyPads; i++) {
            if (player[i].life < 0) {
                player[i].life = 0;
            }
            DrawFormatString(100 + i * 300, 10, WHITE, "P%d", i + 1);
            DrawFormatString(100 + i * 300, 45, WHITE, "SCORE: %d", player[i].score);
            DrawFormatString(100 + i * 300, 80, WHITE, "LIFE: %d", player[i].life);
            if (player[i].state == 0) {
                DrawLine(95 + i * 300, 5, 355 + i * 300, 115, GetColor(255, 0, 0));
                DrawLine(355 + i * 300, 5, 95 + i * 300, 115, GetColor(255, 0, 0));
                continue;
            }
            else if (player[i].noDamage > 0) {
                if (player[i].noDamage % 4 < 2) {
                    DrawCircle(player[i].x, player[i].y, 15, playercolors[i], TRUE);
                }
            }
            else {
                DrawCircle(player[i].x, player[i].y, 15, playercolors[i], TRUE);
            }
        }
    }

    //　脱落時のスライドメッセージ生成(初期化処理)
    void InitDropOutMessage(int p) {
        for (int i = 0; i < MESSAGE_MAX; i++) {
            if (message[i].state == 0) {
                message[i].state = 1;
                message[i].x = SCREEN_WIDTH;
                message[i].y = GetRand(SCREEN_HEIGHT);
                message[i].movespeed_x = 4;
                message[i].playerIndex = p;
                break;
            }
        }
    }

    //ゲームオーバーシーンに遷移したら、メッセージを存在しなくする
    void DeleteDropOutMessage(void) {
        for (int i = 0; i < MESSAGE_MAX; i++) {
            message[i].state = 0;
        }
    }

    void GameOver_Check(void) {
        for (int i = 0; i < Num_JoyPads; i++) {
            if (player[i].state == 1 && player[i].life <= 0) {
                player[i].state = 0;
                InitDropOutMessage(i);
                hi_player_num--;
            }
        }
        if (hi_player_num <= 0) {
            scene = GAME_OVER;
        }
    }

    // 現在アイテムバウンス専用関数(跳ね返り処理)
    float BouncePlayer_vx(void) {
        // ランダムな方向を生成
        float vx;
        float angle = static_cast<float>(rand()) / RAND_MAX * 2 * PI; // 0から2πの範囲の角度
        float bounceForce = 150.0f; // 跳ね返りの力（速度）

        vx = cos(angle) * bounceForce; // x方向の速度
        return vx;
    }

    float BouncePlayer_vy(void) {
        // ランダムな方向を生成
        float vy;
        float angle = static_cast<float>(rand()) / RAND_MAX * 2 * PI; // 0から2πの範囲の角度
        float bounceForce = 150.0f; // 跳ね返りの力（速度）

        vy = sin(angle) * bounceForce; // y方向の速度
        return vy;
    }

    // プレイヤー脱落時のメッセージ生成
    void Draw_DropOut_Message(void) {
        for (int i = 0; i < Num_JoyPads; i++) {
            if (message[i].state == 1) {
                DrawFormatString(message[i].x, message[i].y, WHITE, "プレイヤー P%d 脱落!", message[i].playerIndex + 1);
                message[i].x -= message[i].movespeed_x;
                if (message[i].x < -500) {
                    message[i].state = 0;
                }
            }
        }
    }

    int make_ranking(void) {
        int first_place;
        int first_place_score = 0;
        for (int i = 0; i < Num_JoyPads; i++) {
            if (player[i].score > first_place_score) {
                first_place = i;
                first_place_score = player[i].score;
            }
        }
        return first_place; //ゲーム終了時にスコアが最も高いプレイヤーの"配列番号"
    }

    // -----------------------------
    // 3-2. ブロック処理関連
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    // ブロックの長さを調整する
    void adjust_block_length(void) {
        for (int i = 0; i < BLOCK_MAX_HIGH; i++) {
            if (block_high[i].state == 0 || block_low[i].state == 0) continue;
            // 高さの合計が一定以下なら調整
            int combined_length = block_high[i].y + block_low[i].y;
            if (combined_length >= 600) {
                block_high[i].y -= 40; // 高いブロックを縮める
                block_low[i].y -= 40;  // 低いブロックも縮める
            }
        }
    }

    //ブロックを生成する(上半分、下半分、sinカーブ)
    void create_block(void) {
        //上のブロックの生成
        for (int i = 0; i < BLOCK_MAX_HIGH; i++) {
            if (block_high[i].state == 0) {
                block_high[i].x = SCREEN_WIDTH;
                block_high[i].y = 10 + GetRand(SCREEN_HEIGHT / 2);
                block_high[i].movespeed_x = -10;
                block_high[i].movespeed_y = 0;
                block_high[i].state = 1; //球を存在できる状態にする
                break;
            }
        }
        //下のブロックの生成
        for (int i = 0; i < BLOCK_MAX_LOW; i++) {
            if (block_low[i].state == 0) {
                block_low[i].x = SCREEN_WIDTH;
                block_low[i].y = 10 + GetRand(SCREEN_HEIGHT / 2);
                block_low[i].movespeed_x = -10;
                block_low[i].movespeed_y = 0;
                block_low[i].state = 1; //球を存在できる状態にする
                break;
            }
        }
        adjust_block_length(); // ブロックの長さを調整

        //sinカーブのブロックの生成
        for (int i = 0; i < BLOCK_MAX_SIN; i++) {
            if (block_sin_curve[i].state == 0) {
                block_sin_curve[i].x = SCREEN_WIDTH;
                block_sin_curve[i].y = GetRand(SCREEN_HEIGHT);
                block_sin_curve[i].movespeed_x = -20;
                block_sin_curve[i].movespeed_y += (int)(4 * sin(timer * 0.3));
                block_sin_curve[i].state = 1; //球を存在できる状態にする
                break;
            }
        }
    }

    //ブロックの移動処理
    void MoveBlocks(void) {
        // 上のブロックの移動処理
        for (int i = 0; i < BLOCK_MAX_HIGH; i++) {
            if (block_high[i].state == 0) continue; //空いている配列なら処理しない
            block_high[i].x += block_high[i].movespeed_x;
            block_high[i].y += block_high[i].movespeed_y;
            DrawBox(block_high[i].x, 0, block_high[i].x + 100, block_high[i].y, block_color, TRUE); //描画
            if (block_high[i].x + 100 < 0) {
                block_high[i].state = 0; //画面外に出たら、存在しない状態にする
            }
            for (int j = 0; j < Num_JoyPads; j++) { //当たり判定処理
                if (block_high[i].state == 1 && player[j].noDamage == 0 && player[j].x + 10 >= block_high[i].x && player[j].x - 10 <= block_high[i].x + 100 && player[j].y + 10 >= 0 && player[j].y - 10 <= block_high[i].y) {
                    player[j].life -= 1;
                    player[j].noDamage += FPS;
                }
            }
        }
        // 下のブロックの移動処理
        for (int i = 0; i < BLOCK_MAX_LOW; i++) {
            if (block_low[i].state == 0) continue; //空いている配列なら処理しない
            block_low[i].x += block_low[i].movespeed_x;
            block_low[i].y += block_low[i].movespeed_y;
            DrawBox(block_low[i].x, SCREEN_HEIGHT - block_low[i].y, block_low[i].x + 100, SCREEN_HEIGHT, block_color, TRUE);
            if (block_low[i].x + 100 < 0) {
                block_low[i].state = 0; //画面外に出たら、存在しない状態にする
            }
            for (int j = 0; j < Num_JoyPads; j++) { //当たり判定処理
                if (block_low[i].state == 1 && player[j].noDamage == 0 && player[j].x + 10 >= block_low[i].x && player[j].x - 10 <= block_low[i].x + 100 && player[j].y + 10 >= SCREEN_HEIGHT - block_low[i].y && player[j].y - 10 <= SCREEN_HEIGHT) {
                    player[j].life -= 1;
                    player[j].noDamage += FPS;
                }
            }
        }
        // sinカーブのブロックの移動処理
        for (int i = 0; i < BLOCK_MAX_SIN; i++) {
            if (block_sin_curve[i].state == 0) continue; //空いている配列なら処理しない
            block_sin_curve[i].x += block_sin_curve[i].movespeed_x;
            block_sin_curve[i].y += block_sin_curve[i].movespeed_y;
            DrawBox(block_sin_curve[i].x, block_sin_curve[i].y - 50, block_sin_curve[i].x + 50, block_sin_curve[i].y + 50, block_color, TRUE); //描画
            if (block_sin_curve[i].x + 50 < 0) {
                block_sin_curve[i].state = 0; //画面外に出たら、存在しない状態にする
            }
            for (int j = 0; j < Num_JoyPads; j++) { //当たり判定処理
                if (block_sin_curve[i].state == 1 && player[j].noDamage == 0 && player[j].x + 10 >= block_sin_curve[i].x && player[j].x - 10 <= block_sin_curve[i].x + 50 && player[j].y + 10 >= block_sin_curve[i].y - 50 && player[j].y - 10 <= block_sin_curve[i].y + 50) {
                    player[j].life -= 1;
                    player[j].noDamage += FPS;
                }
            }
        }
    }

    //-------------------------------
    // 3-3. アイテム処理関連
    //-------------------------------

    //アイテムの色を種類ごとに返す
    int GetItemColor(int type) {
        switch (type) {
        case ITEM_SPEED_UP: return GetColor(255, 0, 0); //赤(速度アップ)
        case ITEM_HEAL:     return GetColor(0, 0, 255); //青(ライフ回復)
        case ITEM_BOUNCE:   return GetColor(0, 255, 0); //緑(バウンド)
        default:            return GetColor(255, 255, 255);
        }
    }

    //アイテム取得時の効果の適用
    void ApplyItemEffect(Player& player, ITEM& item) {
        switch (item.type) {
        case ITEM_SPEED_UP:
            player.speed += 6; //スピードアップ 
            player.item_speed_up_buff_time += 60; //バフの効果持続時間
            break;
        case ITEM_HEAL:
            player.life += 1; //ライフ回復
            player.score += 100; //スコア加算
            break;
        case ITEM_BOUNCE:
            player.item_bounce_buff_time += 900; //バウンド効果の持続時間
            break;
            //今後のアイテム追加はここに追加
        }
    }

    //アイテムの生成
    void create_item(void) {
        for (int i = 0; i < ITEM_MAX; i++) {
            if (items[i].state == 0) {
                items[i].x = GetRand(SCREEN_WIDTH);
                items[i].y = 10 + GetRand(SCREEN_HEIGHT);
                items[i].type = GetRand(2); //アイテムの種類をランダムに決定
                items[i].state = 1; //アイテムを存在できる状態にする
                break;
            }
        }
    }

    //ゲームオーバー時にアイテムを初期化する
    void Init_Items(void) {
        for (int i = 0; i < ITEM_MAX; i++) {
            items[i].state = 0; //アイテムを存在しない状態にする
        }
    }

    //アイテムの描画
    void DrawItems(void) {
        for (int i = 0; i < ITEM_MAX; i++) {
            if (items[i].state == 1) {
                int color = GetItemColor(items[i].type);
                DrawCircle(items[i].x, items[i].y, 10, color, TRUE);
            }
        }
    }

    //アイテムとプレイヤーの当たり判定
    void CheckItemCollision(void) {
        for (int i = 0; i < ITEM_MAX; i++) {
            if (items[i].state == 1) {
                for (int j = 0; j < Num_JoyPads; j++) {
                    if (player[j].x + 10 >= items[i].x - 10 && player[j].x - 10 <= items[i].x + 10 && player[j].y + 10 >= items[i].y - 10 && player[j].y - 10 <= items[i].y + 10) {
                        ApplyItemEffect(player[j], items[i]);
                        items[i].state = 0; //アイテムを消す
                    }
                }
            }
        }
    }

    //スコア増加とアイテム取得時のスコア増加倍率の調整
    void Increment_player_Score(void) {
        for (int i = 0; i < Num_JoyPads; i++) {
            if (player[i].state == 0) continue; //プレイヤーが存在しない場合はスコアを加算しない
            player[i].score++;
            if (player[i].item_speed_up_buff_time > 0) {
                player[i].score += 1.3; //speedアイテム取得時のスコア増加倍率
            }
        }
    }

    void Item_bounce(void) {
        for (int i = 0; i < Num_JoyPads; i++) {
            for (int j = 0; j < Num_JoyPads; j++) {
                if (player[i].item_bounce_buff_time > 0 && player[j].item_bounce_buff_time && player[i].state == 1 && player[j].state == 1 && i != j && player[i].x + 20 >= player[j].x - 20 && player[i].x - 20 <= player[j].x + 20 && player[i].y + 20 >= player[j].y - 20 && player[i].y - 20 <= player[j].y + 20) {
                    player[i].x += BouncePlayer_vx();
                    player[i].y += BouncePlayer_vy();
                    player[i].noDamage = FPS / 2;
                    player[j].x += BouncePlayer_vx();
                    player[j].y += BouncePlayer_vy();
                    player[j].noDamage = FPS / 2; //連続の効果発動防止
                }
                if (player[i].item_bounce_buff_time > 0 && player[i].state == 1 && player[j].state == 1 && i != j && player[i].x + 20 >= player[j].x - 10 && player[i].x - 20 <= player[j].x + 10 && player[i].y + 20 >= player[j].y - 10 && player[i].y - 20 <= player[j].y + 10) {
                    player[j].x += BouncePlayer_vx();
                    player[j].y += BouncePlayer_vy();
                    player[j].noDamage = FPS / 2; //連続の効果発動防止
                }
            }
        }
    }

    //-------------------------------
    // 3-4 アイテム効果表示
    //-------------------------------

    //バフの残り時間を示す円弧を描画する
    void DrawBuffIndicator_circle(int centerX, int centerY, int radius, int item_speed_up_buff_time, int max_buff_time) {
        if (item_speed_up_buff_time <= 0) return; //バフ無効状態なら行わない
        double ratio = (double)item_speed_up_buff_time / (double)max_buff_time; //残り時間の割合
        double totalAngle = ratio * 360.0; //残り時間に対応する角度
        int segments = 100; //円弧の滑らかさの決定(大きいほど滑らか)
        double step = totalAngle / (double)segments; //線文間の角度差
        //線分の描画
        for (int i = 0; i < segments; i++) {
            double starAngle = i * step;
            double endAngle = (i + 1) * step;
            int x1 = centerX + (int)(radius * cos(starAngle * PI / 180.0));
            int y1 = centerY + (int)(radius * sin(starAngle * PI / 180.0));
            int x2 = centerX + (int)(radius * cos(endAngle * PI / 180.0));
            int y2 = centerY + (int)(radius * sin(endAngle * PI / 180.0));
            DrawLine(x1, y1, x2, y2, GetColor(255, 255, 0));
        }
    }

    // アイテムバウンスの残り時間と当たり判定を表示
    void ItemBounceField(int centerX, int centerY, int item_bounce_buff_time) {
        if (item_bounce_buff_time <= 0) return;
        if (item_bounce_buff_time <= 180) {
            if (item_bounce_buff_time % 30 < 15) {
                DrawCircle(centerX, centerY, 20, GetColor(167, 87, 168), FALSE);
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 128);
                DrawCircle(centerX, centerY, 20, GetColor(228, 204, 228), TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }
        }
        DrawCircle(centerX, centerY, 20, GetColor(167, 87, 168), FALSE);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 128);
        DrawCircle(centerX, centerY, 20, GetColor(228, 204, 228), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    //-------------------------------
    // 3-5. シーン管理関数
    //-------------------------------

    void GameScene(void) {
        switch (scene) {
        case TITLE:
            TitleScene();
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

    //==============================
    // 4.シーン管理(各画面の処理)
    //==============================

    //-----タイトルシーン-----
    int TitleScene(void) {
        const int blink_interval = 30; //点滅間隔
        bool string_view = true; //テキストの表示状態
        Score = 0;
        title_timer_hi++;
        DrawStringToHandle(ScreenCenter_X - 150, ScreenCenter_Y - 200, "タイトル", WHITE, Fonts[2]);
        Check_Players();
        DrawFormatString(10, 10, WHITE, "Connected Players: %d", Num_JoyPads);
        SetFontSize(40);
        if (title_timer_hi >= blink_interval) {
            DrawStringToHandle(ScreenCenter_X - 250, 380, "PRESS Z KEY", WHITE, Fonts[0]);
            if (title_timer_hi >= 60) {
                title_timer_hi = 0; //カウントリセット
            }
        }
        if (IsKey_PushedNow(DX_INPUT_KEY_PAD1, 4) == TRUE) {
            scene = TUTORIAL;
        }
        return 0;
    }

    //-----チュートリアルシーン-----
    int TutorialScene(void) {
        DrawStringToHandle(4, 4, "チュートリアル", WHITE, Fonts[0]);
        DrawStringToHandle(10, 100, "ブロックを避けつつアイテムを取って", WHITE, Fonts[1]);
        DrawStringToHandle(10, 150, "高得点を目指そう!", WHITE, Fonts[1]);
        DrawStringToHandle(310, 400, "PRESS Z KEY", WHITE, Fonts[1]);

        if (IsKey_PushedNow(DX_INPUT_KEY_PAD1, 4) == TRUE) {
            Check_Players();
            InitPlayers();
            scene = GAME;
            //　カウントダウン開始　(3秒 * FPSフレーム)
            countdown_timer = 3 * FPS;
            is_counting_down = true;
            //初期化処理
            life = 3;
            noDamage = 0;
            timer = 0;
            block_timer = 0;
            memset(block_high, 0, sizeof(block_high));
            memset(block_low, 0, sizeof(block_low));
            memset(block_sin_curve, 0, sizeof(block_sin_curve));
        }
        return 0;
    }

    //-----ゲームシーン-----
    int MainGameScene(void) {
        // 画面描画
        DrawFormatString(0, 0, WHITE, "%d", timer);
        DrawFormatString(0, 30, WHITE, "%d", block_timer);
        DrawPlayers();

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

        timer++; // 経過時間のカウント


        //game_state_countdown();

        move();
        Draw_DropOut_Message();

        Item_bounce();

        for (int i = 0; i < Num_JoyPads; i++) {
            if (player[i].item_speed_up_buff_time > 0) {
                DrawBuffIndicator_circle(player[i].x, player[i].y, 18, player[i].item_speed_up_buff_time, SPEED_UP_ITEM_MAX_BUFF_TIME);
            }
            if (player[i].item_bounce_buff_time > 0) {
                ItemBounceField(player[i].x, player[i].y, player[i].item_bounce_buff_time);
            }
        }

        Increment_player_Score();

        GameOver_Check();

        block_timer++;
        if (block_timer >= 50) {
            create_block();
            block_timer = 0; //タイマーリセット
        }

        Score++;

        if (timer % 300 == 0) {
            create_item();
        }

        DrawItems();
        CheckItemCollision();
        MoveBlocks();
        return 0;
    }

    //-----ゲームオーバーシーン-----
    int GameOverScene(void) {
        DrawStringToHandle(ScreenCenter_X - 155, 100, "GAME OVER", WHITE, Fonts[2]);
        DrawStringToHandle(ScreenCenter_X - 270, 400, "PRESS Z to TITLE", WHITE, Fonts[2]);
        DrawStringToHandle(ScreenCenter_X - 270, 500, "Xでゲーム選択に戻る", WHITE, Fonts[2]);
        int first_place = make_ranking(); //ゲーム終了時にスコアが最も高いプレイヤーの配列番号(0から始まる)
        Init_Items(); // アイテムを初期化
        DeleteDropOutMessage();
        DrawFormatString(ScreenCenter_X - 170, 300, WHITE, "1位 プレイヤー %d SCORE %d", first_place + 1, player[first_place].score);
        if (IsKey_PushedNow(DX_INPUT_KEY_PAD1, 4) == TRUE) {
            scene = TITLE;
        }
        if (IsKey_PushedNow(DX_INPUT_KEY_PAD1, 5) == TRUE) {
            scene = TITLE;
            GAME_MODE = 21; //メインのゲーム選択画面に戻る
        }
        return 0;
    }
}


//アイテム種類の追加（調整）