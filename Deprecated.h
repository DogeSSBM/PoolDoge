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
