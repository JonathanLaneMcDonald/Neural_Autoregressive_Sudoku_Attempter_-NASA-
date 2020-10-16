
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

std::string empty_puzzle = "000000000000000000000000000000000000000000000000000000000000000000000000000000000";

bool update_is_valid(std::vector<int>& puzzle, int position)
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

std::vector<int> update(std::vector<int>& puzzle, int position, int value)
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

std::string puzzle_to_string(std::vector<int> puzzle)
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

std::vector<int> backtrack(std::vector<int> solution)
{
	std::random_device rd;
	std::mt19937 g(rd());

	std::vector<std::vector<int>> puzzle = {solution};

	int current_move = 0;
	std::vector<int> eligible_moves;
	for (int i = 0; i < 81; i ++)
		eligible_moves.push_back(i);
	std::shuffle(eligible_moves.begin(), eligible_moves.end(), g);
	while (current_move < eligible_moves.size() && 5 < eligible_moves.size())
	{
		int position = eligible_moves[current_move];
		int value = puzzle[puzzle.size()-1][position];
		auto new_puzzle = puzzle[puzzle.size()-1];
		new_puzzle[position] = 0;
		if (brutish_solver(puzzle_prep(puzzle_to_string(new_puzzle))).size() == 1)
		{
			puzzle.push_back(new_puzzle);
			eligible_moves.erase(std::remove_if(eligible_moves.begin(), eligible_moves.end(), [position](int x){return x == position;}), eligible_moves.end());
			std::shuffle(eligible_moves.begin(), eligible_moves.end(), g);
			current_move = 0;
		}
		else
		{
			current_move ++;
		}
	}
	std::cout << eligible_moves.size() << ' ' << puzzle.size() << ' ' << puzzle_to_string(puzzle[puzzle.size()-1]) << std::endl;
	return puzzle[puzzle.size()-1];
}

void generate(int threadID, std::atomic_int* puzzle_count, int num_of_puzzles, int seeds_required, std::ofstream* outfile, std::mutex* mutex)
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

				auto base_puzzle = puzzle_to_string(backtrack(solutions[selection]));

				mutex->lock();
				*outfile << base_puzzle << '\t';
				for (int i : solutions[selection])
					*outfile << i;
				*outfile << std::endl;
				mutex->unlock();

				if (++(*puzzle_count) % 1000 == 0)
					std::cout << threadID << "\t" << *puzzle_count << std::endl;
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
	std::atomic_int* puzzle_count = new std::atomic_int(0);
	std::ofstream* outfile = new std::ofstream("valid puzzles");
	std::mutex* mutex = new std::mutex;

	//generate(0, 1000000, 27, outfile, mutex);

	std::vector<std::thread> workers(0);

	int threads = 4;
	for (int i = 0; i < threads; i++)
		workers.emplace_back(std::thread(generate, i, puzzle_count, 100000, 27, outfile, mutex));

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
