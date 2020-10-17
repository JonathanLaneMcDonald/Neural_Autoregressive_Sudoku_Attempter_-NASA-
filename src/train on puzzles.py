
"""
This is where the models are trained and fine-tuned
"""

import numpy as np
from numpy.random import permutation
from numpy.random import random as npr

from keras.models import Model, load_model, save_model
from keras.layers import Input, Conv2D, Conv3D
from keras.layers import BatchNormalization, Add, Activation, Reshape
from keras.initializers import RandomNormal
from keras.optimizers import Adam


def build_2d_sudoku_model(filters, kernels, layers):
	""" This is where the 2d model is built. 2d models kind of suck at sudoku,
		but I figure it's useful to leave in for tutorial purposes. Note that 
		the input and output shapes are the same for the 2d and 3d models, but
		that the Reshape() layer is not needed for the 2d model. """

	inputs = Input(shape=(9, 9, 9))

	x = Conv2D(
		filters=filters, kernel_size=kernels, padding='same',
		kernel_initializer=RandomNormal(mean=0, stddev=0.01))(inputs)
	x = BatchNormalization()(x)
	x = Activation('relu')(x)
	
	conv = 2
	blocks = layers // conv
	for _1 in range(blocks):
		y = x

		for _2 in range(conv):
			y = Conv2D(
				filters=filters, kernel_size=kernels, padding='same',
				kernel_initializer=RandomNormal(mean=0, stddev=0.01))(y)
			y = BatchNormalization()(y)
			y = Activation('relu')(y)

		x = Add()([x, y])

	outputs = Conv2D(
		filters=9, kernel_size=(1, 1), padding='same', activation='softmax',
		kernel_initializer=RandomNormal(mean=0, stddev=0.01))(x)

	model = Model(inputs=inputs, outputs=outputs)
	model.compile(loss='categorical_crossentropy', optimizer='adam', metrics=['accuracy'])
	model.summary()
	return model


def build_3d_sudoku_model(filters, kernels, layers):
	""" This is where the 3d model is built. 3d models are way better at sudoku
		than 2d models. Note the use of the Reshape() layer in the 3d model. """

	inputs = Input(shape=(9, 9, 9))

	x = Reshape((9, 9, 9, 1))(inputs)
	x = Conv3D(
		filters=filters, kernel_size=kernels, padding='same',
		kernel_initializer=RandomNormal(mean=0, stddev=0.01))(x)
	x = BatchNormalization()(x)
	x = Activation('relu')(x)
	
	conv = 2
	blocks = layers // conv
	for _1 in range(blocks):
		y = x

		for _2 in range(conv):
			y = Conv3D(
				filters=filters, kernel_size=kernels, padding='same',
				kernel_initializer=RandomNormal(mean=0, stddev=0.01))(y)
			y = BatchNormalization()(y)
			y = Activation('relu')(y)

		x = Add()([x, y])

	x = Conv3D(
		filters=1, kernel_size=(1, 1, 1), padding='same', activation='relu',
		kernel_initializer=RandomNormal(mean=0, stddev=0.01))(x)
	x = Reshape((9, 9, 9))(x)
	outputs = Conv2D(
		filters=9, kernel_size=(1, 1), padding='same', activation='softmax',
		kernel_initializer=RandomNormal(mean=0, stddev=0.01))(x)

	model = Model(inputs=inputs, outputs=outputs)
	model.compile(loss='categorical_crossentropy', optimizer=Adam(), metrics=['accuracy'])
	model.summary()
	return model


def get_mae_for_solution_prediction_pair(puzzle, solution, prediction, per_unknown=True):
	""" Calculate the loss between solutions and predictions."""

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
	""" Calculate the loss between solutions and predictions for a set of puzzles.
		I'm using this as part of my training schedule where I keep some of the 
		hardest puzzles so I can focus on them. In practice, this seems to speed 
		up training. """

	if n:
		scores = list(reversed(sorted([(get_mae_for_solution_prediction_pair(puzzles[x], solutions[x], predictions[x]), x) for x in range(len(puzzles))])))
		return permutation([(puzzles[x[1]], solutions[x[1]]) for x in scores[:n]])
	else:
		return []


def create_random_homomorphism_in_place(puzzle, solution):
	""" Given a cubic grid, create a random homomorphism in-place. """

	sbox = {k: v for k, v in zip(range(9), permutation(range(9)))}

	new_puzzle = np.zeros((9, 9, 9))
	new_solution = np.zeros((9, 9, 9))
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
	""" Given a puzzle/solution pair as a string, create a random homomorphism. """

	sbox = {'0': '0', '\t': '\t'}
	sbox.update({str(k): str(v) for k, v in zip(range(1, 10), permutation(range(1, 10)))})
	return ''.join([sbox[x] for x in line])


def create_puzzle_solution_pair(line, predictability):
	""" Given a puzzle/solution pair as a string, produce a random homomorphism
		and then place the puzzle in a random state of completion. """

	homo = create_random_homomorphism(line)
	puzzle, solution = list(homo.split()[0]), list(homo.split()[1])
	mutable = [p != s for p, s in zip(puzzle, solution)]
	for i in range(len(mutable)):
		if mutable[i]:
			if npr() < predictability:
				# it only appears in the solution and should only appear in the solution
				pass
			else:
				# it only appears in the solution, but should only appear in the puzzle
				puzzle[i] = solution[i]

	return [int(x) for x in puzzle], [int(x) for x in solution]


def to_sparse(data):
	""" Convert the supplied puzzle from a string to a cubic shape. """

	frame = np.zeros((9, 9, 9), dtype=np.int8)
	for i in range(81):
		if data[i]:
			r = i // 9
			c = i % 9
			frame[r][c][data[i]-1] = 1
	return frame


def create_dataset(source, samples, for_validation=False, puzzles_for_review=[]):
	""" Create the training dataset. """

	puzzles = np.zeros((samples, 9, 9, 9), dtype=np.int8)
	solutions = np.zeros((samples, 9, 9, 9), dtype=np.int8)

	s = 0
	for puzzle, solution in puzzles_for_review:
		puzzles[s], solutions[s] = create_random_homomorphism_in_place(puzzle, solution)
		s += 1

	print('resampled data records:', s)
	while s < samples:
		puzzle, solution = create_puzzle_solution_pair(source[int(npr()*len(source))], int(for_validation)+max(0, npr()))
		puzzles[s] = to_sparse(puzzle)
		solutions[s] = to_sparse(solution)
		s += 1

	return puzzles, solutions


def validate_predictions(puzzles, solutions, model, recursions=1):
	""" Given a set of puzzles and a model, evaluate model accuracy by
		generating one-shot solutions. """

	predicted = model.predict(puzzles)

	for i in range(1, recursions):
		print('recursing in validate_predictions:', i)
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


def autoregressive_validation(puzzles, solutions, model, recursions=1):
	""" Given a set of puzzles and a model, evaluate model accuracy by
		generating solutions autoregressively. """

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
					predictions.append((max(prediction[r][c]), (r, c), np.argmax(prediction[r][c])))
		predictions = list(reversed(sorted(predictions)))
		return predictions[0][1], predictions[0][2]

	solved = 0
	total = 0
	predictable = 0
	accurate_predictions = 0
	for p in range(len(puzzles)):
		working_puzzle = np.copy(puzzles[p])
		
		flawless_victory = True
		while number_of_unpredicted(working_puzzle):
			predicted = model.predict(np.array([working_puzzle]))[0]
		
			for i in range(1, recursions):
				predicted = model.predict(np.array([predicted]))[0]

			(r, c), value = get_max_prediction(working_puzzle, predicted)

			working_puzzle[r][c][value] = 1
			if np.argmax(solutions[p][r][c]) == value:
				accurate_predictions += 1
			else:
				flawless_victory = False
			predictable += 1

		if flawless_victory:
			solved += 1
		total += 1

		print(p+1, solved/total, accurate_predictions/predictable)

	return accurate_predictions / predictable


""" Build a 2d model """
# model = build_2d_sudoku_model(64, (3, 3), 50)

""" Build a 3d model """
model = build_3d_sudoku_model(32, (3, 3, 3), 50)

""" Load an existing model for further training/fine-tuning/evaluation/etc """
# model = load_model('model - one-shot_acc=0.8232')
# model.compile(loss='categorical_crossentropy', optimizer=Adam(lr=0.0002), metrics=['accuracy'])
# model.summary()

""" Load some puzzles for training """
solved_puzzles = open('valid puzzles', 'r').read().split('\n')[:-1]

""" Training loop """
batches = 10000
batch_size = 128
samples = batch_size * batches
resamples = samples // 5
best_pred_acc = 0
puzzles_for_review = []
herstory = {'predictive_acc': [], 'autoregressive_acc': []}

for e in range(1, 1000):
	puzzles, solutions = create_dataset(solved_puzzles, samples, puzzles_for_review=puzzles_for_review)
	history = model.fit(puzzles, solutions, batch_size=batch_size, epochs=1, verbose=1)

	predictions = model.predict(puzzles)
	puzzles_for_review = get_hardest_n_puzzles_by_mae(puzzles, solutions, predictions, resamples)

	puzzles, solutions = create_dataset(solved_puzzles, 1000, True, [])
	herstory['predictive_acc'] += [validate_predictions(puzzles, solutions, model)]

	puzzles, solutions = create_dataset(solved_puzzles, 100, True, [])
	herstory['autoregressive_acc'] += [autoregressive_validation(puzzles, solutions, model)]

	for key, value in history.history.items():
		if key in herstory:
			herstory[key] += value
		else:
			herstory[key] = value

	for key, value in herstory.items():
		print(key, ' '.join([str(x)[:6] for x in value]))

	if best_pred_acc < herstory['predictive_acc'][-1]:
		best_pred_acc = herstory['predictive_acc'][-1]
		save_model(model, 'sudoku model - one-shot_acc='+str(best_pred_acc)[:6], save_format='h5')
		print('model saved')
