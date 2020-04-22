
import numpy as np
from numpy.random import random as npr

from keras.models import load_model

from tkinter import *

h = '0123456789abcdef'
def ftoh(f):
	num = int(f*255)
	a = num//16
	b = num%16
	return h[a] + h[b]

class display( Frame ):

	def draw(self):
		self.canvas.delete('all')
		
		if self.update_prediction:
			self.prediction = self.model.predict(np.array([self.board]))[0]
		
		# draw cursor position
		self.canvas.create_rectangle(self.col*100,self.row*100,(self.col+1)*100,(self.row+1)*100,fill='#ffff88')
		
		# draw lines
		for line in range(9):
			if line % 3 == 0:
				self.canvas.create_line(line*100,0,line*100,900,fill='black',width=3)
				self.canvas.create_line(0,line*100,900,line*100,fill='black',width=3)
			else:
				self.canvas.create_line(line*100,0,line*100,900,fill='black', dash=(5,5))
				self.canvas.create_line(0,line*100,900,line*100,fill='black', dash=(5,5))

		loc = {0:(0,0,30,30),1:(35,0,65,30),2:(70,0,100,30),3:(0,35,30,65),4:(35,35,65,65),5:(70,35,100,65),6:(0,70,30,100),7:(35,70,65,100),8:(70,70,100,100)}
		txt = {0:(15,15),1:(50,15),2:(85,15),3:(15,50),4:(50,50),5:(85,50),6:(15,85),7:(50,85),8:(85,85)}
		
		for r in range(9):
			for c in range(9):
				if self.show_solution:
					self.canvas.create_text(c*100+50,r*100+50,font=self.assign_font,text=str(np.argmax(self.solution[r][c])+1))
				else:
					if max(self.board[r][c]) == 1:
						self.canvas.create_text(c*100+50,r*100+50,font=self.assign_font,text=str(np.argmax(self.board[r][c])+1))
					else:
						prefs = self.prediction[r][c]
						for p in range(9):
							value = 1-prefs[p]#**2
							self.canvas.create_text(c*100 + txt[p][0], r*100 + txt[p][1],font=self.pencil_font,text=str(p+1), fill='#ff'+ftoh(value)+ftoh(value))
							#self.canvas.create_oval(c*100 + loc[p][0], r*100 + loc[p][1], c*100 + loc[p][2],  r*100 + loc[p][3], fill='#ff'+ftoh(value)+ftoh(value))
						
		self.canvas.update_idletasks()
	
	def hard_reset(self):
		self.board = np.zeros((9,9,9))
		self.solution = np.zeros((9,9,9))

	def reset_puzzle(self):
		selection = int(npr()*len(self.puzzle_collection))
		puzzle = [int(x) for x in self.puzzle_collection[selection].split('\t')[0]]
		solution = [int(x) for x in self.puzzle_collection[selection].split('\t')[1]]
		
		self.board = np.zeros((9,9,9))
		for p in range(81):
			r, c = p//9, p%9
			if puzzle[p]:
				self.board[r][c][puzzle[p]-1] = 1

		self.solution = np.zeros((9,9,9))
		for p in range(81):
			r, c = p//9, p%9
			if solution[p]:
				self.solution[r][c][solution[p]-1] = 1

	def keyboard(self, event):

		if event.keysym == 'Up':			self.row -= 1
		if event.keysym == 'Down':			self.row += 1
		if event.keysym == 'Left':			self.col -= 1
		if event.keysym == 'Right':			self.col += 1
		
		if event.keysym == 'r':				self.reset_puzzle()
		if event.keysym == 'R':				self.hard_reset()
		if event.keysym == 's':				self.show_solution ^= 1

		if event.keysym == 'space':
			self.board[self.row][self.col] = np.zeros(9)
			self.board[self.row][self.col][np.argmax(self.prediction[self.row][self.col])] = 1

		if event.keysym.isdigit():
			number = np.zeros(9)
			if int(event.keysym):
				number[int(event.keysym)-1] = 1
			self.board[self.row][self.col] = number

		self.row = max(0, min(8, self.row))
		self.col = max(0, min(8, self.col))

		print (event.keysym, event.char)

		self.draw()
	
	def invent_canvas(self):
		Frame.__init__(self)
		self.master.title('Lane\'s Sudoku Helper')
		self.master.rowconfigure(0,weight=1)
		self.master.columnconfigure(0,weight=1)
		self.grid(sticky=N+S+E+W)

		self.canvas=Canvas(self,width=900, height=900)
		self.canvas.grid(row=0,column=0)

		self.bind_all('<KeyPress>', self.keyboard)

		self.draw()

	def __init__(self):
		
		self.assign_font = ('TakaoMincho', 64)
		self.pencil_font = ('TakaoMincho', 16)

		self.board = np.zeros((9,9,9))
		self.model = load_model('model - ar=0.9057')
		
		self.update_prediction = True
		self.prediction = np.zeros((9,9,9))
		self.solution = np.zeros((9,9,9))

		self.puzzle_collection = open('valid puzzles','r').read().split('\n')[:-1]
		self.show_solution = 0

		self.row = 4
		self.col = 4
		
		self.reset_puzzle()
		self.invent_canvas()

display().mainloop()
