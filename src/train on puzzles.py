
''' do some preliminary work where we populate some arrays of helpers so we can reduce the amount of repeat work we do '''
rows = [[y for y in range(81)][x*9:(x+1)*9] for x in range(9)]
cols = [[y for y in range(81)][x::9] for x in range(9)]

blks = []
for R in range(3):
	for C in range(3):
		blk = []
		for r in range(3):
			for c in range(3):
				blk.append(R*3*9 + C*3 + r*9 + c)
		blks.append(blk)

#print ('\n'.join([str(x) for x in rows]))
#print ('\n'.join([str(x) for x in cols]))
#print ('\n'.join([str(x) for x in blks]))

membership = []
for i in range(81):
	''' find out what groups i'm part of and store them as tuples (r,c,g) 
		then you find the available pencilmarks for each r,c,g and for each of the 81 positions, you do the r,c,g tuple.  great :) '''

	r = i//9
	c = i%9
	b = 0
	for j in range(9):
		if set(blks[j]).issuperset([i]):
			b = j
	membership.append((r,c,b))

#print ('\n'.join([str(x) for x in membership]))

from keras.models import Model
from keras.layers import Input
from keras.layers import Conv2D, Conv3D, Add
from keras.layers import BatchNormalization, Activation, Dropout, Reshape

def build_sudoku_model(filters, kernels, blocks, dilations=[(1,1),(1,1)]):
	
	input = Input(shape=(9,9,9))

	x = Conv2D(filters=filters, kernel_size=kernels, padding='same')(input)
	x = BatchNormalization()(x)
	x = Activation('relu')(x)
	x = Dropout(0.2)(x)
	
	for _ in range(blocks):
		y = x
		for dilation in dilations:
			y = Conv2D(filters=filters, kernel_size=kernels, padding='same', dilation_rate=dilation)(y)
			y = BatchNormalization()(y)
			y = Activation('relu')(y)
			y = Dropout(0.2)(y)
		x = Add()([x,y])

	output = Conv2D(filters=9, kernel_size=(1,1), padding='same', activation='softmax')(x)

	model = Model(inputs=input, outputs=output)
	model.compile(loss='categorical_crossentropy', optimizer='adam', metrics=['accuracy'])
	model.summary()
	return model

def build_3D_sudoku_model(filters, kernels, blocks, dilations=[(1,1,1),(1,1,1)]):
	
	input = Input(shape=(9,9,9))

	x = Reshape((9,9,9,1))(input)
	x = Conv3D(filters=filters, kernel_size=kernels, padding='same')(x)
	x = BatchNormalization()(x)
	x = Activation('relu')(x)
	x = Dropout(0.2)(x)
	
	for _ in range(blocks):
		y = x
		for dilation in dilations:
			y = Conv3D(filters=filters, kernel_size=kernels, padding='same', dilation_rate=dilation)(y)
			y = BatchNormalization()(y)
			y = Activation('relu')(y)
			y = Dropout(0.2)(y)
		x = Add()([x,y])

	x = Conv3D(filters=1, kernel_size=(1,1,1), padding='same', activation='relu')(x)
	x = Reshape((9,9,9))(x)
	output = Conv2D(filters=9, kernel_size=(1,1), padding='same', activation='softmax')(x)

	model = Model(inputs=input, outputs=output)
	model.compile(loss='categorical_crossentropy', optimizer='adam', metrics=['accuracy'])
	model.summary()
	return model

import numpy as np
from numpy.random import permutation, normal
from numpy.random import random as npr

def create_random_homomorphism(line):
	sbox = {'0':'0','\t':'\t'}
	sbox.update({str(k):str(v) for k,v in zip(range(1,10),permutation(range(1,10)))})
	return ''.join([sbox[x] for x in line])

def create_puzzle_solution_pair(line, predictability):
	homo = create_random_homomorphism(line)
	puzzle, solution = list(homo.split()[0]), list(homo.split()[1])
	mutable = [p != s for p,s in zip(puzzle,solution)]
	for i in range(len(mutable)):
		if mutable[i]:
			if npr() < predictability:
				# it only appears in the solution and should only appear in the solution
				pass
			else:
				# it only appears in the solution, but should only appear in the puzzle
				puzzle[i] = solution[i]
		'''
				solution[i] = '0'
		else:
			# it must appear in the puzzle, but currently appears in both, so remove it from the solution
			solution[i] = '0'
		'''
		
	return [int(x) for x in puzzle], [int(x) for x in solution]

def generate_pencil_marks(puzzle):

	# generate pencil marks for rows, cols, and blocks, then "and" them together for each position
	row_marks = [set(range(9)) - set([np.argmax(puzzle[x//9][x%9]) for x in r if np.max(puzzle[x//9][x%9]) == 1]) for r in rows]
	col_marks = [set(range(9)) - set([np.argmax(puzzle[x//9][x%9]) for x in c if np.max(puzzle[x//9][x%9]) == 1]) for c in cols]
	blk_marks = [set(range(9)) - set([np.argmax(puzzle[x//9][x%9]) for x in b if np.max(puzzle[x//9][x%9]) == 1]) for b in blks]

	pencil_marks = np.zeros((9,9,9))
	for m in range(81):
		r, c = m//9, m%9
		for i in row_marks[membership[m][0]].intersection(col_marks[membership[m][1]]).intersection(blk_marks[membership[m][2]]):
			pencil_marks[r][c][i] = 1
	
	return pencil_marks + puzzle

def to_sparse(data):
	frame = np.zeros((9,9,9),dtype=np.int8)
	for i in range(81):
		if data[i]:
			r = i//9
			c = i%9
			frame[r][c][data[i]-1] = 1
	return frame

def create_dataset(source, samples, for_validation=False):
	puzzles = np.zeros((samples, 9, 9, 9),dtype=np.int8)
	solutions = np.zeros((samples, 9, 9, 9),dtype=np.int8)
	pencilmarks = np.zeros((samples, 9, 9, 9),dtype=np.int8)

	for s in range(samples):
		puzzle, solution = create_puzzle_solution_pair(source[int(npr()*len(source))],int(for_validation)+max(0,npr()))
		puzzles[s] = to_sparse(puzzle)
		solutions[s] = to_sparse(solution)
		pencilmarks[s] = generate_pencil_marks(puzzles[s])

	return puzzles, solutions, pencilmarks

def validate_predictions(puzzles, solutions, pencilmarks, model):
	predicted = model.predict(pencilmarks)

	predictable = 0
	accurate_predictions = 0
	for p in range(len(puzzles)):
		for r in range(9):
			for c in range(9):
				if sum(puzzles[p][r][c]) == 0:
					if np.argmax(solutions[p][r][c]) == np.argmax(predicted[p][r][c]):
						accurate_predictions += 1
					predictable += 1

	return accurate_predictions / predictable

def autoregressive_validation(puzzles, solutions, pencilmarks, model):
	def number_of_unpredicted(puzzle):
		unpredicted = 0
		for r in range(9):
			for c in range(9):
				if sum(puzzle[r][c]) == 0:
					unpredicted += 1
		return unpredicted

	def get_max_prediction(puzzle, prediction):
		predictions = []
		for r in range(9):
			for c in range(9):
				if sum(puzzle[r][c]) == 0:
					predictions.append((max(prediction[r][c]),(r,c),np.argmax(prediction[r][c])))
		predictions = list(reversed(sorted(predictions)))
		return predictions[0][1], predictions[0][2]

	predictable = 0
	accurate_predictions = 0
	for p in range(len(puzzles)):
		working_puzzle = np.copy(puzzles[p])
		
		while number_of_unpredicted(working_puzzle):
			(r,c), value = get_max_prediction(working_puzzle, model.predict(np.array([generate_pencil_marks(working_puzzle)]))[0])
			working_puzzle[r][c][value] = 1
		
		for r in range(9):
			for c in range(9):
				if sum(puzzles[p][r][c]) == 0:
					if np.argmax(solutions[p][r][c]) == np.argmax(working_puzzle[r][c]):
						accurate_predictions += 1
					predictable += 1
					
	return accurate_predictions / predictable

model = build_3D_sudoku_model(64, (3,3,3), 5)
#model = build_sudoku_model(64, (3,3), 5)
#model = build_sudoku_model(128, (3,3), 5)
#model = build_sudoku_model(256, (3,3), 20)

solved_puzzles = open('valid puzzles','r').read().split('\n')[:-1]

best_ar = 0
herstory = {'predictive_validity':[],'autoregressive_validation':[]}
for e in range(1,1000):
	puzzles, solutions, pencilmarks = create_dataset(solved_puzzles, 100000)
	history = model.fit(pencilmarks, solutions, epochs=1, verbose=1, validation_split=0.10)

	puzzles, solutions, pencilmarks = create_dataset(solved_puzzles, 1000, True)
	herstory['predictive_validity'] += [validate_predictions(puzzles, solutions, pencilmarks, model)]

	puzzles, solutions, pencilmarks = create_dataset(solved_puzzles, 100, True)
	herstory['autoregressive_validation'] += [autoregressive_validation(puzzles, solutions, pencilmarks, model)]

	for key,value in history.history.items():
		if key in herstory:
			herstory[key] += value
		else:
			herstory[key] = value
	
	for key,value in herstory.items():
		if key.find('val') != -1:
			print (key,' '.join([str(x)[:6] for x in value]))

	if best_ar < herstory['autoregressive_validation'][-1]:
		best_ar = herstory['autoregressive_validation'][-1]
		model.save('model - ar='+str(best_ar)[:6])
		print ('model saved')








