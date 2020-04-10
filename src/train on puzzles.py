
from keras.models import Model
from keras.layers import Input
from keras.layers import Conv2D, Add
from keras.layers import BatchNormalization, Activation, Dropout

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

	output = Conv2D(filters=9, kernel_size=(1,1), padding='same', activation='sigmoid')(x)

	model = Model(inputs=input, outputs=output)
	model.compile(loss='binary_crossentropy', optimizer='adam', metrics=['accuracy'])
	model.summary()
	return model

import numpy as np
from numpy.random import permutation
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
				solution[i] = '0'
		else:
			# it must appear in the puzzle, but currently appears in both, so remove it from the solution
			solution[i] = '0'

	return [int(x) for x in puzzle], [int(x) for x in solution]

def to_sparse(data):
	frame = np.zeros((9,9,9),dtype=np.int8)
	for i in range(81):
		if data[i]:
			r = i//9
			c = i%9
			frame[r][c][data[i]-1] = 1
	return frame

def create_dataset(source, samples):
	puzzles = np.zeros((samples, 9, 9, 9),dtype=np.int8)
	solutions = np.zeros((samples, 9, 9, 9),dtype=np.int8)

	for s in range(samples):
		puzzle, solution = create_puzzle_solution_pair(source[int(npr()*len(source))],npr())
		puzzles[s] = to_sparse(puzzle)
		solutions[s] = to_sparse(solution)
	
	return puzzles, solutions

def validate_predictions(puzzles, solutions, predicted):
	predictable = 0
	accurate_predictions = 0
	for p in range(len(puzzles)):
		for r in range(9):
			for c in range(9):
				if puzzles[p][r][c][np.argmax(puzzles[p][r][c])] == 0:
					if np.argmax(solutions[p][r][c]) == np.argmax(predicted[p][r][c]):
						accurate_predictions += 1
					predictable += 1
	return accurate_predictions / predictable

model = build_sudoku_model(64, (3,3), 5)
#model = build_sudoku_model(128, (3,3), 10)
#model = build_sudoku_model(256, (3,3), 20)

solved_puzzles = open('valid puzzles','r').read().split('\n')[:-1]

herstory = {'predictive validity':[]}
for e in range(1,1000):
	puzzles, solutions = create_dataset(solved_puzzles, 1000)
	history = model.fit(puzzles, solutions, epochs=1, verbose=1, validation_split=0.10)

	puzzles, solutions = create_dataset(solved_puzzles, 1000)
	herstory['predictive validity'] += [validate_predictions(puzzles, solutions, model.predict(puzzles))]

	for key,value in history.history.items():
		if key in herstory:
			herstory[key] += value
		else:
			herstory[key] = value
	
	for key,value in herstory.items():
		if key.find('val') != -1:
			print (key,' '.join([str(x) for x in value]))










