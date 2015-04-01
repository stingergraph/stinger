import sys

filename = sys.argv[1]
node1 = int(sys.argv[2])
node2 = int(sys.argv[3])
header = int(sys.argv[4])

with open(filename) as f:
    flipfilename = filename.split('.')[0] + '_flip.' + filename.split('.')[1]
    ff = open(flipfilename, 'w')
    lines = f.readlines()
    for i in range(header, len(lines)):
        lines[i] = lines[i].strip()
        elems = lines[i].split()
        for j in range(len(elems)):
            elems[j] = str(node2-int(elems[j])+node1)
        flipline = ''
        for j in range(len(elems)-1):
            flipline += elems[j] + ' '
        flipline += elems[len(elems)-1] + '\n'
        lines[i] = flipline
    for i in range(len(lines)):
        ff.write(lines[i])
f.close()
ff.close()
