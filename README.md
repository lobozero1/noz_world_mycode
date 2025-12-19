# noz_world_mycode
ゲーム開発サークルで作成したミニゲーム集、NOZWORLDに入れたミニゲームの内、自分の作成したものを外部公開可能にするため抜粋したもの。全てC言語(c++)で作成され、DXライブラリを使用している。

## game_Hi.cpp
避けゲー。右側からやってくるブロックを制限時間内でよけ続ける。
<img width="1360" height="768" alt="SUPER NOZUE KINGDOM    61FPS 2025_12_19 19_13_55" src="https://github.com/user-attachments/assets/52c26d24-4638-42d7-919a-8ec708694e7d" />

## game_Hi2.cpp
テニスゲームの発展。2人又は4人専用のゲームで、多数やってくるボールを相手フィールドに打ち返すことで得点となる。ボールにはそれぞれ高度が設定されており、地面に落ちるときのみ打ち返せる。バレーを上から見たようなイメージ。<img width="1360" height="768" alt="SUPER NOZUE KINGDOM    61FPS 2025_12_19 19_12_46" src="https://github.com/user-attachments/assets/6d7b5c9b-c667-41b6-ac3a-54f0b5c717ad" />

各ゲームはメインから呼び出され実行する形式となっている。どちらも最大4人の同時プレイ可能かつコントローラー対応としている。
