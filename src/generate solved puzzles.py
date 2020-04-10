
''' do some preliminary work where we populate some arrays of helpers so we can reduce the amount of repeat work we do '''
rows = [[y for y in range(81)][x*9:(x+1)*9] for x in range(9)]
cols = [[y for y in range(81)][x::9] for x in range(9)]

groups = []
for R in range(3):
	for C in range(3):
		group = []
		for r in range(3):
			for c in range(3):
				group.append(R*3*9 + C*3 + r*9 + c)
		groups.append(group)

#print ('\n'.join([str(x) for x in rows]))
#print ('\n'.join([str(x) for x in cols]))
#print ('\n'.join([str(x) for x in groups]))

membership = []
for i in range(81):
	''' find out what groups i'm part of and store them as tuples (r,c,g) 
		then you find the available pencilmarks for each r,c,g and for each of the 81 positions, you do the r,c,g tuple.  great :) '''

	r = i//9
	c = i%9
	g = 0
	for j in range(9):
		if set(groups[j]).issuperset([i]):
			g = j
	membership.append((r,c,g))

#print ('\n'.join([str(x) for x in membership]))

import numpy as np
from numpy.random import random as npr
from numpy.random import permutation
from copy import copy

number_mask = 2**4-1
pencil_mask = (2**13-1) - number_mask

print (number_mask)
print (pencil_mask)

def puzzlesea(puzzle):
	for i in range(81):
		s = ''
		for j in range(1,10):
			if puzzle[i] & (1<<(3+j)):
				s += str(j)
			else:
				s += '_'
		s += '('+str(puzzle[i]&number_mask)+','+str(puzzle[i])+')\t'
		print (s,end='')
		if (i+1) % 9 == 0:
			print ()

def update_is_valid(puzzle, position):
	
	value = puzzle[position]
	
	r, c, g = membership[position]
	for i in rows[r]:
		if i != position and (puzzle[i] == 0 or puzzle[i] == value):
			return False
	for i in cols[c]:
		if i != position and (puzzle[i] == 0 or puzzle[i] == value):
			return False
	for i in groups[g]:
		if i != position and (puzzle[i] == 0 or puzzle[i] == value):
			return False

	return True

def update(puzzle, position, value):
	new_puzzle = np.copy(puzzle)
	new_puzzle[position] = value
	
	r, c, g = membership[position]
	for i in rows[r]:
		new_puzzle[i] &= ~(1<<(3+value))
	for i in cols[c]:
		new_puzzle[i] &= ~(1<<(3+value))
	for i in groups[g]:
		new_puzzle[i] &= ~(1<<(3+value))

	return new_puzzle

def brutish_solver(puzzle):
	solutions = []

	recursible = False
	for position in range(81):
		marks = [x for x in range(1,10) if puzzle[position] & (1<<(3+x))]
		a_mark_was_valid = False
		for mark in marks:
			puzzle[position] &= ~(1<<(3+mark))
			new_puzzle = update(puzzle, position, mark)
			if update_is_valid(new_puzzle, position):
				a_mark_was_valid = True
				solutions += brutish_solver(new_puzzle)
			recursible = True
		if not a_mark_was_valid and not puzzle[position] & number_mask:
			return solutions
	
	if not recursible:
		valid_puzzle = ''.join([str(x) if int(x&number_mask) else '0' for x in puzzle])
		#print (valid_puzzle)
		return [valid_puzzle]
	else:
		return solutions

def puzzle_prep(puzzle_as_string = '0'*81):
	puzzle = np.zeros(81,dtype=np.int) + pencil_mask
	
	for i in range(len(puzzle_as_string)):
		if int(puzzle_as_string[i]):
			puzzle = update(puzzle, i, int(puzzle_as_string[i]))
	
	#print ('\n'.join([str(x) for x in puzzle]))
	
	return puzzle

def generate(puzzle_target, seeds_required, savefile):
	puzzle_file = open(savefile,'w')
	valid_puzzles = []
	while len(valid_puzzles) < puzzle_target:
		puzzle = [puzzle_prep()]
		seeds_sewn = 0
		while seeds_sewn < seeds_required:
			position = int(npr()*81)
			if puzzle[-1][position] & number_mask:
				pass
			else:
				puzzle.append(update(puzzle[-1], position, (seeds_sewn%9) + 1))
				if update_is_valid(puzzle[-1], position):
					seeds_sewn += 1
				else:
					puzzle.pop()
		solutions = brutish_solver(puzzle[-1])
		if len(solutions):
			valid_puzzles.append(permutation(solutions)[0])
			puzzle_file.write(valid_puzzles[-1]+'\n')
			print (len(valid_puzzles))
	puzzle_file.close()
	return valid_puzzles

def puzzle_to_string(puzzle):
	return ''.join([str(x) if x&number_mask else '0' for x in puzzle])

def backtrack(valid_puzzle):
	
	# state, position, value
	puzzle = [(valid_puzzle, None, None)]

	current_move = 0
	eligible_moves = permutation(list(range(81)))
	while current_move < len(eligible_moves):
		position = eligible_moves[current_move]
		value = puzzle[-1][0][position]
		new_puzzle = np.copy(puzzle[-1][0])
		new_puzzle[position] = 0
		if len(brutish_solver(puzzle_prep(puzzle_to_string(new_puzzle)))) == 1:
			print (81-len(puzzle),len(puzzle),current_move,puzzle_to_string(new_puzzle))
			puzzle.append((new_puzzle,position,value))
			eligible_moves = permutation(list(set(eligible_moves)-set([position])))
			current_move = 0
		else:
			current_move += 1
			print (current_move)
	return puzzle_to_string(puzzle[-1][0])

from sys import argv

if len(argv) < 2:
	puzzles_with_solutions = open('puzzles with solutions','w')
	valid_puzzles = permutation([x for x in open('valid puzzles','r').read().split('\n') if len(x) == 81])
	for solution in valid_puzzles:
		puzzle = backtrack(puzzle_prep(solution))
		puzzles_with_solutions.write(puzzle + '\t' + solution + '\n')
	puzzles_with_solutions.close()
else:
	print ('\n'.join(brutish_solver(puzzle_prep(argv[1]))))

