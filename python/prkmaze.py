from prk import Prk
import maze

def prkmaze(width, height, space=2, name='Maze'):
	park = Prk()
	park.name = name
	grid, w, h = maze.format(
		maze = maze.BackTracker(width, height),
		wall = 15,
		floor = 0,
		ws = 1,
		fs = space
	)
	park.setcoords(w, h)
	for x, y, v in grid:
		park.ground(park.x+x, park.y+y, v)
	return park

if __name__ == '__main__':
	park = prkmaze(5, 9)
	open(park.name + '.PRK', 'wb').write(park.tobytes())
