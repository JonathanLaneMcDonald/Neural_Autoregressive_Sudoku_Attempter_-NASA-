
from sys import argv

from groups import rows, cols, blks, membership

import numpy as np
from numpy.random import random as npr

from keras.models import load_model

from tkinter import *

SQ = 80


def ftoh(f):
	h = '0123456789abcdef'
	num = int(f*255)
	a = num // 16
	b = num % 16
	return h[a] + h[b]


class SudokuUI(Frame):

	def draw(self):
		""" Draw the pencil marks and use a model for annotation if the user wishes """

		self.canvas.delete('all')
		
		if self.update_annotation:
			if self.annotate_with_model:
				self.annotation = self.model.predict(np.array([self.board]))[0]
			else:
				self.annotation = self.generate_pencil_marks()
			self.update_annotation = False
		
		# draw cursor position
		self.canvas.create_rectangle(self.col*SQ, self.row*SQ, (self.col+1)*SQ, (self.row+1)*SQ, fill='#ffff88')
		
		# draw lines
		for line in range(9):
			if line % 3 == 0:
				self.canvas.create_line(line*SQ, 0, line*SQ, 9*SQ, fill='black', width=3)
				self.canvas.create_line(0, line*SQ, 9*SQ, line*SQ, fill='black', width=3)
			else:
				self.canvas.create_line(line*SQ, 0, line*SQ, 9*SQ, fill='black', dash=(5, 5))
				self.canvas.create_line(0, line*SQ, 9*SQ, line*SQ, fill='black', dash=(5, 5))

		txt = {
			0: (2*SQ // 10, 2*SQ // 10),
			1: (5*SQ // 10, 2*SQ // 10),
			2: (8*SQ // 10, 2*SQ // 10),
			3: (2*SQ // 10, 5*SQ // 10),
			4: (5*SQ // 10, 5*SQ // 10),
			5: (8*SQ // 10, 5*SQ // 10),
			6: (2*SQ // 10, 8*SQ // 10),
			7: (5*SQ // 10, 8*SQ // 10),
			8: (8*SQ // 10, 8*SQ // 10)	}
		
		for r in range(9):
			for c in range(9):
				if self.show_solution:
					self.canvas.create_text(
						c*SQ+SQ // 2, r*SQ+SQ // 2, font=self.assign_font,
						text=str(np.argmax(self.solution[r][c])+1))
				else:
					if max(self.board[r][c]) == 1:
						self.canvas.create_text(
							c*SQ+SQ // 2, r*SQ+SQ // 2, font=self.assign_font,
							text=str(np.argmax(self.board[r][c])+1))
					else:
						prefs = self.annotation[r][c]
						for p in range(9):
							value = 1-prefs[p]
							if prefs[p] > 0.98:
								self.canvas.create_text(
									c*SQ + txt[p][0], r*SQ + txt[p][1], font=self.pencil_font,
									text=str(p+1), fill='#'+ftoh(value)+ftoh(value)+'ff')
							else:
								self.canvas.create_text(
									c*SQ + txt[p][0], r*SQ + txt[p][1], font=self.pencil_font,
									text=str(p+1), fill='#ff'+ftoh(value)+ftoh(value))
						
		self.canvas.update_idletasks()
	
	def generate_pencil_marks(self):
		""" Given a puzzle, generate pencil marks by doing basic elimination. """

		# generate pencil marks for rows, cols, and blocks, then "and" them together for each position
		row_marks = [set(range(9)) - set([np.argmax(self.board[x // 9][x % 9]) for x in r if np.max(self.board[x // 9][x % 9]) == 1]) for r in rows]
		col_marks = [set(range(9)) - set([np.argmax(self.board[x // 9][x % 9]) for x in c if np.max(self.board[x // 9][x % 9]) == 1]) for c in cols]
		blk_marks = [set(range(9)) - set([np.argmax(self.board[x // 9][x % 9]) for x in b if np.max(self.board[x // 9][x % 9]) == 1]) for b in blks]

		pencil_marks = np.zeros((9, 9, 9))
		for m in range(81):
			r = m // 9
			c = m % 9
			for i in row_marks[membership[m][0]].intersection(col_marks[membership[m][1]]).intersection(blk_marks[membership[m][2]]):
				pencil_marks[r][c][i] = 1
		
		return pencil_marks + self.board

	def hard_reset(self):
		""" Reset everything to zeros """

		self.board = np.zeros((9, 9, 9))
		self.solution = np.zeros((9, 9, 9))
		self.update_annotation = True

	def reset_puzzle(self):
		""" Repopulate everything with a random puzzle """

		selection = int(npr()*len(self.puzzle_collection))
		puzzle = [int(x) for x in self.puzzle_collection[selection].split('\t')[0]]
		solution = [int(x) for x in self.puzzle_collection[selection].split('\t')[1]]
		
		self.board = np.zeros((9, 9, 9))
		for p in range(81):
			r = p // 9
			c = p % 9
			if puzzle[p]:
				self.board[r][c][puzzle[p]-1] = 1

		self.solution = np.zeros((9, 9, 9))
		for p in range(81):
			r = p // 9
			c = p % 9
			if solution[p]:
				self.solution[r][c][solution[p]-1] = 1

		self.update_annotation = True

	def keyboard(self, event):
		""" Handle keyboard input. Doubles as a user manual ;) """

		if event.keysym == 'Up':
			self.row -= 1
		if event.keysym == 'Down':
			self.row += 1
		if event.keysym == 'Left':
			self.col -= 1
		if event.keysym == 'Right':
			self.col += 1
		
		if event.keysym == 'r':
			self.reset_puzzle()
		if event.keysym == 'R':
			self.hard_reset()
		if event.keysym == 's':
			self.show_solution ^= 1

		if event.keysym == 'h':
			self.annotate_with_model ^= 1
			self.update_annotation = True

		if event.keysym == 'space':
			self.board[self.row][self.col] = np.zeros(9)
			self.board[self.row][self.col][np.argmax(self.annotation[self.row][self.col])] = 1
			self.update_annotation = True

		if event.keysym.isdigit():
			number = np.zeros(9)
			if int(event.keysym):
				number[int(event.keysym)-1] = 1
			self.board[self.row][self.col] = number
			self.update_annotation = True

		self.row = max(0, min(8, self.row))
		self.col = max(0, min(8, self.col))

		print(event.keysym, event.char)

		self.draw()
	
	def mouse_update(self, event):
		""" Have the cursor follow the mouse around """

		self.row = int(event.y // SQ)
		self.col = int(event.x // SQ)
		self.draw()

	def invent_canvas(self):
		""" Set up the canvas """

		Frame.__init__(self)
		self.master.title('Lane\'s Sudoku Helper')
		self.master.rowconfigure(0, weight=1)
		self.master.columnconfigure(0, weight=1)
		self.grid(sticky=N+S+E+W)

		self.canvas = Canvas(self, width=9*SQ, height=9*SQ)
		self.canvas.grid(row=0, column=0)

		self.bind_all('<KeyPress>', self.keyboard)
		self.bind_all('<Motion>', self.mouse_update)

		self.draw()

	def __init__(self):
		""" Define some variables and load a model and some puzzles """

		self.assign_font = ('Helvetica', 48)
		self.pencil_font = ('Helvetica', 12)

		self.board = np.zeros((9, 9, 9))

		if len(argv) == 2:
			try:
				self.model = load_model(argv[1])
			except IOError:
				print('error loading model')
				self.model = None
		else:
			self.model = None

		if self.model:
			self.annotate_with_model = True
		else:
			self.annotate_with_model = False

		self.update_annotation = True
		self.annotation = np.zeros((9, 9, 9))
		self.solution = np.zeros((9, 9, 9))

		self.puzzle_collection = open('valid puzzles', 'r').read().split('\n')[:-1]
		self.show_solution = 0

		self.row = 4
		self.col = 4
		
		self.reset_puzzle()
		self.invent_canvas()


SudokuUI().mainloop()
