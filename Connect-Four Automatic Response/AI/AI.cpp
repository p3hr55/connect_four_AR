#include <iostream>
#include <windows.h>
#include <string>
#include <thread>
#define RED 'R'
#define BLACK 'B'
using namespace std;

int minim = INT_MIN;
int maxim = INT_MAX;
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

//A node class that is used to encapsulate items that get placed into the tree
struct Node
{
	unsigned __int64 yellow = 0;
	unsigned __int64 red = 0;
	unsigned __int64 colh[7];
	thread c;
};

class Connect4
{
	int best = 0;
	unsigned __int64 black_board = 0; //yellow_board, the computer
	unsigned __int64 red_board = 0;  //the user
	unsigned __int64 col_heights[7] = { 0 }; //column heights start at zero and work up to 42
	bool first_move_block = true; //used to circumvent if the user goes into spot 3
	int node_values[7]; //the hueristic values returned by threaded function
	Node nodes[7];

public:
	Connect4() {  }

	//used to check if the user made a correct move, computer doesn't use
	bool validmove(int col)
	{
		if (col_heights[col] != 42)
			return true;

		return false;
	}

	//used to tell if the game is a tie-game
	int moves()
	{
		int number_of_moves = 0;
		for (int i = 0; i < 7; i++)
			if (col_heights[i] != 42)
				number_of_moves++;

		return number_of_moves;
	}

	//used for inserting into a node object, the value s signifies the nodes index number
	void thread_insert(int col, char player, int s)
	{
		//needs two lines because of integer overflow
		unsigned __int64 insert_pos = (1 << col);
		insert_pos = insert_pos << nodes[s].colh[col];

		//next four lines can be phased out eventually
		if (player == RED)
			nodes[s].red = nodes[s].red | insert_pos;
		else
			nodes[s].yellow = nodes[s].yellow | insert_pos;

		nodes[s].colh[col] += 7;
	}

	//used for deleting a peice out of a nodes board, the value s signifies the nodes index number
	void thread_remove(int col, char player, int s)
	{
		//changed because of integer overflow
		unsigned __int64 remove_pos = (1 << col);
		remove_pos = remove_pos << (nodes[s].colh[col] - 7);
		remove_pos = ~remove_pos;

		if (player == RED)
			nodes[s].red = nodes[s].red & remove_pos;
		else
			nodes[s].yellow = nodes[s].yellow & remove_pos;

		nodes[s].colh[col] -= 7;
	}

	//while threaded insert and remove functions are used for inserting within a thread object,
	//   this is used for inserting into the global board integers
	void insert(int col, char player)
	{
		unsigned __int64 insert_pos = (1 << col);
		insert_pos = insert_pos << col_heights[col];

		if (player == RED)
			red_board = red_board | insert_pos;

		else if (player == BLACK)
			black_board = black_board | insert_pos;

		col_heights[col] += 7;
	}

	//similar to insert function, used for inserting into global board integers
	void remove(int col, char player)
	{
		unsigned __int64 remove_pos = (1 << col);
		remove_pos = remove_pos << (col_heights[col] - 7);
		remove_pos = ~remove_pos;

		if (player == RED)
			red_board = red_board & remove_pos;

		else if (player == BLACK)
			black_board = black_board & remove_pos;

		col_heights[col] -= 7;
	}

	//displays the connect-four board with colored graphics :)
	void display()
	{
		//2 ^ 42, will start at the max bit and works its way down
		unsigned __int64 n = 2199023255552;

		SetConsoleTextAttribute(hConsole, 266 | FOREGROUND_INTENSITY);
		cout << "_____________________________" << endl;

		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 7; j++)
			{
				SetConsoleTextAttribute(hConsole, 266 | FOREGROUND_INTENSITY);
				cout << "|";

				//if red_board or black_board has the n bit on, then it will display that peice
				if ((red_board | n) == red_board)
				{
					SetConsoleTextAttribute(hConsole, 268 | FOREGROUND_INTENSITY);
					cout << " O ";
				}

				else if ((black_board | n) == black_board)
				{
					SetConsoleTextAttribute(hConsole,  270 | FOREGROUND_INTENSITY);
					cout << " O ";
				}

				//print blank spot if no peice exists
				else
					cout << "   ";

				//shift eh bit each time to the right
				n = n >> 1;
			}

			SetConsoleTextAttribute(hConsole, 266 | FOREGROUND_INTENSITY);
			cout << "|\n-----------------------------" << endl;
		}

		cout << "  6   5   4   3   2   1   0" << endl;

		SetConsoleTextAttribute(hConsole, 263 | FOREGROUND_INTENSITY);
	}

	//checks to see if the passed board contains a win
	//can be implemented more optimally, chose to go with the safe option due to time concerns
	bool w(unsigned __int64 board)
	{
		//the base horizontal, verticle, diagonal, and reverse diagonal wins
		unsigned __int64 h = 15;
		unsigned __int64 v = 2113665;
		unsigned __int64 d = 16843009;
		unsigned __int64 o = 17043520;

		//itterates through every possible horizontal win and tests
		for (int x = 0; x < 6; x++)
		{
			for (int i = 0; i < 4; i++)
			{
				if ((board | h) == board)
					return true;

				h = h << 1;
			}

			h = h << 3;
		}

		//ittereates through every possible verticle win and tests
		for (int i = 0; i < 21; i++)
		{
			if ((board | v) == board)
				return true;

			v = v << 1;
		}

		//itterates through every possible diagonal win and tests
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				if ((board | d) == board)
					return true;

				d = d << 1;
			}

			d = d << 4;
		}

		//itterates through every possible reverse diagonal win and tests
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				if ((board | o) == board)
					return true;
				
				o = o >> 1;
			}

			o = o << 7;
		}


		return false; //if no win is found
	}

	//threaded game-tree with alpha-beta pruning.
	//can phase out some arguments in future implementation for time optimization
	int T(unsigned __int64 black, unsigned __int64 red, int ply, int level, char player, int alpha, int beta, int num)
	{
		int value;
		int temp;

		//if we are as deep as we need to go
		if (level == ply)
			return H(black, red);

		//sets up v for alpha-beta pruning
		if (player == BLACK)
			value = minim;

		else
			value = maxim;

		//itterates through each possible move on the board, tests if it's a valid move, makes the move, calls next-levels method,
		//when max level is acheived will return hueristic value and perform alpha-beta pruning on it.
		for (int i = 0; i < 7; i++)
		{
			if (nodes[num].colh[i] != 42) //move is valid
			{
				thread_insert(i, player, num);

				//if there is a win then this move is best
				if (w(nodes[num].yellow))
				{
					thread_remove(i, player, num);
					return maxim;
				}

				//if there is a lose then we want to prioritize 
				else if (w(nodes[num].red))
				{
					thread_remove(i, player, num);
					return minim;
				}

				//there are no wins
				else 
				{
					if (player == BLACK)
					{
						//call with incemented level and switched player
						temp = T(nodes[num].yellow, nodes[num].red, ply, level + 1, RED, alpha, beta, num);

						//code responsible for alpha-beta pruning
						if (temp > value)
						{
							value = temp;
							if (level == 1)
								node_values[num] = value;

							if (value > alpha)
								alpha = value;

							if (value >= beta)
							{
								thread_remove(i, player, num);
							    
								//if at top level of tree and can prune
								if (level == 1)
									node_values[num] = value;

								return value;
							}
						}
					}

					else //if player is red
					{
						temp = T(nodes[num].yellow, nodes[num].red, ply, level + 1, BLACK, alpha, beta, num);

						if (temp < value)
						{
							value = temp;

							if (level == 1)
								node_values[num] = value;

							if (value < beta)
								beta = value;

							if (value <= alpha)
							{
								thread_remove(i, player, num);
								if (level == 1)
									node_values[num] = value;

								return value;
							}
						}
					}

					thread_remove(i, player, num);

				}
			}
		}

		if (level == 1)
		{
			if (player == BLACK)
				node_values[num] = value;
			else
				node_values[num] = value;
		}

		return value;
	}

	//the hueristic function for analyzing non-win game boards
	//can be optimized, but due to time constraints will be delayed
	int H(unsigned __int64 b1, unsigned __int64 b2)
	{
		int hvalue = 0;

		//all possible three in a row combinations for horizontal, verticle, diagonal, and reverse-diagonal
		unsigned __int64 hpatterns[4] = { 7, 11, 13, 14 };
		unsigned __int64 vpatterns[3] = { 16513, 2113664, 270548992 };
		unsigned __int64 dapatterns[4] = { 65793, 16842753, 16777473, 16843008 };
		unsigned __int64 dbpatterns[4] = { 16843008, 17039424, 266304, 16781376 };

		//baselines for four-in-a-row
		unsigned __int64 hc = 15;
		unsigned __int64 vc = 2113665;
		unsigned __int64 dac = 16843009;
		unsigned __int64 dbc = 17043520;

		//First board's horizontals
		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				for (int w = 0; w < 4; w++)
					if (!(hc & b2) && ((b1 | hpatterns[w]) == b1))
						hvalue += 100;

				for (int w = 0; w < 4; w++)
					hpatterns[w] = hpatterns[w] << 1;

				hc = hc << 1;

			}

			for (int w = 0; w < 4; w++)
				hpatterns[w] = hpatterns[w] << 3;

			hc = hc << 3;
		}

		//Check first board's verticles
		for (int i = 0; i < 7; i++)
		{
			for (int w = 0; w < 3; w++)
			{
				if (!(vc & b2) && ((b1 | vpatterns[w]) == b1))
					hvalue += 100;

				vpatterns[w] = vpatterns[w] << 1;
				vc = vc << 1;
			}
		}

		//Check first board's first diagonal
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				for (int w = 0; w < 4; w++)
				{
					if (!(dac & b2) && ((b1 | dapatterns[w]) == b1))
						hvalue += 100;

					dapatterns[w] = dapatterns[w] << 1;
					dac = dac << 1;
				}
			}

			for (int w = 0; w < 3; w++)
				dapatterns[w] = dapatterns[w] << 4;

			dac = dac << 4;
		}

		//Check first board's second diagonal
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				for (int w = 0; w < 4; w++)
				{
					if (!(dbc & b2) && ((b1 | dbpatterns[w]) == b1))
						hvalue += 100;

					dbpatterns[w] = dbpatterns[w] >> 1;
					dbc = dbc >> 1;
				}
			}

			for (int w = 0; w < 3; w++)
				dbpatterns[w] = dbpatterns[w] << 10;

			dbc = dbc << 10;
		}


		//resetting values
		for (int i = 0; i < 4; i++)
			hpatterns[i] = hpatterns[i] >> 42;

		for (int i = 0; i < 3; i++)
			vpatterns[i] = vpatterns[i] >> 21;

		for (int i = 0; i < 4; i++)
			dapatterns[i] = dapatterns[i] >> 18;

		for (int i = 0; i < 4; i++)
			dbpatterns[i] = dbpatterns[i] << 12;

		hc = 15;
		vc = 2113665;
		dac = 16843009;
		dbc = 17043520;

		//Second board's horizontals
		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				for (int w = 0; w < 4; w++)
					if (!(hc & b1) && ((b2 | hpatterns[w]) == b2))
						hvalue -= 100;

				for (int w = 0; w < 4; w++)
					hpatterns[w] = hpatterns[w] << 1;

				hc = hc << 1;

			}

			for (int w = 0; w < 4; w++)
				hpatterns[w] = hpatterns[w] << 4;

			hc = hc << 4;
		}

		//Check second board's verticles
		for (int i = 0; i < 21; i++)
		{
			for (int w = 0; w < 3; w++)
			{
				if (!(vc & b1) && ((b2 | vpatterns[w]) == b2))
					hvalue -= 100;

				vpatterns[w] = vpatterns[w] << 1;
				vc = vc << 1;
			}
		}

		//Check second board's first diagonal
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				for (int w = 0; w < 4; w++)
				{
					if (!(dac & b1) && ((b2 | dapatterns[w]) == b2))
						hvalue -= 100;

					dapatterns[w] = dapatterns[w] << 1;
					dac = dac << 1;
				}
			}

			for (int w = 0; w < 3; w++)
				dapatterns[w] = dapatterns[w] << 3;

			dac = dac << 3;
		}

		//Check second board's second diagonal
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				for (int w = 0; w < 4; w++)
				{
					if (!(dbc & b1) && ((b2 | dbpatterns[w]) == b2))
						hvalue -= 100;

					dbpatterns[w] = dbpatterns[w] >> 1;
					dbc = dbc >> 1;
				}
			}

			for (int w = 0; w < 3; w++)
				dbpatterns[w] = dbpatterns[w] << 10;

			dbc = dbc << 10;
		}

		return hvalue;
	}

	//used in main when red_board is not accessible
	bool redw()
	{
		return w(red_board);
	}

	//used in main when black_board is not accessible
	bool blackw()
	{
		return w(black_board);
	}
	
	//checks to make sure that there isn't a common sense loss, if there is loss then will make the move the prevents loss
	//if there is win then will priorize that move, if none then will begin threads
	void run()
	{
		bool mademove = false;

		if (first_move_block && ((black_board & 8) == black_board))
		{
			insert(4, BLACK);
			first_move_block = false;
		}
		else
		{
			for (int i = 0; i < 7; i++)
			{
				if (col_heights[i] != 42)
				{
					insert(i, BLACK);

					if (w(black_board))
					{
						mademove = true;
						break;
					}

					else
						remove(i, BLACK);

					insert(i, RED);

					if (w(red_board))
					{
						remove(i, RED);
						insert(i, BLACK);
						mademove = true;
						break;
					}

					else
						remove(i, RED);
				}
			}

			if (!mademove)
			{
				for (int i = 0; i < 7; i++)
				{
					if (col_heights[i] != 42)
					{
						nodes[i].yellow = black_board;
						nodes[i].red = red_board;

						for (int x = 0; x < 7; x++)
							nodes[i].colh[x] = col_heights[x];

						node_values[i] = 0;
						thread_insert(i, BLACK, i);
						nodes[i].c = thread(&Connect4::T, this, nodes[i].yellow, nodes[i].red, 10, 1, RED, maxim, minim, i);
					}
				}

				for (int i = 0; i < 7; i++)
				{
					if (col_heights[i] != 42)
						nodes[i].c.join();
					thread_remove(i, BLACK, i);
				}

				best;
				int best_hvalue;

				//to make sure that a node that cant be used doesnt end up being used
				for (int i = 0; i < 7; i++)
					if (col_heights[i] != 42)
					{
						best_hvalue = node_values[i];
						best = i;
					}

				for (int i = 1; i < 7; i++)
				{
					if (node_values[i] > best_hvalue && (col_heights[i] != 42))
					{
						best = i;
						best_hvalue = node_values[i];
					}
				}

				insert(best, BLACK);
			}
		}
	}

	//for printing moves
	int B()
	{
		return best;
	}
};

int main()
{
	Connect4 game;
	int move;

	//while the game is active
	while (!game.redw() && !game.blackw() && (game.moves() > 0))
	{
		game.display();
		cout << endl << "Enter your move: ";
		cin >> move;

		if (game.validmove(move))
		{
			game.insert(move, RED);


			if (game.redw())
			{
				cout << "Player has won!!" << endl;
				system("pause");
				return 0;
			}

			game.run();
			system("cls");

			if (game.blackw())
			{
				game.display();
				cout << "Computer has won!!" << endl;
				system("pause");
				return 0;
			}

			else
				cout << "Computer made move " << game.B() << endl;
		}

		else
		{
			system("cls");
			cout << "Move wasn't valid, make another one." << endl;
		}
	}

	if (game.moves() == 0)
		cout << "Game was a tie..." << endl;
	
	system("pause");
	return 0;
}

