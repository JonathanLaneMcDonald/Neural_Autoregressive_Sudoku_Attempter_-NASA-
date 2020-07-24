
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
