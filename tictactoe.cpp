//version 1.0.0

//This file has been created with the create command
//This is a C++ file
//A CMakeLists file has been created as well
//The CMakeLists file has been optimized to work with the Clion IDE

#ifdef linux
    #include <unistd.h>
#endif

#ifdef _WIN32
    #include <Windows.h>
#endif


#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <limits>
using namespace std;




class Board
{
private:
    vector<int> current_game = {0,0,0,0,0,0,0,0,0};

    string translateList(int to_translate) const
    {
        if (to_translate == 0) return " ";
        else if (to_translate == 1) return "X";
        else if (to_translate == 2) return "O";
        return "none";
    }

public:
    void printBoard() const
    {
        cout << " " + this->translateList(this->current_game[0]) + " " + "|" + " " + this->translateList(this->current_game[1]) + " "  + "|" + " "  + this->translateList(this->current_game[2]) + "        " + " " + "1" + " " + "|" + " " + "2" + " "  + "|" + " "  + "3" << endl;
        cout << " " + this->translateList(this->current_game[3]) + " " + "|" + " " + this->translateList(this->current_game[4]) + " "  + "|" + " "  + this->translateList(this->current_game[5]) + "        " + " " + "4" + " " + "|" + " " + "5" + " "  + "|" + " "  + "6" << endl;
        cout << " " + this->translateList(this->current_game[6]) + " " + "|" + " " + this->translateList(this->current_game[7]) + " "  + "|" + " "  + this->translateList(this->current_game[8]) + "        " + " " + "7" + " " + "|" + " " + "8" + " "  + "|" + " "  + "9" << endl;
    }

    void changeListByIndex(int index, int player_number)
    {
        this->current_game[index] = player_number;
    }

    bool checkIfMoveIsAllowed(int index)
    {
        if (this->current_game[index] == 0) return true;
        return false;
    }

    bool gameNotOver()
    {
        for (int i: this->current_game)
        {
            if (i == 0) return true;
        }
        return false;
    }

    bool checkForWin(int player_number)
    {
        // horizontal rows;
        if (this->current_game[0] == player_number && this->current_game[1] == player_number && this->current_game[2] == player_number) return true;
        else if (this->current_game[3] == player_number && this->current_game[4] == player_number && this->current_game[5] == player_number) return true;
        else if (this->current_game[6] == player_number && this->current_game[7] == player_number && this->current_game[8] == player_number) return true;

        //vertical columns == 1 || 2
        else if (this->current_game[0] == player_number && this->current_game[3] == player_number && this->current_game[6] == player_number) return true;
        else if (this->current_game[1] == player_number && this->current_game[4] == player_number && this->current_game[7] == player_number) return true;
        else if (this->current_game[2] == player_number && this->current_game[5] == player_number && this->current_game[8] == player_number) return true;

        //diagonal == 1 || 2
        else if (this->current_game[0] == player_number && this->current_game[4] == player_number && this->current_game[8] == player_number) return true;
        else if (this->current_game[2] == player_number && this->current_game[4] == player_number && this->current_game[6] == player_number) return true;

        return false;
    }

};


class Player
{
public:
    Player(int player_number)
    {
        this->player_number = player_number;
    }
    int player_number;
};



int main() {

    system("clear");
    Board board1;
    Player player1(1); //X
    Player player2(2); //O
    board1.printBoard();


    int move;
    vector<int> allowed_numbers = {1,2,3,4,5,6,7,8,9};
    while(board1.gameNotOver())
    {
        if (!board1.gameNotOver()) break;
        // player 1 move
        cout << "Player 1 make a move: ";
        cin >> move;
        if (cin.fail())
        {
            cin.clear();
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }

        if (count(allowed_numbers.begin(), allowed_numbers.end(), move))
        {
            move -= 1;
            while(!board1.checkIfMoveIsAllowed(move))
            {
                cout << "Invalid move please provide a new one" << endl;
                cout << "Player 1 make a move: ";
                cin >> move;
                move -= 1;
            }
            system("clear");
            board1.changeListByIndex(move, player1.player_number);
            board1.printBoard();
            if (board1.checkForWin(player1.player_number))
            {
                cout << "Player 1 has won the game" << endl;
                cout << "Congratulations" << endl;
                return 0;
            }
        }
        else {cout << "Please provide a allowed number" << endl; continue;}

        if (!board1.gameNotOver()) break;
        //player 2 move
        while(true)
        {
            cout << "Player 2 make a move: ";
            cin >> move;
            if (cin.fail())
            {
                cin.clear();
                cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }

            if (count(allowed_numbers.begin(), allowed_numbers.end(), move))
            {
                move -= 1;
                while (!board1.checkIfMoveIsAllowed(move))
                {
                    cout << "Invalid move please provide a new one" << endl;
                    cout << "Player 2 make a move: " << endl;
                    cin >> move;
                    move -= 1;
                }
                system("clear");
                board1.changeListByIndex(move, player2.player_number);
                board1.printBoard();
                if (board1.checkForWin(player2.player_number))
                {
                    cout << "Player 2 has won the game" << endl;
                    cout << "Congratulations" << endl;
                    return 0;
                }
                break;
            }
            else
            {
                cout << "Please provide a allowed number" << endl;
                continue;
            }
        }
    }
    cout << "It is a draw" << endl;

   return 0;
}

