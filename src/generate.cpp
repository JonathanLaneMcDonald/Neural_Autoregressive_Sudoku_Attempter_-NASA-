
// g++ generate.cpp -std=c++17 -lpthread -o generate -O3

#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

std::vector<std::vector<int>> rows = {	{ 0, 1, 2, 3, 4, 5, 6, 7, 8},
										{ 9,10,11,12,13,14,15,16,17},
										{18,19,20,21,22,23,24,25,26},
										{27,28,29,30,31,32,33,34,35},
										{36,37,38,39,40,41,42,43,44},
										{45,46,47,48,49,50,51,52,53},
										{54,55,56,57,58,59,60,61,62},
										{63,64,65,66,67,68,69,70,71},
										{72,73,74,75,76,77,78,79,80}	};

std::vector<std::vector<int>> cols = {	{ 0, 9,18,27,36,45,54,63,72},
										{ 1,10,19,28,37,46,55,64,73},
										{ 2,11,20,29,38,47,56,65,74},
										{ 3,12,21,30,39,48,57,66,75},
										{ 4,13,22,31,40,49,58,67,76},
										{ 5,14,23,32,41,50,59,68,77},
										{ 6,15,24,33,42,51,60,69,78},
										{ 7,16,25,34,43,52,61,70,79},
										{ 8,17,26,35,44,53,62,71,80}	};

std::vector<std::vector<int>> blks = {	{ 0, 1, 2, 9,10,11,18,19,20},
										{ 3, 4, 5,12,13,14,21,22,23},
										{ 6, 7, 8,15,16,17,24,25,26},
										{27,28,29,36,37,38,45,46,47},
										{30,31,32,39,40,41,48,49,50},
										{33,34,35,42,43,44,51,52,53},
										{54,55,56,63,64,65,72,73,74},
										{57,58,59,66,67,68,75,76,77},
										{60,61,62,69,70,71,78,79,80}	};

struct Membership
{
public:
	Membership(int r, int c, int b)
	: _r(r)
	, _c(c)
	, _b(b)
	{

	}

	int _r;
	int _c;
	int _b;
};

std::vector<Membership> mems = {	
									Membership(0,0,0),
									Membership(0,1,0),
									Membership(0,2,0),
									Membership(0,3,1),
									Membership(0,4,1),
									Membership(0,5,1),
									Membership(0,6,2),
									Membership(0,7,2),
									Membership(0,8,2),
									Membership(1,0,0),
									Membership(1,1,0),
									Membership(1,2,0),
									Membership(1,3,1),
									Membership(1,4,1),
									Membership(1,5,1),
									Membership(1,6,2),
									Membership(1,7,2),
									Membership(1,8,2),
									Membership(2,0,0),
									Membership(2,1,0),
									Membership(2,2,0),
									Membership(2,3,1),
									Membership(2,4,1),
									Membership(2,5,1),
									Membership(2,6,2),
									Membership(2,7,2),
									Membership(2,8,2),
									Membership(3,0,3),
									Membership(3,1,3),
									Membership(3,2,3),
									Membership(3,3,4),
									Membership(3,4,4),
									Membership(3,5,4),
									Membership(3,6,5),
									Membership(3,7,5),
									Membership(3,8,5),
									Membership(4,0,3),
									Membership(4,1,3),
									Membership(4,2,3),
									Membership(4,3,4),
									Membership(4,4,4),
									Membership(4,5,4),
									Membership(4,6,5),
									Membership(4,7,5),
									Membership(4,8,5),
									Membership(5,0,3),
									Membership(5,1,3),
									Membership(5,2,3),
									Membership(5,3,4),
									Membership(5,4,4),
									Membership(5,5,4),
									Membership(5,6,5),
									Membership(5,7,5),
									Membership(5,8,5),
									Membership(6,0,6),
									Membership(6,1,6),
									Membership(6,2,6),
									Membership(6,3,7),
									Membership(6,4,7),
									Membership(6,5,7),
									Membership(6,6,8),
									Membership(6,7,8),
									Membership(6,8,8),
									Membership(7,0,6),
									Membership(7,1,6),
									Membership(7,2,6),
									Membership(7,3,7),
									Membership(7,4,7),
									Membership(7,5,7),
									Membership(7,6,8),
									Membership(7,7,8),
									Membership(7,8,8),
									Membership(8,0,6),
									Membership(8,1,6),
									Membership(8,2,6),
									Membership(8,3,7),
									Membership(8,4,7),
									Membership(8,5,7),
									Membership(8,6,8),
									Membership(8,7,8),
									Membership(8,8,8)	};

#define number_mask 15	//(1<<4)-1
#define pencil_mask 8176//(1<<13)-1-number_mask

std::string empty_puzzle = "000000000000000000000000000000000000000000000000000000000000000000000000000000000";

bool update_is_valid(std::vector<int>& puzzle, int position)
{
	int value = puzzle[position];

	for(int i : rows[mems[position]._r])
		if (i != position && (puzzle[i] == 0 || puzzle[i] == value))
			return false;
	for(int i : cols[mems[position]._c])
		if (i != position && (puzzle[i] == 0 || puzzle[i] == value))
			return false;
	for(int i : blks[mems[position]._b])
		if (i != position && (puzzle[i] == 0 || puzzle[i] == value))
			return false;

	return true;
}

std::vector<int> update(std::vector<int>& puzzle, int position, int value)
{
	std::vector<int> new_puzzle(puzzle);
	new_puzzle[position] = value;

	for(int i : rows[mems[position]._r])
		new_puzzle[i] &= ~(1<<(3+value));
	for(int i : cols[mems[position]._c])
		new_puzzle[i] &= ~(1<<(3+value));
	for(int i : blks[mems[position]._b])
		new_puzzle[i] &= ~(1<<(3+value));

	return new_puzzle;
}

std::vector<std::vector<int>> brutish_solver(std::vector<int> puzzle)
{
	std::vector<std::vector<int>> solutions;

	bool recursible = false;
	for (int position = 0; position < 81; position ++)
	{
		bool mark_was_valid = false;
		for (int mark = 1; mark < 10; mark ++)
		{
			if (puzzle[position] & (1<<(3+mark)))
			{
				puzzle[position] &= ~(1<<(3+mark));
				auto new_puzzle = update(puzzle, position, mark);

				if (update_is_valid(new_puzzle, position))
				{
					mark_was_valid = true;
					for (auto solution : brutish_solver(new_puzzle))
						solutions.push_back(solution);
				}

				recursible = true;
			}
		}

		if (!mark_was_valid && !(puzzle[position] & number_mask))
			return solutions;
	}

	if (!recursible)
		return {puzzle};
	else
		return solutions;
}

std::vector<int> puzzle_prep(std::string puzzle_as_string = empty_puzzle)
{
	std::vector<int> puzzle;
	for (int i = 0; i < 81; i ++)
		puzzle.push_back(pencil_mask);

	for (int i = 0; i < 81; i ++)
	{
		if (puzzle_as_string[i] != '0')
			puzzle = update(puzzle, i, puzzle_as_string[i]-48);
	}

	return puzzle;
}

void generate(int threadID, int num_of_puzzles, int seeds_required, std::ofstream* outfile, std::mutex* mutex)
{
	mutex->lock();
	std::cout << "thread " << threadID << ": reporting for duty" << std::endl;
	mutex->unlock();

	std::random_device rd;
    std::mt19937 _gen(rd());
    std::uniform_real_distribution<> _dis(0.0, 1.0);

	int puzzles_completed = 0;
	while (puzzles_completed < num_of_puzzles)
	{
		std::vector<std::vector<int>> puzzle = {puzzle_prep()};

		int attempts = 0;
		int seeds_sewn = 0;
		while (seeds_sewn < seeds_required && attempts < 1000)
		{
			int position = int(81*_dis(_gen));
			if (puzzle[puzzle.size()-1][position] & number_mask)
			{
				//pass
			}
			else
			{
				puzzle.push_back(update(puzzle[puzzle.size()-1], position, (seeds_sewn%9)+1));
				if (update_is_valid(puzzle[puzzle.size()-1], position))
					seeds_sewn ++;
				else
					puzzle.pop_back();
			}
			attempts ++;
		}
		if (seeds_sewn == seeds_required)
		{
			auto solutions = brutish_solver(puzzle[puzzle.size()-1]);
			if (!solutions.empty())
			{
				int selection = int(solutions.size()*_dis(_gen));

				mutex->lock();
				for (int i : solutions[selection])
					*outfile << i;
				*outfile << std::endl;
				mutex->unlock();

				if (++puzzles_completed % 1000 == 0)
					std::cout << threadID << "\t" << puzzles_completed << std::endl;
			}
		}
		else
		{
			std::cout << "attempts exceeded limit: ";
			for (int i : puzzle[puzzle.size()-1])
				if (i&number_mask)
					std::cout << i;
				else
					std::cout << '0';
			std::cout << std::endl;
		}		
	}
}

int main()
{
	std::mutex* mutex = new std::mutex;
	std::vector<std::thread> workers(0);
	std::ofstream* outfile = new std::ofstream("valid puzzles");

	generate(0, 1000000, 27, outfile, mutex);

	return 0;
}/*
	int threads = 4;
	for (int i = 0; i < threads; i++)
		workers.emplace_back(std::thread(generate, i, 100000, 27, outfile, mutex));

	for (int i = 0; i < workers.size(); i++)
		workers[i].join();

	outfile->close();

	return 0;
}
/*
	std::cout << number_mask << std::endl;
	std::cout << pencil_mask << std::endl;

	std::string puzzle = "000000000001020300020304050006030700030872040002090800060209080007060900000000000";
	brutish_solver(puzzle_prep(puzzle));
	return 0;
}*/
