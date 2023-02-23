#include "DogeLib/Includes.h"
#define BOARD_X 1200
#define BOARD_Y 600
#define WALL_LEN 32
#define WIN_X (BOARD_X+WALL_LEN*2)
#define WIN_Y (BOARD_Y+WALL_LEN*2)
#define BALL_RAD 14
#define POCKET_RAD 32
#define NUMBALLS 16
#define MAX_SPEED 32.0f
#define MAX_STROKE 200.0f

const Color TABLECOLOR = {0, 128, 32, 255};
const Color BUMPERCOLOR = {0, 64, 16, 255};
const Color WALLCOLOR = {0x4a,0x21, 0x06, 255};
const Length window = {.x = WIN_X, .y = WIN_Y};
const Length board = {.x = BOARD_X, .y = BOARD_Y};

const Coordf pockets[6] = {
    {.x=WALL_LEN,.y=WALL_LEN},
    {.x=WIN_X/2,.y=WALL_LEN/2+WALL_LEN/4},
    {.x=WIN_X-WALL_LEN,.y = WALL_LEN},
    {.x=WALL_LEN,.y=WIN_Y-WALL_LEN},
    {.x=WIN_X/2,.y = WIN_Y-(WALL_LEN/2+WALL_LEN/4)},
    {.x=WIN_X-WALL_LEN,WIN_Y-WALL_LEN}
};

typedef enum{
    S_HOVER,
    S_CLICKD,
    S_CLICKU,
    S_WAIT,
    S_PLACE,
    S_N
}StrokeState;

typedef struct{
    uint turn;
    StrokeState state;
    Coordf clickDown;
    Coordf clickUp;
    bool canPlace;
}Stroke;

typedef enum{B_CUE, B_SOLID, B_STRIPE}BallType;

typedef struct{
    BallType type;
    bool isSunk;
    uint sunkTurn;
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
            fillCircleCoord(CfC(balls[i].pos), BALL_RAD);
            if(balls[i].type == B_STRIPE){
                setColor(WHITE);
                fillCircleCoord(CfC(balls[i].pos), BALL_RAD/3);
            }
        }
    }
}

bool ballsMoving(Ball *balls)
{
    for(uint i = 0; i < NUMBALLS; i++)
        if(!balls[i].isSunk && cfMax(balls[i].vel))
            return true;
    return false;
}

bool validPlace(const Coord pos, Ball *balls)
{
    if(
        pos.x >= WIN_X-(BALL_RAD+WALL_LEN) || pos.x < BALL_RAD+WALL_LEN ||
        pos.y >= WIN_Y-(BALL_RAD+WALL_LEN) || pos.y < BALL_RAD+WALL_LEN
    )
        return false;
    for(uint h = 0; h < 6; h++)
        if(cfDist(CCf(pos), pockets[h]) < POCKET_RAD + BALL_RAD/4)
            return false;

    for(uint i = 1; i < NUMBALLS; i++)
        if(!balls[i].isSunk && cfDist(CCf(pos), balls[i].pos) < BALL_RAD*2+1)
            return false;
    return true;
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
        stroke.state = S_WAIT;
        break;
    case S_WAIT:
        if(ballsMoving(balls))
            break;
        stroke.state = balls[0].isSunk ? S_PLACE : S_HOVER;
        stroke.turn++;
        break;
    case S_PLACE:
        stroke.canPlace = validPlace(mouse.pos, balls);
        if(stroke.canPlace && mouseBtnReleased(MOUSE_L)){
            balls[0].pos = CCf(mouse.pos);
            balls[0].isSunk = false;
            stroke.state = S_HOVER;
        }
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
    if(ball.isSunk){
        printf("Attempted to hit ball with isSunk = true\n");
        exit(-1);
    }
    if(stroke.state != S_CLICKU){
        printf("Attempted to hit ball while stroke.state = %s\n", stateStrs[stroke.state]);
        exit(-1);
    }
    const float ang = cfToRad(cfAdd(stroke.clickDown, cfNeg(stroke.clickUp)));
    const float power = fmin(cfDist(stroke.clickUp, stroke.clickDown), MAX_STROKE);
    ball.vel = radMagToCf(ang, power / MAX_STROKE * MAX_SPEED);
    printf("stroke (%.3f, %.3f)\n", ball.vel.x, ball.vel.y);
    return ball;
}

bool pocketBall(Ball *ball, const Coordf newpos)
{
    for(uint h = 0; h < 6; h++){
        if(fmin(
            cfDist(cfAdd(ball->pos, radMagToCf(cfToRad(ball->vel),cfMag(ball->vel)/2)), pockets[h]),
            cfDist(newpos, pockets[h]
        )) < POCKET_RAD){
            ball->isSunk = true;
            ball->vel.x = 0;
            ball->vel.y = 0;
            return true;
        }
    }
    return false;
}

void pocketAndWallBalls(Ball *balls)
{
    for(uint i = 0; i < NUMBALLS; i++){
        const Coordf newpos = cfAdd(balls[i].pos, balls[i].vel);

        if(pocketBall(&balls[i], newpos))
            continue;

        if(newpos.x>=WIN_X-(BALL_RAD+WALL_LEN) || newpos.x<BALL_RAD+WALL_LEN){
            balls[i].vel.x = -balls[i].vel.x;
        }
        if(newpos.y>=WIN_Y-(BALL_RAD+WALL_LEN) || newpos.y<BALL_RAD+WALL_LEN){
            balls[i].vel.y = -balls[i].vel.y;
        }
    }
}

void updateBalls(Ball *balls)
{
    for(uint i = 0; i < NUMBALLS; i++){
        if(!balls[i].isSunk && cfMax(balls[i].vel)){
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

void collide(Ball *a, Ball *b, const float overlap)
{
    // const float ovrlap = 2*BALL_RAD-cfDist(a->pos, b->pos);
    const Coordf normal = cfNormalize(cfSub(a->pos, b->pos));
    a->pos = cfAdd(a->pos, cfMul(normal, overlap/2));
    b->pos = cfSub(b->pos, cfMul(normal, overlap/2));

    const float athing = cfDot(normal, a->vel);
    const float bthing = cfDot(normal, b->vel);
    const Coordf aperp = cfMul(normal, athing);
    const Coordf bperp = cfMul(normal, bthing);
    a->vel = cfAdd(cfSub(a->vel, aperp), bperp);
    b->vel = cfAdd(cfSub(b->vel, bperp), aperp);
}

uint collideBalls(Ball *balls)
{
    uint total = 0;
    for(uint i = 0; i < NUMBALLS-1; i++){
        if(balls[i].isSunk)
            continue;
        for(uint j = i+1; j < NUMBALLS; j++){
            if(balls[j].isSunk)
                continue;
            Ball *a = &balls[i];
            Ball *b = &balls[j];
            const float overlap = 2*BALL_RAD-cfDist(a->pos, b->pos);
            if(overlap > 0){
                collide(a, b, overlap);
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
            fillCircleCoord(mouse.pos, 4);
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

            setColor(GREY);
            drawLineCoords(CfC(bpos), CfC(cfAdd(cfMul(offset, -dist/48), bpos)));

            break;
        case S_CLICKU:
            break;
        case S_WAIT:
            break;
        case S_PLACE:
            setColor(stroke.canPlace ? GREY1 : (const Color){255,64,64,128});
            fillCircleCoord(mouse.pos, BALL_RAD);
            break;
        default:
            printf("wat\n");
            exit(-1);
            break;
    }
}

bool ballHoled(const Coordf bpos, const Coord hpos)
{
    return cfMax(cfSub(bpos, CCf(hpos))) <= BALL_RAD+2.0f;
}

void initBalls(Ball *balls)
{
    balls[0].type = B_CUE;
    balls[0].color = WHITE;
    balls[0].pos = (const Coordf){.x=WIN_X/4, .y=WIN_Y/2};
    balls[0].vel = (const Coordf){0};
    balls[0].isSunk = false;


    uint i = 1;
    const Coordf init = {.x=WIN_X*3.0f/4.0f, .y=WIN_Y/2.0f};
    for(uint x = 0; x < 5; x++){
        const Coordf posTop = cfAdd(cfMul(degMagToCf(30.0f, BALL_RAD*2.0f),x), init);
        for(uint y = 0; y <= x; y++){
            balls[i].pos.x = posTop.x;
            balls[i].pos.y = posTop.y - BALL_RAD*2*y;
            balls[i].vel = (const Coordf){0};
            balls[i].num = i;
            balls[i].type = i&1 ? B_STRIPE : B_SOLID;
            balls[i].isSunk = false;
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

void drawBoard(void)
{
    const Length wallpair = {.x=WALL_LEN, .y=WALL_LEN};
    setColor(WALLCOLOR);
    fillRectCoordLength((const Coord){0}, window);
    setColor(BUMPERCOLOR);
    fillRectCoordLength(coordAdd(wallpair, -WALL_LEN/4), coordAdd(board, WALL_LEN/2));
    setColor(TABLECOLOR);
    fillRectCoordLength(
        wallpair,
        coordAdd(window, -WALL_LEN*2)
    );

    setColor(BLACK);
    for(uint i = 0; i < 6; i++)
        fillCircleCoord(CfC(pockets[i]), POCKET_RAD);
}

int main(int argc, char const *argv[])
{
    (void)argc;
    (void)argv;
    init();
    setWindowLen(window);
    setWindowResizable(false);

    Stroke stroke = {.turn = 1};
    Ball balls[NUMBALLS] = {0};
    initBalls(balls);
    while(1){
        const uint t = frameStart();

        if(keyState(SDL_SCANCODE_LCTRL)){
            if(keyPressed(SDL_SCANCODE_Q))
                return 0;
            if(keyPressed(SDL_SCANCODE_R))
                initBalls(balls);
        }

        if(mouseBtnPressed(MOUSE_R)){
            balls[0].pos = CCf(mouse.pos);
            balls[0].isSunk = false;
        }


        if(stroke.state == S_CLICKU && !balls[0].isSunk)
            balls[0] = hitBall(balls[0], stroke);

        stroke = updateStroke(stroke, balls);

        updateBalls(balls);
        collideBalls(balls);
        pocketAndWallBalls(balls);

        drawBoard();
        drawBalls(balls);
        drawStroke(stroke, balls[0].pos);

        frameEnd(t);
    }
    return 0;
}
