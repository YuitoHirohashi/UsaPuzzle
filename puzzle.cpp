#include "DxLib\DxLib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#define VSIZE 6 //縦のサイズ
#define HSIZE 8 //横のサイズ
#define TIME 60 //制限時間    
#define LIFE 500 //体力  
#define LIFE_NUM 5 //体力の最大数    
#define SKILL1 30 //スキル1発動条件
#define SKILL2 20 //スキル2発動条件
#define SKILL3 20 //スキル3発動条件
#define S 250000 //S評価
#define A 200000 //A評価
#define B 150000 //B評価
#define C 120000 //C評価

int life = LIFE; //ウサギの体力
int life_num = LIFE_NUM; //画面乗に表示されている体力の数

int Key[256]; // キーが押されているフレーム数を格納する

// キーの入力状態を更新する
int gpUpdateKey(){
	char tmpKey[256]; // 現在のキーの入力状態を格納する
	GetHitKeyStateAll( tmpKey ); // 全てのキーの入力状態を得る
	for( int i=0; i<256; i++ ){ 
		if( tmpKey[i] != 0 ){ // i番のキーコードに対応するキーが押されていたら
			Key[i]++;     // 加算
		} else {              // 押されていなければ
			Key[i] = 0;   // 0にする
		}
	}
	return 0;
}

double gettime(void); //時間の取得
void Usa(int &usax, int &usay, int usa, int life_pic[]); //ウサギ
void Life(int life_pic[]); //体力を減少させたり、体力の描画を行う関数
void Delete(int flag[][HSIZE], int field[][HSIZE]); //ブロックを削除する関数
void Select(int field[][HSIZE], int flag[][HSIZE], int usax, int usay, int *phold, int *hold,  int *score, int *skill1, int *skill2, int *skill3, double time, int *time_ex, double *rate, int *level, int select_se, int skill1_se, int skill2_se, int skill3_se); //ブロックを消したり落下させたりする関数
void Display(int field[][HSIZE], int flag[][HSIZE], int Image[], int select); //盤面を表示する関数
void Result(int score, int clear, int s, int a, int b, int c, int d); //結果の表示
void Restart(double *time, double *start, double *end, int *score, int *skill1, int *skill2, int *skill3, int *time_ex, double *rate, int *level, int field[][HSIZE], int flag[][HSIZE], int bgm); //ゲームの状態を初期状態に戻す関数
int Record(int score, int hiscore); //ハイスコアを記録する関数

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow){
    ChangeWindowMode(TRUE), DxLib_Init(), SetDrawScreen( DX_SCREEN_BACK ); //ウィンドウモード変更と初期化と裏画面設定

    srand((unsigned int)time(NULL));

    int field[VSIZE][HSIZE], //フィールド (0、1、2 の情報を格納)
        flag[VSIZE][HSIZE], //フラグが立ってる場所
        usax, usay, //ウサギの座標系 (x座標、y座標)
        phold = -1, //最初に選択したブロック
        hold, //次に選択したブロック
        score = 0, //得点
        hiscore = 0, //ハイスコア
        level = 1, //上昇率のレベル(スキルレベル)
        skill1 = 0, //スキルゲージ1
        skill2 = 0, //スキルゲージ2
        skill3 = 0, //スキルゲージ3
        time_ex = 0; //時間延長

    double start, end, time = 0;
    double rate = 1.2; //スコア上昇率

    /*画像関係*/
    int Image[3], //0、1、2 の数値に対応する画像を格納
        //0 -> 白ブロック 、1 -> 人参ブロック　、2 -> 草ブロック
        life_pic[2], //体力に対応する画像を格納
        select = LoadGraph("src/select.png"), //選択時の画像の読みこみ
        usa = LoadGraph("src/usa.png"), //ウサギの画像の読み込み
        clear = LoadGraph("src/clear.png"), //クリア画面の読みこみ
        gameover = LoadGraph("src/gameover.png"), //死亡画面の表示
        s = LoadGraph("src/eval_S.png"), //S評価画像  
        a = LoadGraph("src/eval_A.png"), //A評価画像  
        b = LoadGraph("src/eval_B.png"), //B評価画像  
        c = LoadGraph("src/eval_C.png"), //C評価画像  
        d = LoadGraph("src/eval_D.png"); //D評価画像  
        
        LoadDivGraph("src/map.png", 3, 3, 1, 32, 32, Image); //ブロック画像の読み込み
        LoadDivGraph("src/life.png", 2, 2, 1, 32, 32, life_pic); //体力画像の読み込み

    /*音声関係*/
    int bgm = LoadSoundMem("src/famipop3.mp3"), //BGMの読み込み
        select_se = LoadSoundMem("src/select.mp3"), //選択SEの読み込み
        skill1_se =LoadSoundMem("src/skill1_se.mp3"), //スキル1SEの読み込み
        skill2_se =LoadSoundMem("src/skill2_se.mp3"), //スキル2SEの読み込み
        skill3_se =LoadSoundMem("src/skill3_se.mp3"); //スキル3SEの読み込み

        ChangeVolumeSoundMem(80, bgm); //BGMの音量の調製
        ChangeVolumeSoundMem(100, select_se); //SEの音量の調製 
        ChangeVolumeSoundMem(100, skill1_se); //SEの音量の調製 
        ChangeVolumeSoundMem(100, skill2_se); //SEの音量の調製 
        ChangeVolumeSoundMem(100, skill3_se); //SEの音量の調製 

    for(int i = 0; i < VSIZE; i++){
        for(int j = 0; j < HSIZE; j++){
            field[i][j] = rand() % 3; //フィールドを適当に初期化
            flag[i][j] = 0; //フラグ配列を0で初期化
        }
    }

    start = end = gettime(); //基準となる時間の取得
    PlaySoundMem(bgm, DX_PLAYTYPE_LOOP); //BGMの再生

    // while(裏画面を表画面に反映, メッセージ処理, 画面クリア, キーの入力状態を更新)
    while( ScreenFlip()==0 && ProcessMessage()==0 && ClearDrawScreen()==0 && gpUpdateKey()==0){
        Display(field, flag, Image, select); 
        DrawFormatString( 450, 410, GetColor( 0, 255, 0), "SCORE : %d", score);
        
        if(TIME - time > 0 && life > 0){
            time = end - start; //経過時刻の更新
            Life(life_pic); //体力の表示や操作
            DrawFormatString( 280, 430, GetColor( 255, 0, 0), "LIFE : %d", life);
            DrawFormatString( 450, 450, GetColor( 0, 255, 0), "TIME : %.2f sec", TIME - time); //残り時間            

        }
        else if ((TIME - time <= 0 || life == 0) && Key[KEY_INPUT_SPACE] != 0){ //時間終了の状態でスペースキーが入力されたらリスタート
            Restart(&time, &start, &end,&score, &skill1, &skill2, &skill3, &time_ex, &rate, &level, field, flag, bgm); 
        }
        else if (TIME - time > 0 && life == 0){
            DrawRotaGraph(300, 200, 0.6, 0.0, gameover, FALSE); //ゲームオーバー画面の表示
            DrawFormatString( 280, 430, GetColor( 450, 450, 0), "GAME OVER!");
            StopSoundMem(bgm);
        }
        else{ //時間終了
            DrawFormatString( 450, 450, GetColor( 0, 255, 0), "TIME UP!"); //時間切れ
            Result(score, clear, s, a, b, c, d); //最終結果の表示
            hiscore = Record(score, hiscore); //ハイスコアの更新
            StopSoundMem(bgm);
        }

        Usa(usax, usay, usa, life_pic); //ウサギの操作
        Select(field, flag, usax, usay, &phold, &hold, &score, &skill1, &skill2, &skill3, time, &time_ex, &rate, &level, select_se, skill1_se, skill2_se, skill3_se); //消すブロックの選択
        end = gettime(); //時刻の更新
        if(time_ex > 0){
            end -= 5*time_ex; //スキル3を発動する度に時間を5秒ずつ延長
        }

    }

    DeleteSoundMem(bgm); //読み込んだBGMの削除
    DeleteSoundMem(select_se); //読み込んだSEの削除
    DeleteSoundMem(skill1_se); //読み込んだSEの削除
    DeleteSoundMem(skill2_se); //読み込んだSEの削除
    DeleteSoundMem(skill3_se); //読み込んだSEの削除
    DxLib_End(); // DXライブラリ終了処理
    return 0;
}

double gettime(void){ //時間の取得
    struct timeval tp;
    double ret;
    gettimeofday(&tp, NULL); 
    ret = (double)(tp.tv_sec & 0x00ffffff) + (double)tp.tv_usec / 1000000;
    return ret;
}

void Usa(int &usax, int &usay, int usa, int life_pic[]){
    GetMousePoint(&usax, &usay);
    
    if(life > 0){ //体力が0より大きいときはウサギを表示
        if(usay <= 50 && usax <= 100) DrawRotaGraph(100, 50, 2, 0.0, usa, TRUE);
        else if(usay >= 64*(VSIZE - 1) + 50 && usax <= 100) DrawRotaGraph(100, 64*(VSIZE - 1) + 50, 2, 0.0, usa, TRUE);
        else if(usax >=  64*(HSIZE - 1) + 100 && usay <= 50) DrawRotaGraph(64*(HSIZE - 1) + 100, 50, 2, 0.0, usa, TRUE);
        else if(usax >= 64*(HSIZE - 1) + 100 && usay >= 64*(VSIZE - 1) + 50) DrawRotaGraph(64*(HSIZE - 1) + 100, 64*(VSIZE - 1) + 50, 2, 0.0, usa, TRUE);
        else if(usay <= 50) DrawRotaGraph(usax, 50, 2, 0.0, usa, TRUE);
        else if(usax <= 100) DrawRotaGraph(100, usay, 2, 0.0, usa, TRUE);
        else if(usay >= 64*(VSIZE - 1) + 50) DrawRotaGraph(usax, 64*(VSIZE - 1) + 50, 2, 0.0, usa, TRUE);
        else if(usax >= 64*(HSIZE - 1) + 100) DrawRotaGraph(64*(HSIZE - 1) + 100, usay, 2.0, 0.0, usa, TRUE); 
        else DrawRotaGraph(usax, usay, 2, 0.0, usa, TRUE);
    }
    else{ //体力がなくなったときは"X"を表示
        if(usay <= 50 && usax <= 100) DrawRotaGraph(100, 50, 2, 0.0, life_pic[1], TRUE);
        else if(usay >= 64*(VSIZE - 1) + 50 && usax <= 100) DrawRotaGraph(100, 64*(VSIZE - 1) + 50, 2, 0.0, life_pic[1], TRUE);
        else if(usax >=  64*(HSIZE - 1) + 100 && usay <= 50) DrawRotaGraph(64*(HSIZE - 1) + 100, 50, 2, 0.0, life_pic[1], TRUE);
        else if(usax >= 64*(HSIZE - 1) + 100 && usay >= 64*(VSIZE - 1) + 50) DrawRotaGraph(64*(HSIZE - 1) + 100, 64*(VSIZE - 1) + 50, 2, 0.0, life_pic[1], TRUE);
        else if(usay <= 50) DrawRotaGraph(usax, 50, 2, 0.0, life_pic[1], TRUE);
        else if(usax <= 100) DrawRotaGraph(100, usay, 2, 0.0, life_pic[1], TRUE);
        else if(usay >= 64*(VSIZE - 1) + 50) DrawRotaGraph(usax, 64*(VSIZE - 1) + 50, 2, 0.0, life_pic[1], TRUE);
        else if(usax >= 64*(HSIZE - 1) + 100) DrawRotaGraph(64*(HSIZE - 1) + 100, usay, 2.0, 0.0, life_pic[1], TRUE); 
        else DrawRotaGraph(usax, usay, 2, 0.0, life_pic[1], TRUE);
    }   
}

void Life(int life_pic[]){ //体力に関する関数

    if(life > 0)
        life--;
    
    if(life % 100 == 0)
        life_num--;

    for(int i = 0; i < LIFE_NUM; i++){
        if(i < LIFE_NUM - life_num)
            DrawRotaGraph(36, 64*i + 50, 2, 0.0, life_pic[1], FALSE);
        else
            DrawRotaGraph(36, 64*i + 50, 2, 0.0, life_pic[0], FALSE);
    }
}

void Delete(int flag[][HSIZE], int field[][HSIZE]){ //選択したブロックの削除、ブロックの落下
    int tmp[VSIZE];
    int used, count;

    for(int j = 0; j < HSIZE; j++){

        for(int k = 0; k < VSIZE; k++) tmp[k] = -1;
        used = 0;
        count = 0;

        for(int i = VSIZE - 1; i >= 0; i--){
            if(flag[i][j] == 0) tmp[used++] = field[i][j];
        }

        for(int i = VSIZE - 1; i >= 0; i--){
            if(count < used) field[i][j] = tmp[count++];
            else field[i][j] = rand() % 3;
            flag[i][j] = 0;
        }
    }

}

void Select(int field[][HSIZE], int flag[][HSIZE], int usax, int usay, int *phold, int *hold,  int *score, int *skill1, int *skill2, int *skill3, double time, int *time_ex, double *rate, int *level, int select_se, int skill1_se, int skill2_se, int skill3_se){
    int Mouse = GetMouseInput(); //マウス操作を有効にする
    int x, y, flag_num = 0;

    if(Mouse & MOUSE_INPUT_LEFT){
        if(usay <= 50 && usax <= 100){
            x = 50/64;
            y = (100 - 50)/64;
        } 
        else if(usay >= 64*(VSIZE - 1) + 50 && usax <= 100){
            x = (64*(VSIZE - 1) + 50)/64;
            y = (100 - 50)/64;
        } 
        else if(usax >=  64*(HSIZE - 1) + 100 && usay <= 50){
            x = 50/64;
            y = ((64*(HSIZE - 1) + 100) - 50)/64;
        }
        else if(usax >= 64*(HSIZE - 1) + 100 && usay >= 64*(VSIZE - 1) + 50){
            x = (64*(VSIZE - 1) + 50)/64;
            y = ((64*(HSIZE - 1) + 100) - 50)/64;
        }
        else if(usay <= 50){
            x = 50 / 64;
            y = (usax - 50)/64; 
        } 
        else if(usax <= 100){
            x = usay/64;
            y = (100 - 50)/64;
        } 
        else if(usay >= 64*(VSIZE - 1) + 50){
            x = (64*(VSIZE - 1) + 50)/64;
            y = (usax - 50)/64;
        } 
        else if(usax >= 64*(HSIZE - 1) + 100){
            x = usay/64;
            y = ((64*(HSIZE - 1) + 100) - 50)/64;
        }         
        else{
            x = usay/64;
            y = (usax - 50)/64;                       
        }        

        flag[x][y] = 1; //選択したブロックの場所にフラグを立てる
        PlaySoundMem(select_se, DX_PLAYTYPE_BACK); //SEの再生 

        if(*phold == -1) *phold = field[x][y]; //まだ選択されてるブロックがないときは、最初に選んだブロックをpholdに格納する
        else *phold = *hold; //ひとつ前に選択されたブロックをpholdに格納する
            
        *hold = field[x][y]; //選択したブロックをholdに格納する

        if(*phold != *hold){
        for(int i = 0; i < VSIZE; i++){
            for(int j = 0; j < HSIZE; j++){
                flag[i][j] = 0; //途中で違うブロックを選択した場合、全てのフラグを取り除く
            }
        }
        *phold = -1;
        }

    }

    for(int i = 0; i < VSIZE; i++){
        for(int j = 0; j < HSIZE; j++){
            if(flag[i][j] == 1) flag_num++; //フラグの本数
        }
    }

    DrawFormatString( 450, 430, GetColor( 255, 0, 0), "CHAIN : %d", flag_num); //連続で何本のフラグを立てたか

    if(!(Mouse & MOUSE_INPUT_LEFT)){
        if(flag_num>0){
            Delete(flag, field);
            if(*hold == 1){ //人参ブロックの選択
                if(TIME - time > 0 && life > 0){
                    *score += 200 * pow(flag_num,*rate);
                    life= LIFE; //人参を消したら体力全回復
                    life_num = LIFE_NUM; //人参を消したら画面上に表示されている体力を全回復
                }
                (*skill1) += flag_num;
                if(*skill1 > SKILL1) *skill1 = SKILL1;
            }
            if(*hold == 0){ //白ブロックの選択
                if(TIME - time > 0 && life > 0) *score += 100 * pow(flag_num,*rate);
                (*skill2) += flag_num;
                if(*skill2 > SKILL2) *skill2 = SKILL2;
            }
            if(*hold == 2){ //草ブロックの選択
                if(TIME - time > 0 && life > 0) *score += 100 * pow(flag_num,*rate);
                (*skill3) += flag_num;
                if(*skill3 > SKILL3) *skill3 = SKILL3;
            }
            *phold = -1;
        }
        //if(TIME - time > 0 && life > 0)
            //printf("%d\n",*score); /*スコアの集計*/
    }

    DrawFormatString( 100, 410, GetColor( 0, 255, 255), "SKILL1 : %d / %d", *skill1, SKILL1); //人参を消したときに溜まるゲージ
    DrawFormatString( 100, 430, GetColor( 155, 0, 255), "SKILL2 : %d / %d", *skill2, SKILL2); //白ブロックを消したときに溜まるゲージ
    DrawFormatString( 100, 450, GetColor( 255, 255, 0), "SKILL3 : %d / %d", *skill3, SKILL3); //草ブロックを消したときに溜まるゲージ
    DrawFormatString( 280, 410, GetColor( 255, 0, 0), "LEVEL : %d", *level); //スキルレベル

    if(Mouse & MOUSE_INPUT_RIGHT){
        if(*skill1 == SKILL1){ //スキルゲージが30のとき、右クリックでスキル発動
            for(int i = 0; i < VSIZE; i++){
                for(int j = 0; j < HSIZE; j++){
                    field[i][j] = 2; //ブロックをすべて草ブロックにする
                }
            }
            if(life > 0){
                life = LIFE; //体力全回復
                life_num = LIFE_NUM; //体力全回復
            }
                
            *skill1 = 0; //スキルゲージを0に戻す。
            PlaySoundMem(skill1_se, DX_PLAYTYPE_BACK); //SEの再生 
        }
        if(*skill2 == SKILL2){
            *rate += 0.2;
            (*level)++;
            *skill2 = 0;
            PlaySoundMem(skill2_se, DX_PLAYTYPE_BACK); //SEの再生
        }
        if(*skill3 == SKILL3){
            (*time_ex)++;
            *skill3 = 0;
            PlaySoundMem(skill3_se, DX_PLAYTYPE_BACK); //SEの再生
        }
    }

}

void Display(int field[][HSIZE], int flag[][HSIZE], int Image[], int select){ //フィールドを表示する関数
    for(int i = 0; i < VSIZE; i++){
        for(int j = 0; j < HSIZE; j++){
            DrawRotaGraph(64*j + 100, 64*i + 50, 2, 0.0, Image[field[i][j]], FALSE);
            if(flag[i][j] == 1) DrawRotaGraph(64*j + 100, 64*i + 50, 2, 0.0, select, TRUE);
        }
    }
}

void Result(int score, int clear, int s, int a, int b, int c, int d){
    DrawRotaGraph(300, 200, 0.6, 0.0, clear, FALSE); //クリア画面の表示
    if(score >= S) DrawRotaGraph(410, 180, 0.07, 0.0, s, FALSE); //S評価
    else if(score >= A && score < S) DrawRotaGraph(410, 180, 0.07, 0.0, a, FALSE); //A評価
    else if(score >= B && score < A) DrawRotaGraph(410, 180, 0.07, 0.0, b, FALSE); //B評価
    else if(score >= C && score < B) DrawRotaGraph(410, 180, 0.07, 0.0, c, FALSE); //C評価
    else DrawRotaGraph(410, 180, 0.07, 0.0, d, FALSE); //D評価
}

void Restart(double *time, double *start, double *end, int *score, int *skill1, int *skill2, int *skill3, int *time_ex, double *rate, int *level, int field[][HSIZE], int flag[][HSIZE], int bgm){
    *time = 0;
    *start = *end = gettime();
    *score = 0;
    *skill1 = 0;
    *skill2 = 0;
    *skill3 = 0;
    *time_ex = 0;
    *rate = 1.2;
    *level = 1;
    life = LIFE;
    life_num = LIFE_NUM;

    for(int i = 0; i < VSIZE; i++){
        for(int j = 0; j < HSIZE; j++){
            field[i][j] = rand() % 3; //フィールドを適当に初期化
            flag[i][j] = 0; //フラグ配列を0で初期化
        }
    }
    PlaySoundMem(bgm, DX_PLAYTYPE_LOOP); //BGMの再生
}

int Record(int score, int hiscore){ //スコアの更新、表示

    if(score >= hiscore){
        DrawFormatString( 380, 250, GetColor( 255, 255, 0), "NEW RECORD!");
        DrawFormatString( 380, 270, GetColor( 0, 255, 0), "SCORE : %d", score);
        DrawFormatString( 380, 290, GetColor(255, 102, 0), "HIGH SCORE : %d", hiscore);
        return score;
    }
    
    else{
        DrawFormatString( 380, 270, GetColor( 0, 255, 0), "SCORE : %d", score);
        DrawFormatString( 380, 290, GetColor(255, 102, 0), "HIGH SCORE : %d", hiscore);
        return hiscore;
    }
    
}