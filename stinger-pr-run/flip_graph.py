import sys

filename = sys.argv[1]

with open(filename) as f:
    flipfilename = filename.split('.')[0] + '_flip.' + filename.split('.')[1]
    ff = open(flipfilename, 'w')
    lines = f.readlines()
    ff.write(lines[0])
    lines[0] = lines[0].strip()
    elems = lines[0].split()
    nodes = int(elems[0])
    edges = int(elems[1])
    edgecount = 0
    for i in range(1, len(lines)):
        lines[i] = lines[i].strip()
        elems = lines[i].split()
        edgecount += len(elems)
        for j in range(len(elems)):
            elems[j] = str(nodes+1-int(elems[j]))
        flipline = ''
        for j in range(len(elems)-1):
            flipline += elems[j] + ' '
        flipline += elems[len(elems)-1] + '\n'
        lines[i] = flipline
    for i in range(1, len(lines)):
        ff.write(lines[nodes+1-i])
f.close()
ff.close()
