puzzles = [x.split()[0] for x in open('valid puzzles','r').read().split('\n')[:-1]]

count = dict()
write = dict()
for hints in set([len([y for y in x if y != '0']) for x in puzzles]):
	count[hints] = 0
	write[hints] = open('puzzles by hint count - '+str(hints),'w')

for p in puzzles:
	count[len([x for x in p if x != '0'])] += 1
	write[len([x for x in p if x != '0'])].write(p+'\n')

print ('\n'.join([str(x) for x in count.items()]))

