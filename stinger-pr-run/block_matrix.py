import numpy as np

def gen_bdm(num_blocks, block_size):
	'''Generate a Block Diagonal Matrix with num_blocks blocks, each of size block_size along the diagonal'''
	
	return np.kron(np.eye(num_blocks), 1-np.eye(block_size))

def roc(bdm, num_blocks, block_size):
	''' Ring of Cliques from a Block Diagonal Matrix'''
	
	for i in range(num_blocks):
		if i==0:
			c1 = (num_blocks-1)*block_size
		else:
			c1 = (i-1)*block_size
			
		if i==(num_blocks-1):
			c2 = 0
		else:
			c2 = (i+1)*block_size
			
		bdm[i*block_size][c1] = 1
		bdm[i*block_size][c2] = 1

def move_node(node, clique, block_size, roc, del_matrix, add_matrix, new_roc):
	''' Move node from its present-clique to clique '''
	
	del_matrix[node, :] = roc[node, :]
	del_matrix[:, node] = roc[:, node]
	
	new_roc[node, :] = 0
	new_roc[:, node] = 0
	add_matrix[node, :] = 0
	add_matrix[:, node] = 0
	clique_nodes = np.where(new_roc[clique*block_size]>0)[0]
	clique_nodes = clique_nodes[(clique_nodes%block_size!=0)]
	clique_nodes = np.append(clique_nodes, [clique*block_size])
	new_roc[node, clique_nodes] = 1
	new_roc[clique_nodes, node] = 1
	add_matrix[node, clique_nodes] = 1
	add_matrix[clique_nodes, node] = 1
	
	and_matrix = add_matrix*del_matrix
	del_matrix = np.where(and_matrix==1, 0, del_matrix)
	add_matrix = np.where(and_matrix==1, 0, add_matrix)
	

def gen_alist_file(matrix, fname):
	nodes = matrix.shape[0]
	edges = int(np.sum(matrix))
				
	f = open(fname, "w")
	f.write(str(nodes)+" "+str(edges)+"\n")
	for i in range(nodes):
		line = ""
		for j in range(nodes):
			if matrix[i][j] != 0:
				line += str(j+1) + " "
		
		line = line[:-1]
		line += "\n"
		f.write(line)
		
	f.close()
	

		
if __name__ == "__main__":
	
	num_blocks = 3
	block_size = 10
	am = np.zeros((num_blocks*block_size, num_blocks*block_size))
	dm = np.zeros((num_blocks*block_size, num_blocks*block_size))
	a = gen_bdm(num_blocks, block_size)
	roc(a, num_blocks, block_size)
	new_a = np.copy(a)
	gen_alist_file(a, "roc_"+str(num_blocks)+"_"+str(block_size)+".csv")
	print a
	for i in range(1, num_blocks):
		move_node(i*block_size-1, i, block_size, a, dm, am, new_a)
		
	move_node(block_size*num_blocks-1, 0, block_size, a, dm, am, new_a)
	gen_alist_file(am, "roc_"+str(num_blocks)+"_"+str(block_size)+"_add.csv")
	gen_alist_file(dm, "roc_"+str(num_blocks)+"_"+str(block_size)+"_rm.csv")
	print a
	print
	print am
	print
	print dm
	"""
	num_blocks = 3
	block_size = 3
	am = np.zeros((num_blocks*block_size, num_blocks*block_size))
	dm = np.zeros((num_blocks*block_size, num_blocks*block_size))
	a = gen_bdm(num_blocks, block_size)
	roc(a, num_blocks, block_size)
	new_a = np.copy(a)
	print a
	print am
	print dm
	move_node(2, 0, 3, a, dm, am, new_a)
	print a - new_a
	"""

