
// g++ generate.cpp -std=c++17 -lpthread -o generate -O3

#include "groups.h"

#include <atomic>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>

#define number_mask 15	//(1<<4)-1
#define pencil_mask 8176//(1<<13)-1-number_mask

static std::string empty_puzzle = "000000000000000000000000000000000000000000000000000000000000000000000000000000000";

/*
Determine whether the value in the specified position invalidates the puzzle by checking to make sure that all 
uncommitted positions have possible values and that no committed positions equal the newly placed value.

@param		puzzle			is a const ref to a puzzle
@param		position		is the newly filled position
*/
bool update_is_valid(const std::vector<int>& puzzle, int position)
{
	int value = puzzle[position];

	for(int i : rows[mems[position].row])
		if (i != position && (puzzle[i] == 0 || puzzle[i] == value))
			return false;
	for(int i : cols[mems[position].col])
		if (i != position && (puzzle[i] == 0 || puzzle[i] == value))
			return false;
	for(int i : blks[mems[position].blk])
		if (i != position && (puzzle[i] == 0 || puzzle[i] == value))
			return false;

	return true;
}

/*
Copy/update an existing puzzle and update the pencilmarks in the new puzzle.

@param		puzzle			is a const ref to a puzzle
@param		position		is the target position
@param		value			is the value to place in that position
*/
std::vector<int> update(const std::vector<int>& puzzle, int position, int value)
{
	std::vector<int> new_puzzle(puzzle);
	new_puzzle[position] = value;

	for(int i : rows[mems[position].row])
		new_puzzle[i] &= ~(1<<(3+value));
	for(int i : cols[mems[position].col])
		new_puzzle[i] &= ~(1<<(3+value));
	for(int i : blks[mems[position].blk])
		new_puzzle[i] &= ~(1<<(3+value));

	return new_puzzle;
}

/*
Recursively solve a given puzzle using brute force and return a vector of all valid solutions.

@param		puzzle			is the current state of the puzzle
*/
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

/*
Prepare a playable puzzle from a string of a puzzle

@param		puzzle_as_string		is a string representation of the current state of the puzzle
*/
std::vector<int> puzzle_prep(const std::string puzzle_as_string = empty_puzzle)
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

/*
Prepare a string representation from a given, playable puzzle

@param		puzzle			is a const ref to a playable puzzle
*/
std::string puzzle_to_string(const std::vector<int>& puzzle)
{
	std::string string;
	string.reserve(81);

	for(int i : puzzle)
	{
		if (i&number_mask)
			string += char(48+i);
		else
			string += '0';
	}

	return string;
}

/*
Given a solved sudoku puzzle, randomly remove numbers until there is more than one solution.

@param		solution		is a const ref to a solved sudoku puzzle
@param		minimum_hints	is the minimum number of hints that should exist in the puzzle
*/
std::vector<int> backtrack(const std::vector<int>& solution)
{
	std::random_device rd;
	std::mt19937 g(rd());

	std::vector<std::vector<int>> puzzle = {solution};

	std::vector<int> eligible_moves;
	for (int i = 0; i < 81; i ++)
		eligible_moves.push_back(i);

	while (eligible_moves.size())
	{
		std::shuffle(eligible_moves.begin(), eligible_moves.end(), g);

		int position = eligible_moves.front();
		auto new_puzzle = puzzle.back();
		new_puzzle[position] = 0;
		if (brutish_solver(puzzle_prep(puzzle_to_string(new_puzzle))).size() == 1)
		{
			puzzle.push_back(new_puzzle);
		}
		std::rotate(eligible_moves.begin(), eligible_moves.begin() + 1, eligible_moves.end());
		eligible_moves.pop_back();
	}
	//std::cout << eligible_moves.size() << ' ' << puzzle.size() << ' ' << puzzle_to_string(puzzle.back()) << std::endl;
	return puzzle.back();
}

/*
Generate puzzles by randomly placing numbers in an empty puzzle and backtracking from a random solution

@param		threadID		the ID of the current thread
@param		puzzle_count	the number of puzzles generated so far across all threads
@param		num_of_puzzles	the number of puzzles we want to generate in total
@param		seeds_required	the number of seed values to place when initializing a puzzle
@param		outfile			the file to which puzzles and solutions are written
@param		mutex			make sure we don't talk over each other on the shared ofstream
*/
void generate(int threadID, std::shared_ptr<std::atomic_int> puzzle_count, int num_of_puzzles,
	int seeds_required, std::shared_ptr<std::ofstream> outfile, std::shared_ptr<std::mutex> mutex)
{
	mutex->lock();
	std::cout << "thread " << threadID << ": reporting for duty" << std::endl;
	mutex->unlock();

	std::random_device rd;
    std::mt19937 _gen(rd());
    std::uniform_real_distribution<> _dis(0.0, 1.0);

	while (*puzzle_count < num_of_puzzles)
	{
		std::vector<std::vector<int>> puzzle = {puzzle_prep()};

		int attempts = 0;
		int seeds_sewn = 0;
		while (seeds_sewn < seeds_required && attempts < 1000)
		{
			int position = int(81*_dis(_gen));
			if (puzzle.back()[position] & number_mask)
			{
				//pass
			}
			else
			{
				puzzle.push_back(update(puzzle.back(), position, (seeds_sewn % 9)+1));
				if (update_is_valid(puzzle.back(), position))
					seeds_sewn ++;
				else
					puzzle.pop_back();
			}
			attempts ++;
		}
		if (seeds_sewn == seeds_required)
		{
			auto solutions = brutish_solver(puzzle.back());
			if (!solutions.empty())
			{
				int selection = int(solutions.size()*_dis(_gen));

				auto base_puzzle = puzzle_to_string(backtrack(solutions[selection]));

				mutex->lock();
				*outfile << base_puzzle << '\t';
				for (int i : solutions[selection])
					*outfile << i;
				*outfile << std::endl;
				mutex->unlock();

				std::cout << threadID << "\t" << ++(*puzzle_count) << "\t" << base_puzzle << std::endl;
			}
		}
	}
}

int main()
{
	int puzzles_to_create = 100000;
	int seed_numbers = 27;

	std::shared_ptr<std::atomic_int> puzzle_count = std::make_shared<std::atomic_int>(0);
	std::shared_ptr<std::ofstream> outfile = std::make_shared<std::ofstream>("valid puzzles");
	std::shared_ptr<std::mutex> mutex = std::make_shared<std::mutex>();

	std::vector<std::thread> workers(0);

	int threads = 1;
	for (int i = 0; i < threads; i++)
		workers.emplace_back(std::thread(generate, i, puzzle_count, puzzles_to_create, seed_numbers, outfile, mutex));

	for (int i = 0; i < workers.size(); i++)
		workers[i].join();

	outfile->close();

	return 0;
}
