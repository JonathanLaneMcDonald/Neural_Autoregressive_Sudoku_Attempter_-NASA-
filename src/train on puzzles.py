
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
from keras.initializers import RandomNormal
from keras.optimizers import Adam

def build_sudoku_model(filters, kernels, layers):
	
	input = Input(shape=(9,9,9))

	x = Conv2D(filters=filters, kernel_size=kernels, padding='same', kernel_initializer=RandomNormal(mean=0, stddev=0.01))(input)
	x = BatchNormalization()(x)
	x = Activation('relu')(x)
	
	conv = 2
	blocks = layers//conv
	for _1 in range(blocks):
		y = x

		for _2 in range(conv):
			y = Conv2D(filters=filters, kernel_size=kernels, padding='same', kernel_initializer=RandomNormal(mean=0, stddev=0.01))(y)
			y = BatchNormalization()(y)
			y = Activation('relu')(y)

		x = Add()([x,y])

	output = Conv2D(filters=9, kernel_size=(1,1), padding='same', activation='softmax', kernel_initializer=RandomNormal(mean=0, stddev=0.01))(x)

	model = Model(inputs=input, outputs=output)
	model.compile(loss='categorical_crossentropy', optimizer='adam', metrics=['accuracy'])
	model.summary()
	return model

def build_3D_sudoku_model(filters, kernels, layers):
	
	input = Input(shape=(9,9,9))

	x = Reshape((9,9,9,1))(input)
	x = Conv3D(filters=filters, kernel_size=kernels, padding='same', kernel_initializer=RandomNormal(mean=0, stddev=0.01))(x)
	x = BatchNormalization()(x)
	x = Activation('relu')(x)
	
	conv = 2
	blocks = layers//conv
	for _1 in range(blocks):
		y = x

		for _2 in range(conv):
			y = Conv3D(filters=filters, kernel_size=kernels, padding='same', kernel_initializer=RandomNormal(mean=0, stddev=0.01))(y)
			y = BatchNormalization()(y)
			y = Activation('relu')(y)

		x = Add()([x,y])

	x = Conv3D(filters=1, kernel_size=(1,1,1), padding='same', activation='relu', kernel_initializer=RandomNormal(mean=0, stddev=0.01))(x)
	x = Reshape((9,9,9))(x)
	output = Conv2D(filters=9, kernel_size=(1,1), padding='same', activation='softmax', kernel_initializer=RandomNormal(mean=0, stddev=0.01))(x)

	model = Model(inputs=input, outputs=output)
	model.compile(loss='categorical_crossentropy', optimizer=Adam(), metrics=['accuracy'])
	model.summary()
	return model

import numpy as np
from numpy.random import permutation
from numpy.random import random as npr

def get_mae_for_solution_prediction_pair(puzzle, solution, prediction, per_unknown = True):
	if per_unknown:
		unknowns = 0
		for r in range(9):
			for c in range(9):
				if np.sum(puzzle[r][c]) == 0:
					unknowns += 1
		if unknowns:
			return np.sum(np.abs(solution - prediction)) / float(unknowns)
		else:
			return 0
	else:
		return np.sum(np.abs(solution - prediction))

def get_hardest_n_puzzles_by_mae(puzzles, solutions, predictions, n):
	if n:
		scores = list(reversed(sorted([(get_mae_for_solution_prediction_pair(puzzles[x], solutions[x], predictions[x]), x) for x in range(len(puzzles))])))
		return permutation([(puzzles[x[1]], solutions[x[1]]) for x in scores[:n]])
	else:
		return []

def create_random_homomorphism_in_place(puzzle, solution):
	sbox = {k:v for k,v in zip(range(9),permutation(range(9)))}

	new_puzzle = np.zeros((9,9,9))
	new_solution = np.zeros((9,9,9))
	for r in range(9):
		for c in range(9):
			if np.sum(puzzle[r][c]):
				puz_max = np.argmax(puzzle[r][c])
				new_puzzle[r][c][sbox[puz_max]] = 1
			
			if np.sum(solution[r][c]):
				sol_max = np.argmax(solution[r][c])
				new_solution[r][c][sbox[sol_max]] = 1
			
	return new_puzzle, new_solution

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

def create_dataset(source, samples, for_validation=False, puzzles_for_review = []):
	puzzles = np.zeros((samples, 9, 9, 9),dtype=np.int8)
	solutions = np.zeros((samples, 9, 9, 9),dtype=np.int8)
	pencilmarks = np.zeros((samples, 9, 9, 9),dtype=np.int8)
	
	s = 0
	for puzzle, solution in puzzles_for_review:
		puzzles[s], solutions[s] = create_random_homomorphism_in_place(puzzle, solution)
		s += 1

	print ('resampled data records:',s)
	while s < samples:
		puzzle, solution = create_puzzle_solution_pair(source[int(npr()*len(source))],int(for_validation)+max(0,npr()))
		puzzles[s] = to_sparse(puzzle)
		solutions[s] = to_sparse(solution)
		#pencilmarks[s] = generate_pencil_marks(puzzles[s])
		s += 1

	return puzzles, solutions, pencilmarks

def validate_predictions(puzzles, solutions, pencilmarks, model, recursions=1):
	predicted = model.predict(puzzles)

	for i in range(1,recursions):
		print ('recursing in validate_predictions:',i)
		predicted = model.predict(predicted)

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

def autoregressive_validation(puzzles, solutions, pencilmarks, model, recursions=1):
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
			predicted = model.predict(np.array([working_puzzle]))[0]
		
			for i in range(1, recursions):
				predicted = model.predict(np.array([predicted]))[0]

			(r,c), value = get_max_prediction(working_puzzle, predicted)

			if np.argmax(solutions[p][r][c]) == value:
						accurate_predictions += 1
					predictable += 1
					
	return accurate_predictions / predictable

from keras.models import load_model

'''
the sudoku "paper"

	why is sudoku an interesting problem from the point of view of machine learning?

		used to fuzzy rules, but sudoku works with discrete values and logical relationships + it's my first crack at 3d convolutions

		results
			show a graph of the 5(?) models in training
				show the loss graph
				show the one-shot solution graph
				show the autoregressive solution graph
			
			show a gif of the best model making pencilmarks and me just doing what it says, so you can watch the colors change in intensity :D
	
	concepts

		sudoku
			is this type of game with simple rules. feel free to gloss over and then offer a link to wikipedia/sudoku

		data representation

			sudoku is a 3d game, masquerading as a 2d game. its most natural representation is as a binary puzzle in 3d and to carry those through with conv3d
		
		data augmentation
			
			sudoku has properties that can be exploited for the purpose of data augmentation
				
				board symmetries, which i exploited in my reimplementation of the alphago paper, but don't care about here
				
				sudoku is strongly homomorphic - explain what that means and maybe give some examples?

			random subsets

				puzzles and solutions have been generated - this leaves us typically with a 50-60 step process to complete a puzzle

				to augment data, we can select random subsets to set as knowns and predict the rest of the puzzle from that

		autoregressive inference

			autoregressive inference is used regularly in natural language processing and it is used here to great effect!

			in this application, it just means trying to predict everything, like the model was trained to do, but then only making the single move with the highest confidence. this process is repeated until the puzzle is complete.
			
			the model is trained to predict solutions in one shot from a partially completed puzzle

		recursion

			we all know about recursion in programming, but there's also a thing called a recursive neural net. i'm not using recursive layers here, but i did attempt to apply the network recursively, which was possible because the outputs are the same shape as the inputs, and the results were pretty good! - so just point out that this is vocabulary at this point and i don't actually have experience with recursion in networks, but i've heard the vocabulary and it got me thinking of trying something.

	methods and results

		generation of puzzles and solutions

		lots of opportunity for data augmentation (symmetries and homomorphisms)

		deciding how to represent data as input/output, but also as it moves through the network

			it only really makes sense to represent as binary at i/o, but:

				we can use conv2d within the network, or

				we can use conv3d within the network

		during training, can we speed up training by replaying homomorphisms of difficult puzzles

		post-training analysis

			turns out i wrote this really old sudoku solver, which is also available

			then i used that to analyze the results and see what it got right on a per-technique basis?

		exploration

			so we've got what we've got... how can we make it better?

			specifically:
				we've trained a series of networks and we can see that conv3d > conv2d, deeper > shallow, replay > no replay, and we've used fine-tuning. this has taken us from ~50% to ~80% accuracy predicting the solution at once and from ~55% to ~95% accuracy predicting the solution autoregressively. so let's poke around and see what else we can find...

				lazy recursion
					recursive layers are a thing. sudoku takes binary inputs and produces a bunch of probability distributions, but they're the same shape as the input. this means if we ask the network for a set of predictions, we can just feed those predictions through the network again as many times as we want. the question is whether or not that helps with anything.

					whole solution prediction is faster than autoregressive prediction, so we can start with that and it turns out to be pretty effective! two passes take us from 81 to 85% prediction accuracy! but that benefit seems to attenuate after just two applications, but we see there's some promise here, so we set up to measure autoregressive performance, too.

					the final step (for me) will be to just train a deeper network and apply it recursively, so i'll throw away the weight limit and train a 3d 32x100 model with replay and fine-tuning and apply it recursively and see what happens!
			
			things that could take this further

				the main thing i can think of is attention. attention mechanisms have been used very successfully in nlp. it might be kind of complicated to extend to our 3d space, but it'd probably take this project to the next level!
		
		what has our model actually learned?

			I was really into sudoku back in like 2005 with a friend of mine and i wrote code to apply a number of logical techniques. this tool can be used to help us figure out what techniques our model has learned!

			this is all basically going to be an on-going process of rewriting that project to make it easier to get puzzles in and out and to turn on options as we like and see how accurately the model finds these options

			this is going to need to be a work in progress... i don't mind putting spare cycles into this over time.

			aside: i think between this and the japanese project and my phd, it's going to be apparent that i can do a deep dive into just about anything...

do a model comparison where the models pretty much all have around 1.5 million weights

	baseline model conv2d
	make it better with conv3d
	make it better by making it deeper
	make it better with an improved training regime (replay difficult puzzles)
	make it better by making it even deeper(?)
	: the moral of the story is look how much better performance can be with good architecture, a good representation, and a good training regime
	: more lessons in the form of objectively measuring performance instead of solely relying on loss and accuracy as reported by tf

	baseline is one of the intermediate models used by leela go zero
		3,3 x 128 x 10

	jump straight to keeping a high fidelity internal repr with conv3d
		3,3,3 x 32 x 50 (or something, to keep the weights roughly equal)

	try making it deeper
		3,3,3 x 16 x 200
	
	try training with replay
		should accelerate learning
	
	fine-tune the model after training

	make each of these models available in the repo
	allow the user to load a specified model to play with in the play sudoku.py script
'''

model = build_3D_sudoku_model(32, (3,3,3), 50)
#model = build_sudoku_model(64, (3,3), 50)

#model = load_model('model - one-shot_acc=0.9002 conv3d 32x100 + replay + ft')
#model.compile(loss='categorical_crossentropy', optimizer=Adam(lr=0.0002), metrics=['accuracy'])
#model.summary()

solved_puzzles = open('valid puzzles','r').read().split('\n')[:-1]

batches = 10000
batch_size = 128
samples = batch_size * batches
resamples = samples//5
best_pred_acc = 0
puzzles_for_review = []
herstory = {'predictive_acc':[],'autoregressive_acc':[]}
for e in range(1,1000):
	puzzles, solutions, pencilmarks = create_dataset(solved_puzzles, samples, puzzles_for_review=puzzles_for_review)
	history = model.fit(puzzles, solutions, batch_size=batch_size, epochs=1, verbose=1)

	predictions = model.predict(puzzles)
	puzzles_for_review = get_hardest_n_puzzles_by_mae(puzzles, solutions, predictions, resamples)

	puzzles, solutions, pencilmarks = create_dataset(solved_puzzles, 5000, True, [])
	herstory['predictive_acc'] += [validate_predictions(puzzles, solutions, pencilmarks, model)]

	puzzles, solutions, pencilmarks = create_dataset(solved_puzzles, 500, True, [])
	herstory['autoregressive_acc'] += [autoregressive_validation(puzzles, solutions, pencilmarks, model)]

	for key,value in history.history.items():
		if key in herstory:
			herstory[key] += value
		else:
			herstory[key] = value

	for key,value in herstory.items():
		print (key,' '.join([str(x)[:6] for x in value]))

	if best_pred_acc < herstory['predictive_acc'][-1]:
		best_pred_acc = herstory['predictive_acc'][-1]
		model.save('model - one-shot_acc='+str(best_pred_acc)[:6])
		print ('model saved')







