#pragma once
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <bit>
#include "RGB565.hh"
#include "qrcode.h"
#include "FullTextLineRenderer.hh"
#define TAG "QR"
#include <esp_log.h>

namespace BREAKOUT
{
    class Vector2
    {
    public:
        int16_t x;
        int16_t y;
        Vector2(int16_t x, int16_t y) : x(x), y(y) {}
        Vector2() : x(0), y(0) {}
    };


    enum class STATE
    {
        RUNNING,
        LOOSE,
        WIN,
        WAIT,
    };

    enum class RENDER_STEP
    {
        STATUS_LIMITS,
        STATUS_RENDER,
        BALL_DELETE,
        BALL_DRAW,
        PLAYER_DELETE,
        PLAYER_APPEND,
        BRICK_DELETE,
        FINISHED,
    };

    constexpr int16_t BALL_SIZE{8};
    constexpr int16_t PLAYER_WIDTH{48};
    constexpr int16_t PLAYER_HEIGHT{6};
    constexpr int16_t PLAYER_DISTANCE_FROM_BOTTOM{2};
    constexpr int16_t BRICK_X{16};
    constexpr int16_t BRICK_Y{8};
    constexpr int16_t BRICK_PADDING{2};

    constexpr size_t BRICK_X_CNT{15};
    constexpr size_t BRICK_Y_CNT{8};
    constexpr uint16_t w = Color::WHITE.toST7789_SPI_native();
    constexpr uint16_t b = Color::BLACK.toST7789_SPI_native();
    constexpr Color::Color565 PALETTE[]{
        Color::RED, 
        Color::BLUE,
        Color::GREEN, 
        Color::YELLOW,
        Color::MAGENTA,
        Color::PURPLE,
        Color::NAVY,
        Color::CYAN,
        };

    template <uint16_t SCREEN_X = 240, uint16_t SCREEN_Y = 240>
    class Renderer : public IAsyncRenderer
    {
    private:
        uint8_t bricks[BRICK_X_CNT][BRICK_Y_CNT];
        int bricksAvailable{0};
        int points=0;
        
        float ball_x{(SCREEN_X / 2) - (BALL_SIZE / 2)};
        int16_t ball_y{(SCREEN_Y / 2) - (BALL_SIZE / 2)}; // ist absolute Bildschirmkoordinate obere linke Ecke
        float ball_x_old{(SCREEN_X / 2) - (BALL_SIZE / 2)};
        int16_t ball_y_old{(SCREEN_Y / 2) - (BALL_SIZE / 2)};
        STATE state{STATE::RUNNING};
        RENDER_STEP render{RENDER_STEP::FINISHED};
        float vx{0.0};
        int16_t vy{2};
        int16_t player_x{(SCREEN_X-PLAYER_WIDTH)/2};
        int16_t player_x_old{(SCREEN_X-PLAYER_WIDTH)/2};
        int brickToDeleteX{-1};
        int brickToDeleteY{-1};
        const lcd_common::FontDesc* const font;


        const int16_t player_top = SCREEN_Y-32-PLAYER_DISTANCE_FROM_BOTTOM-PLAYER_HEIGHT;
        spilcd16::FullTextlineRenderer<32, SCREEN_X, 5,5,24>* win_loose_renderer;
        spilcd16::FullTextlineRenderer<32, SCREEN_X, 5,5,24>* state_renderer;

        // Funktion zur Überprüfung, ob sich zwei Rechtecke überlappen
        //-1=keine Berührung, 0 von oben, 1 von rechts, 2 von unten
        int16_t doRectanglesOverlap(Vector2 upperLeft1, Vector2 lowerRight1, Vector2 upperLeft2, Vector2 lowerRight2)
        {
            // Überprüfen, ob ein Rechteck links vom anderen ist
            if (lowerRight1.x < upperLeft2.x || lowerRight2.x < upperLeft1.x)
            {
                return false;
            }

            // Überprüfen, ob ein Rechteck unter dem anderen ist (weil y von oben nach unten geht)
            if (upperLeft1.y > lowerRight2.y || upperLeft2.y > lowerRight1.y)
            {
                return false;
            }

            // Rechtecke überlappen sich
            return true;
        }

    public:
        void GameInit(iRectFiller* rectFiller)
        {
            bricksAvailable=0;
            points=0;
            rectFiller->FillRectSyncPolling(Point2D(0, 0), Point2D(SCREEN_X, SCREEN_Y), Color::BLACK);
        
            for (size_t x = 0; x < BRICK_X_CNT; x++)
            {
                for (size_t y = 0; y < BRICK_Y_CNT; y++)
                {
                    bricks[x][y] = 1;
                    rectFiller->FillRectSyncPolling(Point2D(x*BRICK_X+BRICK_PADDING, 32+y*BRICK_Y+BRICK_PADDING), Point2D((x+1)*BRICK_X-BRICK_PADDING, 32+(y+1)*BRICK_Y-BRICK_PADDING),PALETTE[y]);
                    bricksAvailable++;
                }
            }
            
           
            rectFiller->FillRectSyncPolling(Point2D(ball_x, ball_y), Point2D(ball_x+BALL_SIZE, ball_y+BALL_SIZE), Color::WHITE);
            rectFiller->FillRectSyncPolling(Point2D(player_x, SCREEN_Y-PLAYER_HEIGHT-PLAYER_DISTANCE_FROM_BOTTOM), Point2D(player_x+PLAYER_WIDTH, SCREEN_Y-PLAYER_DISTANCE_FROM_BOTTOM), Color::WHITE);
            ball_x=(SCREEN_X / 2) - (BALL_SIZE / 2);
            ball_y=(SCREEN_Y / 2) - (BALL_SIZE / 2);
            ball_x_old=ball_x;
            ball_y_old=ball_y;
            state=STATE::RUNNING;
            render=RENDER_STEP::FINISHED;
            vx=0.0;
            vy=-2;
            player_x=(SCREEN_X-PLAYER_WIDTH)/2;
            player_x_old=player_x;
            brickToDeleteX=-1;
            brickToDeleteY=-1;
        }
/*
        void CheckBrickHit()
        {
            brickToDeleteX=-1;
            brickToDeleteY=-1;
            int16_t ballYinBrick = ball_y + (BALL_SIZE / 2) / BRICK_Y;
            if (ballYinBrick < 0 || ballYinBrick >= BRICK_Y_CNT)
                return;
            int16_t ballXinBrick = ball_x + (BALL_SIZE / 2) / BRICK_X;
            // if(ballXinBrick<0 || ballXinBrick>=BRICK_X_CNT) return; can never happen
            for (int x = std::max(0, ballXinBrick - 1); x < std::min((int)BRICK_X_CNT, ballXinBrick + 1); x++)
            {
                for (int y = std::max(0, ballYinBrick - 1); y < std::min((int)BRICK_Y_CNT, ballYinBrick + 1); y++)
                {
                    // check, whether ball touches bricks[x][y]
                    Vector2 brick_ul(x * BRICK_X + BRICK_PADDING, y * BRICK_Y + BRICK_PADDING);
                    Vector2 brick_lr((x + 1) * BRICK_X - BRICK_PADDING, (y + 1) * BRICK_Y - BRICK_PADDING);
                    Vector2 ball_lr(ball_x + BALL_SIZE, ball_y + BALL_SIZE);
                    if(doRectanglesOverlap(ball, ball_lr, brick_ul, brick_lr)){
                        brickToDeleteX=x;
                        brickToDeleteY=y;
                        return;
                    }
                }
            }
        }
*/
        void GameLoop(int8_t playerMovement, bool reset, iRectFiller* rectFiller)
        {
            if(reset){
                GameInit(rectFiller);
            }
            if(state==STATE::LOOSE ||state==STATE::WIN){
                state=STATE::WAIT;
                return;
            }
            if(state==STATE::WAIT){
                return;
            }
            if (ball_y > SCREEN_Y)
            {
                state = STATE::LOOSE;
                win_loose_renderer->printfl(3, Color::RED, Color::BLACK, "You Loose!");
                return;
            }
            if (bricksAvailable==0)
            {
                state = STATE::WIN;
                win_loose_renderer->printfl(3, Color::GREEN, Color::BLACK, "You Win!");
                return;
            }

            assert(state==STATE::RUNNING);
            
            if(render!=RENDER_STEP::FINISHED){
                ESP_LOGE(TAG, "render!=RENDER_STEP::FINISHED");
            }

            ball_x_old=ball_x;
            ball_y_old=ball_y;
            player_x_old=player_x;

            if (playerMovement < 0)
            {
                player_x = std::max(0, player_x + playerMovement);
            }
            else if (playerMovement > 0)
            {
                player_x = std::min(SCREEN_X - PLAYER_WIDTH, player_x + playerMovement);
            }
            ESP_LOGD(TAG, "Playerx %d",player_x);

            ball_x += vx;
            ball_y += vy;
            //Rechte Begrenzung
            if (ball_x+BALL_SIZE > SCREEN_X)
            {
                ball_x = SCREEN_X-BALL_SIZE;
                vx = -vx;
            }
            //Linke Begrenzung
            else if (ball_x < 0)
            {
                ball_x = 0;
                vx = -vx;
            }
            //Fliegt nach unten durch-->bereits oben abgeprüft

            //Stößt an obere Begrenzung
            if (ball_y < 0)
            {
                ball_y = 0;
                vy = -vy;
            }
            else if (ball_y+BALL_SIZE>=player_top && ball_x+BALL_SIZE > player_x && ball_x < player_x + PLAYER_WIDTH){
            //Trifft den Schläger
                vx = -((((float)player_x + ((float)PLAYER_WIDTH / 2.0)) - (ball_x+BALL_SIZE/2)) / ((float)PLAYER_WIDTH / 2.0));
                vx *= 1.8;
                ESP_LOGI(TAG, "Hit Player, vx=%f", vx);
                vy = -vy;
            }

            //Trifft einen Brick?
            brickToDeleteX=-1;
            brickToDeleteY=-1;
            int16_t ballYinBrick = (ball_y + (BALL_SIZE / 2)) / BRICK_Y;
            bool needStatusUpdate{false};
            if (ballYinBrick >= 0 && ballYinBrick < BRICK_Y_CNT){
                int16_t ballXinBrick = (ball_x + (BALL_SIZE / 2)) / BRICK_X;
                //ESP_LOGI(TAG, "Ball in Brick x%d y%d",ballXinBrick, ballYinBrick);
                if(bricks[ballXinBrick][ballYinBrick]!=0){
                    bricks[ballXinBrick][ballYinBrick]=0;
                    brickToDeleteX=ballXinBrick;
                    brickToDeleteY=ballYinBrick;
                    bricksAvailable--;
                    points++;
                    needStatusUpdate=true;
                    if (ball_y <= ballYinBrick * BRICK_Y+2 || ball_y >= ballYinBrick * BRICK_Y  + 4)
                    {
                        vy = -vy;
                    }
                    else
                    {
                        vx = -vx;
                    }
                }
            }
            if(needStatusUpdate){
                render = RENDER_STEP::STATUS_LIMITS;
                ESP_LOGI(TAG, "State update for %d points", points);
                state_renderer->printfl(0, Color::BLUE, Color::WHITE, "You have %d points", points);
            }else{
                render = RENDER_STEP::BALL_DELETE;
            }
        }

        Renderer(const lcd_common::FontDesc* const font) : font(font)
        {
            win_loose_renderer = new spilcd16::FullTextlineRenderer<32, SCREEN_X, 5,5,24>(font);
            state_renderer     = new spilcd16::FullTextlineRenderer<32, SCREEN_X, 5,5,24>(font);
        }

        bool GetNextOverallLimits(size_t bufferSize, Point2D &start, Point2D &end_excl) override
        {
            if(state==STATE::WAIT){
                return false;
            }else if (state == STATE::LOOSE){
                return win_loose_renderer->GetNextOverallLimits(bufferSize, start, end_excl);
            }else if(state==STATE::WIN){
                return win_loose_renderer->GetNextOverallLimits(bufferSize, start, end_excl);
            }

            if(render==RENDER_STEP::STATUS_LIMITS){
                state_renderer->GetNextOverallLimits(bufferSize, start, end_excl);
                render=RENDER_STEP::STATUS_RENDER;
                return true;
            }
            if(render==RENDER_STEP::STATUS_RENDER){
                render=RENDER_STEP::BALL_DELETE;
            }
            if (render == RENDER_STEP::BALL_DELETE)
            { // Delete und Draw des Balls ist immer erforderlich
                start.x = ball_x_old;
                start.y = ball_y_old+32;
                end_excl.x = ball_x_old + BALL_SIZE;
                end_excl.y = ball_y_old + BALL_SIZE+32;
                return true;
            }
            else if (render == RENDER_STEP::BALL_DRAW)
            {
                start.x = ball_x;
                start.y = ball_y+32;
                end_excl.x = ball_x + BALL_SIZE;
                end_excl.y = ball_y + BALL_SIZE+32;
                return true;
            }
            start.y = player_top+32;
            end_excl.y = player_top+32+PLAYER_HEIGHT;
            if (render == RENDER_STEP::PLAYER_DELETE)
            {
                // render=RENDER::BRICK;
                if (player_x < player_x_old) // move left
                {
                    start.x = player_x + PLAYER_WIDTH;
                    end_excl.x = player_x_old + PLAYER_WIDTH;
                    ESP_LOGD(TAG, "RENDER_STEP::PLAYER_DELETE  %d-%d",start.x, end_excl.x);
                    return true;
                }
                else if (player_x > player_x_old)
                {
                    start.x = player_x_old;
                    end_excl.x = player_x;
                    ESP_LOGD(TAG, "RENDER_STEP::PLAYER_DELETE  %d-%d",start.x, end_excl.x);
                    return true;
                }
                else
                {
                    render = RENDER_STEP::BRICK_DELETE; // FALL through
                    start.x = end_excl.x = 0;
                }
                
            }
            
            else if (render == RENDER_STEP::PLAYER_APPEND)
            {
                if (player_x < player_x_old) // move left
                {
                    start.x = player_x;
                    end_excl.x = player_x_old;
                    ESP_LOGD(TAG, "RENDER_STEP::PLAYER_APPEND  %d-%d",start.x, end_excl.x);
                    return true;
                }
                else if (player_x > player_x_old)
                {
                    start.x = player_x_old + PLAYER_WIDTH;
                    end_excl.x = player_x + PLAYER_WIDTH;
                    ESP_LOGD(TAG, "RENDER_STEP::PLAYER_APPEND  %d-%d",start.x, end_excl.x);
                    return true;
                }
                else
                {
                    render = RENDER_STEP::BRICK_DELETE; // FALL through
                }
                ESP_LOGI(TAG, "RENDER_STEP::PLAYER_APPEND  %d-%d",start.x, end_excl.x);
            }
            if (render == RENDER_STEP::BRICK_DELETE)
            { // kein else if, weil aus den oberen States hier reingesprungen werden kann
                if(brickToDeleteX>=0){
                    start.x = brickToDeleteX*BRICK_X+BRICK_PADDING;
                    end_excl.x = (brickToDeleteX+1)*BRICK_X-BRICK_PADDING;
                    start.y=brickToDeleteY*BRICK_Y+BRICK_PADDING+32;
                    end_excl.y=(brickToDeleteY+1)*BRICK_Y-BRICK_PADDING+32;
                    return true;
                }else{
                    render = RENDER_STEP::FINISHED;
                }
            }

            return false;
        }

        void Render(uint16_t startline, uint16_t lines_cnt, uint16_t *buffer) override
        {
            if(state==STATE::WAIT){
                ESP_LOGE(TAG, "In Reder while State::WAIT");
                return;
            }
            if(state==STATE::LOOSE || state==STATE::WIN){
                win_loose_renderer->Render(startline, lines_cnt, buffer);
                return;
            }
            
            
            switch (render)
            {
            case RENDER_STEP::STATUS_RENDER:
                state_renderer->Render(startline, lines_cnt, buffer);
                break;
            case RENDER_STEP::BALL_DELETE:
                for (int i = 0; i < BALL_SIZE * BALL_SIZE; i++)
                    *(buffer++) = b;
                render=RENDER_STEP::BALL_DRAW;
                break;
            case RENDER_STEP::BALL_DRAW:
                for (int i = 0; i < BALL_SIZE * BALL_SIZE; i++)
                    *(buffer++) = w;
                render=RENDER_STEP::PLAYER_DELETE;
                break;
            case RENDER_STEP::PLAYER_DELETE:
                for (int i = 0; i < std::abs(player_x - player_x_old) * PLAYER_HEIGHT; i++)
                    *(buffer++) = b;
                render=RENDER_STEP::PLAYER_APPEND;
                break;
            case RENDER_STEP::PLAYER_APPEND:
                for (int i = 0; i < std::abs(player_x - player_x_old) * PLAYER_HEIGHT; i++)
                    *(buffer++) = w;
                render=RENDER_STEP::BRICK_DELETE;
                break;
            case RENDER_STEP::BRICK_DELETE:
                for (int i = 0; i < (BRICK_X-2*BRICK_PADDING)*(BRICK_Y-2*BRICK_PADDING); i++)
                    *(buffer++) = b;
                render=RENDER_STEP::FINISHED;
            default:
                break;
            }
            
        }
    };
}
#undef TAG