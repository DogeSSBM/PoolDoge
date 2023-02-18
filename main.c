#include "DogeLib/Includes.h"
#define ROUND 8
#define MAX_SPEED 12.0f
#define MAX_STROKE 200.0f
const Color HOLECOLOR = {200, 64, 0, 255};
const Color GRASSCOLOR = {0, 128, 32, 255};

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

typedef struct{
    Coordf pos;
    Coordf vel;
}Ball;



typedef struct{

}Hole;

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

Stroke updateStroke(Stroke stroke, const Ball ball)
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
        if(ball.vel.x || ball.vel.y)
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

Ball updateBall(Ball ball, const Length window)
{
    if(ball.vel.x || ball.vel.y)
    {
        const Coordf newpos = cfAdd(ball.pos, ball.vel);
        if(newpos.x >= window.x || newpos.x < 0)
        {
            ball.vel.x = -ball.vel.x;
        }
        if(newpos.y >= window.y || newpos.y < 0)
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

Coord randHole(const Length window)
{
    return (const Coord){
        .x = 100 + rand() % (window.x - 200),
        .y = 100 + rand() % (window.y - 200)};
}

Ball randBall(const Length window, const Coord hole)
{
    Ball ball = {.pos = CCf(randHole(window))};
    if(fabs(ball.pos.x - hole.x) < 200)
        ball.pos.x = fclamp(
            ball.pos.x < hole.x ? hole.x - 200 : hole.x + 200, 100.0f, window.x - 100);
    if(fabs(ball.pos.y - hole.y) < 200)
        ball.pos.y = fclamp(
            ball.pos.y < hole.y ? hole.y - 200 : hole.y + 200, 100.0f, window.y - 100);
    return ball;
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

int main(int argc, char const *argv[])
{
    (void)argc;
    (void)argv;
    init();
    Length window = {.x = 1200, .y = 800};
    setWindowLen(window);
    gfx.defaultColor = GRASSCOLOR;

    Stroke stroke = {.strokenumber = 1};
    Coord hole = randHole(window);
    printf("hole (%i,%i)\n", hole.x, hole.y);
    Ball ball = randBall(window, hole);
    printf("ball (%f,%f)\n", ball.pos.x, ball.pos.y);

    while(1){
        const uint t = frameStart();

        if(keyState(SDL_SCANCODE_LCTRL) && keyPressed(SDL_SCANCODE_Q)){
            return 0;
        }

        if(keyPressed(SDL_SCANCODE_R)){
            hole = randHole(window);
            ball = randBall(window, hole);
        }

        if(ballHoled(ball.pos, hole)){
            printf("Sunk ball on stroke number %i\n", stroke.strokenumber);
            printf("You are a good golfer dog!\n");
            hole = randHole(window);
            ball = randBall(window, hole);
        }

        if(stroke.state == S_CLICKU)
            ball = hitBall(ball, stroke);
        stroke = updateStroke(stroke, ball);
        ball = updateBall(ball, window);

        drawStroke(stroke, ball.pos);

        setColor(HOLECOLOR);
        fillCircleCoord(hole, ROUND);
        setColor(WHITE);
        fillCircleCoord(CfC(ball.pos), ROUND);

        frameEnd(t);
    }
    return 0;
}
