class Sudoku:
  def __init__(self, filepath):
    self.rows = []
    y = 0
    for line in open(filepath):
      i = list(map(int, list(line.rstrip().replace(".", "0"))))
      i = [j if j != 0 else None for j in i]
      i = list(enumerate(i))
      i = [Cell(j[1], j[0], y, self) for j in i]
      self.rows.append(i)
      y += 1

    self.refreshValues()

  def __str__(self):
    return """{} {} {}  {} {} {}  {} {} {}
{} {} {}  {} {} {}  {} {} {}
{} {} {}  {} {} {}  {} {} {}

{} {} {}  {} {} {}  {} {} {}
{} {} {}  {} {} {}  {} {} {}
{} {} {}  {} {} {}  {} {} {}

{} {} {}  {} {} {}  {} {} {}
{} {} {}  {} {} {}  {} {} {}
{} {} {}  {} {} {}  {} {} {}""".format(*self.values)

  def refreshValues(self):
    self.values = []
    for row in self.rows:
      self.values += list(map(str, row))
    return None

class Cell:
  def __init__(self, value, x, y, sudoku):
    self.value = value
    self.x = x
    self.y = y
    #x, y coordinates where top left is (0, 0) and bottom right is (8, 8)
    
    if   y in range(0, 2 +1):
      if   x in range(0, 2 +1): self.box = "A"
      elif x in range(3, 5 +1): self.box = "B"
      elif x in range(6, 8 +1): self.box = "C"
    elif y in range(3, 5 +1):
      if   x in range(0, 2 +1): self.box = "D"  # A B C
      elif x in range(3, 5 +1): self.box = "E"  # D E F
      elif x in range(6, 8 +1): self.box = "F"  # G H I
    elif y in range(6, 8 +1):
      if   x in range(0, 2 +1): self.box = "G"
      elif x in range(3, 5 +1): self.box = "H"
      elif x in range(6, 8 +1): self.box = "I"

    if value: self.p = [False]*9
    else: self.p = [True]*9
    self.sudoku = sudoku

  def __str__(self):
    return str(self.value)
  
  def refreshPoss(self):
    if self.p.count(True) != 1:
      for row in self.sudoku.rows:
        for cell in row:
          if (cell.x == self.x or cell.y == self.y or cell.box == self.box) and cell.value:
            self.p[cell.value -1] = False
      
    if self.p.count(True) == 1:
      i = self.p.index(True)
      self.value = i + 1
      self.p[i] = False
      return None
    
    return None

s = Sudoku("sudoku.txt")

while "None" in s.values: #Cell.__str__ cannot return a non-string, hence "None"
  for row in s.rows:
    for cell in row:
      cell.refreshPoss()
  s.refreshValues()

print(s)
