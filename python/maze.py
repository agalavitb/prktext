import random

class Maze:
	width = 0
	height = 0
	def __init__(self, width, height):
		pass
	def north(self, x, y):
		pass
	def east(self, x, y):
		pass

class BackTracker(Maze):
	def __init__(self, width, height):
		def loop(grid, x, y):
			grid[x, y] = ''
			nbr = [(x, y-1), (x+1, y), (x, y+1), (x-1, y)]
			for i in random.sample(range(len(nbr)), len(nbr)):
				nx, ny = nbr[i]
				if nx<0 or nx>=width or ny<0 or ny>=height:
					continue
				if (nx,ny) in grid:
					continue
				grid[x, y] += 'nesw'[i]
				loop(grid, nx, ny)
				grid[nx, ny] += 'swne'[i]
		self.grid = {}
		self.width = width
		self.height = height
		loop(self.grid,random.randrange(width),random.randrange(height))

	def north(self, x, y):
		return 'n' in self.grid[x, y]

	def east(self, x, y):
		return 'e' in self.grid[x, y]


def format(maze, wall='#', floor=' ', ws=1, fs=1):
	def pass1(row):
		out = [wall]*ws
		for x in range(maze.width):
			out += [floor if maze.north(x,row) else wall] * fs
			out += [wall] * ws
		return out

	def pass2(row):
		out = [wall] * ws
		for x in range(maze.width):
			out += [floor] * fs
			out += [floor if maze.east(x,row) else wall] * ws
		return out

	out = []
	for row in range(maze.height):
		for _ in range(ws):
			out.append(pass1(row))
		for _ in range(fs):
			out.append(pass2(row))
	for _ in range(ws):
		out.append([wall]*((ws+fs)*maze.width + ws))

	width, height = len(out[0]), len(out)
	def iterator():
		for y in range(height):
			for x in range(width):
				yield x, y, out[y][x]
	return iterator(), width, height

if __name__ == '__main__':
	grid, w, h = format(BackTracker(24, 12), fs=2)
	line = ''
	for x, y, value in grid:
		line += value
		if x+1 == w:
			print(line)
			line = ''
