#include <vector>

#define BOARD_SIZE_H 60
#define BOARD_SIZE_V 15

#define UP    0
#define RIGHT 1
#define DOWN  2
#define LEFT  3

class Snake
{
private:
    struct Head
    {
        int x_Pos = BOARD_SIZE_H / 2;
        int y_Pos = (BOARD_SIZE_V + 2) / 2;
    };

    struct BodyPart
    {
        int x_Pos = 0;
        int y_Pos = 0;
    };
    std::vector<BodyPart> m_Parts;
    Head m_Head;
    char m_Direction = RIGHT;
public:
    bool p_Destroyed = false;
public:
    Snake() = default;
    void print();
    void update();
    void setDirection(const char& dir);
private:
    void move();
    bool collided();
};
