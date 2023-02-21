#include "DogeLib/Includes.h"
#define ROUND 14
#define NUMBALLS 16
#define MAX_SPEED 32.0f
#define MAX_STROKE 200.0f
const Color HOLECOLOR = {200, 64, 0, 255};
const Color TABLECOLOR = {0, 128, 32, 255};

typedef enum{
    S_HOVER,
    S_CLICKD,
    S_CLICKU,
    S_WAIT,
    S_N
}StrokeState;

typedef struct{
    StrokeState state;
    Coordf clickDown;
    Coordf clickUp;
    uint strokenumber;
}Stroke;

typedef enum{B_CUE, B_SOLID, B_STRIPE}BallType;

typedef struct{
    BallType type;
    bool isSunk;
    uint num;
    Color color;
    Coordf pos;
    Coordf vel;
}Ball;

void printStroke(const Stroke stroke)
{
    const char *stateStrs[] = {"S_HOVER", "S_CLICKD", "S_CLICKU", "S_WAIT"};
    printf(
        "Stroke -\n\tstate: %s\n\tclickDown: (%f,%f)\n\tclickUp: (%f,%f)\n",
        stateStrs[stroke.state],
        stroke.clickDown.x,
        stroke.clickDown.y,
        stroke.clickUp.x,
        stroke.clickUp.y
    );
}

void drawBalls(Ball *balls)
{
    for(uint i = 0; i < NUMBALLS; i++){
        if(!balls[i].isSunk){
            setColor(balls[i].color);
            fillCircleCoord(CfC(balls[i].pos), ROUND);
        }
    }
}

bool ballsMoving(Ball *balls)
{
    for(uint i = 0; i < NUMBALLS; i++)
        if(cfMax(balls[i].vel))
            return true;
    return false;
}

Stroke updateStroke(Stroke stroke, Ball *balls)
{
    switch (stroke.state)
    {
    case S_HOVER:
        if(mouseBtnPressed(MOUSE_L)){
            stroke.state = S_CLICKD;
            stroke.clickDown = CCf(mouse.pos);
        }
        setColor(GREY);
        fillCircleCoord(mouse.pos, 8);
        break;
    case S_CLICKD:
        if(keyPressed(SDL_SCANCODE_ESCAPE)){
            stroke.state = S_HOVER;
            break;
        }
        if(mouseBtnReleased(MOUSE_L)){
            stroke.clickUp = CCf(mouse.pos);
            stroke.state = S_CLICKU;
            break;
        }
        break;
    case S_CLICKU:
        stroke.strokenumber++;
        stroke.state = S_WAIT;
        break;
    case S_WAIT:
        if(ballsMoving(balls))
            break;
        stroke.state = S_HOVER;
        break;
    default:
        printf("wat\n");
        exit(-1);
        break;
    }
    return stroke;
}

Ball hitBall(Ball ball, const Stroke stroke)
{
    const char *stateStrs[] = {"S_HOVER", "S_CLICKD", "S_CLICKU", "S_WAIT"};
    if(ball.vel.x || ball.vel.y){
        printf("Attempted to hit ball while moving\n");
        exit(-1);
    }
    if(stroke.state != S_CLICKU){
        printf("Attempted to hit ball while stroke.state = %s\n", stateStrs[stroke.state]);
        exit(-1);
    }
    const float ang = cfToRad(cfAdd(stroke.clickDown, cfNeg(stroke.clickUp)));
    const float power = fmin(cfDist(stroke.clickUp, stroke.clickDown), MAX_STROKE);
    ball.vel = radMagToCf(ang, power / MAX_STROKE * MAX_SPEED);
    return ball;
}

Ball ballWallBounce(Ball ball, const Length window)
{
    if(ball.vel.x || ball.vel.y)
    {
        const Coordf newpos = cfAdd(ball.pos, ball.vel);
        if(newpos.x >= window.x-ROUND || newpos.x < ROUND)
        {
            ball.vel.x = -ball.vel.x;
        }
        if(newpos.y >= window.y-ROUND || newpos.y < ROUND)
        {
            ball.vel.y = -ball.vel.y;
        }
        ball.pos = cfAdd(ball.pos, ball.vel);
        const float ang = cfToRad(ball.vel);
        float speed = cfMag(ball.vel);
        if(speed < .02f)
        {
            ball.vel.x = 0;
            ball.vel.y = 0;
            return ball;
        }
        speed = speed * .98f;
        ball.vel = radMagToCf(ang, speed);
    }
    return ball;
}

void updateBalls(Ball *balls, const Length window)
{
    for(uint i = 0; i < NUMBALLS; i++){
        if(balls[i].vel.x || balls[i].vel.y){
            const Coordf newpos = cfAdd(balls[i].pos, balls[i].vel);
            if(newpos.x >= window.x-ROUND || newpos.x < ROUND)
            {
                balls[i].vel.x = -balls[i].vel.x;
            }
            if(newpos.y >= window.y-ROUND || newpos.y < ROUND)
            {
                balls[i].vel.y = -balls[i].vel.y;
            }
            balls[i].pos = cfAdd(balls[i].pos, balls[i].vel);
            const float ang = cfToRad(balls[i].vel);
            float speed = cfMag(balls[i].vel);
            if(speed < .02f)
            {
                balls[i].vel.x = 0;
                balls[i].vel.y = 0;
            }else{
                speed = speed * .98f;
                balls[i].vel = radMagToCf(ang, speed);
            }
        }
    }
}


// mi the mass
// vi the vector of velocity before collision
// v'i the vector of velocity after collision
// Oi the point of center
// xi the vector of Oi position

void collide(Ball *a, Ball *b)
{
    const Coordf temp = {.x = a->vel.x, .y = a->vel.y};
    a->vel.x = b->vel.x;
    a->vel.y = b->vel.y;
    b->vel.x = temp.x;
    b->vel.y = temp.y;
}

uint collideBalls(Ball *balls)
{
    uint total = 0;
    for(uint i = 0; i < NUMBALLS; i++){
        for(uint j = 0; j < NUMBALLS; j++){
            Ball *a = &balls[i];
            Ball *b = &balls[(i+1+j)%NUMBALLS];
            if(cfDist(cfAdd(a->pos, a->vel), cfAdd(b->pos, b->vel)) < ROUND*2){
                collide(a, b);
                total++;
            }
        }
    }
    return total;
}

void drawStroke(const Stroke stroke, const Coordf bpos)
{
    switch (stroke.state){
        case S_HOVER:
            setColor(GREY);
            fillCircleCoord(mouse.pos, 8);
            break;
        case S_CLICKD:;
            const Coordf mpos = CCf(mouse.pos);
            Coordf offset = cfSub(mpos, stroke.clickDown);
            float dist = cfDist(stroke.clickDown, mpos);

            if(dist > MAX_STROKE){
                offset = cfMul(offset, MAX_STROKE / dist);
                dist = MAX_STROKE;
            }

            const u8 r = 255.0f * dist / MAX_STROKE;
            setRGB(r, 0, 255 - r);
            drawLineCoords(CfC(bpos), CfC(cfAdd(bpos, offset)));
            break;
        case S_CLICKU:
            break;
        case S_WAIT:
            break;
        default:
            printf("wat\n");
            exit(-1);
            break;
    }
}

bool ballHoled(const Coordf bpos, const Coord hpos)
{
    return cfMax(cfSub(bpos, CCf(hpos))) < ROUND;
}

void initBalls(Ball *balls, const Length window)
{
    balls[0].type = B_CUE;
    balls[0].color = WHITE;
    balls[0].pos = (const Coordf){.x=window.x/4, .y=window.y/2};
    balls[0].vel = (const Coordf){0};

    uint i = 1;
    for(uint x = 0; x < 5; x++){
        const Coord posTop = {.x=x+window.x*3/4+ROUND*2*x, .y=window.y/2-ROUND*x};
        for(uint y = 0; y <= x; y++){
            balls[i].pos = CCf(coordShift(posTop, DIR_D, 2*y+ROUND*2*y));
            balls[i].num = i;
            balls[i].type = i&1 ? B_SOLID : B_STRIPE;
            i++;
        }
    }

    balls[ 1].color = (const Color){255, 255,   0, 255};
    balls[ 2].color = (const Color){  0,   0, 255, 255};
    balls[ 3].color = (const Color){255,   0,   0, 255};
    balls[ 4].color = (const Color){128,   0, 128, 255};
    balls[ 5].color = (const Color){255, 165,   0, 255};
    balls[ 6].color = (const Color){  0, 255,   0, 255};
    balls[ 7].color = (const Color){128,   0,  32, 255};
    balls[ 8].color = (const Color){  0,   0,   0, 255};
    balls[ 9].color = (const Color){255, 255,   0, 255};
    balls[10].color = (const Color){  0,   0, 255, 255};
    balls[11].color = (const Color){255,   0,   0, 255};
    balls[12].color = (const Color){128,   0, 128, 255};
    balls[13].color = (const Color){255, 165,   0, 255};
    balls[14].color = (const Color){  0, 255,   0, 255};
    balls[15].color = (const Color){128,   0,  32, 255};
}

int main(int argc, char const *argv[])
{
    (void)argc;
    (void)argv;
    init();
    Length window = {.x = 1200, .y = 600};
    setWindowLen(window);
    gfx.defaultColor = TABLECOLOR;

    Stroke stroke = {.strokenumber = 1};
    Ball balls[NUMBALLS] = {0};
    initBalls(balls, window);
    // Ball ball1 = {.pos = CCf(coordShift(coordDiv(window,2),DIR_R,window.x/4))};
    // Ball ball2 = {.pos = CCf(coordShift(coordDiv(window,2),DIR_L,window.x/4))};
    while(1){
        const uint t = frameStart();

        if(keyState(SDL_SCANCODE_LCTRL) && keyPressed(SDL_SCANCODE_Q)){
            return 0;
        }

        if(stroke.state == S_CLICKU)
            balls[0] = hitBall(balls[0], stroke);
        stroke = updateStroke(stroke, balls);

        collideBalls(balls);
        updateBalls(balls, window);

        drawStroke(stroke, balls[0].pos);
        drawBalls(balls);

        frameEnd(t);
    }
    return 0;
}
