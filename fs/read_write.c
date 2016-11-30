#include "testfs.h"
#include "list.h"
#include "super.h"
#include "block.h"
#include "inode.h"

#define NR_MAX_BLOCK NR_DIRECT_BLOCKS+NR_INDIRECT_BLOCKS+NR_INDIRECT_BLOCKS*NR_INDIRECT_BLOCKS
#define addr_size (BLOCK_SIZE / 4)
/* given logical block number, read the corresponding physical block into block.
 * return physical block number.
 * returns 0 if physical block does not exist.
 * returns negative value on other errors. */
static int
testfs_read_block(struct inode *in, int log_block_nr, char *block)
{
	if(log_block_nr > NR_MAX_BLOCK){
		return EFBIG;
	}
	
	int phy_block_nr = 0;

	assert(log_block_nr >= 0);
	if (log_block_nr < NR_DIRECT_BLOCKS) {
		phy_block_nr = (int)in->in.i_block_nr[log_block_nr];
	} 
	
	else {
		log_block_nr -= NR_DIRECT_BLOCKS;
		
		if (log_block_nr >= NR_INDIRECT_BLOCKS){
			//TBD();
			log_block_nr = log_block_nr - NR_INDIRECT_BLOCKS;
			int page = log_block_nr / addr_size;
			int off_set = log_block_nr % addr_size;
			

		
			if (in->in.i_dindirect <= 0) {
				bzero(block,BLOCK_SIZE);
				return phy_block_nr;
				
			}
			else{
				read_blocks(in->sb, block, in->in.i_dindirect, 1);
				phy_block_nr = ((int *)block)[page];
			}
			
			if(phy_block_nr <= 0){
				bzero(block,BLOCK_SIZE);
				return phy_block_nr;
				
				
			}
			else{
				read_blocks(in->sb, block, phy_block_nr, 1);
				phy_block_nr = ((int*)block)[off_set];
			}
			
			if(phy_block_nr <= 0){
				bzero(block, BLOCK_SIZE);
			}
			else{
				read_blocks(in->sb, block, phy_block_nr, 1);
				
			}
			return phy_block_nr;
			
		}
		else if(in->in.i_indirect > 0){
			read_blocks(in->sb, block, in->in.i_indirect, 1);
			phy_block_nr = ((int*)block)[log_block_nr];
		}
	}
	if (phy_block_nr > 0) {
		read_blocks(in->sb, block, phy_block_nr, 1);
	} else {
		/* we support sparse files by zeroing out a block that is not
		 * allocated on disk. */
		bzero(block, BLOCK_SIZE);
	}
	return phy_block_nr;
}

int
testfs_read_data(struct inode *in, char *buf, off_t start, size_t size)
{
	char block[BLOCK_SIZE];
	long block_nr = start / BLOCK_SIZE;
	long block_ix = start % BLOCK_SIZE;
	int ret;
	
	int original_size = size;
	
	if(block_nr > NR_MAX_BLOCK){
		return EFBIG;
	}
	
	assert(buf);
	if (start + (off_t) size > in->in.i_size) {	//in.i_size is start of file
		size = in->in.i_size - start;
	}
	
	do{
		if (block_ix + size > BLOCK_SIZE) {
			//TBD();
			if(block_nr > NR_MAX_BLOCK){
				return EFBIG;
			}
			ret = testfs_read_block(in, block_nr, block);
			if(ret < 0){
				return ret;
			}
			size_t byte_left = BLOCK_SIZE - block_ix;
			
			memcpy(buf, block+ block_ix, byte_left);
			buf = byte_left + buf;
			size = size - byte_left;
			block_nr++;
			block_ix = 0;
			
		}
	}while(size > BLOCK_SIZE);
	
	if ((ret = testfs_read_block(in, block_nr, block)) < 0)
		return ret;
	memcpy(buf, block + block_ix, size);
	/* return the number of bytes read or any error */
	return original_size;
}

/* given logical block number, allocate a new physical block, if it does not
 * exist already, and return the physical block number that is allocated.
 * returns negative value on error. */
static int
testfs_allocate_block(struct inode *in, int log_block_nr, char *block)
{
	
	if(log_block_nr >= NR_MAX_BLOCK){
		return EFBIG;
	}
	
	int phy_block_nr;
	char indirect[BLOCK_SIZE];
	int indirect_allocated = 0;

	assert(log_block_nr >= 0);
	phy_block_nr = testfs_read_block(in, log_block_nr, block);

	/* phy_block_nr > 0: block exists, so we don't need to allocate it, 
	   phy_block_nr < 0: some error */
	if (phy_block_nr != 0)
		return phy_block_nr;

	/* allocate a direct block */
	if (log_block_nr < NR_DIRECT_BLOCKS) {
		assert(in->in.i_block_nr[log_block_nr] == 0);
		phy_block_nr = testfs_alloc_block_for_inode(in);
		if (phy_block_nr >= 0) {
			in->in.i_block_nr[log_block_nr] = phy_block_nr;
		}
		return phy_block_nr;
	}

	log_block_nr -= NR_DIRECT_BLOCKS;
	if (log_block_nr >= NR_INDIRECT_BLOCKS){
		//TBD();
		log_block_nr  = log_block_nr - NR_INDIRECT_BLOCKS;
		
		int offset = log_block_nr % addr_size;
		int pg = log_block_nr / addr_size;
		char nr1[BLOCK_SIZE];
		char nr2[BLOCK_SIZE];
		bzero(nr1,BLOCK_SIZE);
		bzero(nr2,BLOCK_SIZE);
		
		int block_check = 0;
		int pg_check = 0;
		int block_nr1 = 0;
		int block_nr2 = 0;
		
		if(in->in.i_dindirect != 0){
			read_blocks(in->sb, nr2, in->in.i_dindirect, 1);
			block_nr2 = in->in.i_dindirect;
			
		}
		else{
			block_nr2 = testfs_alloc_block_for_inode(in);
			if(block_nr2 < 0){
				return block_nr2;
			}
			block_check++;
			in->in.i_dindirect = block_nr2;
			
		}
				
		if(((int*)nr2)[pg] == 0){
			block_nr1 = testfs_alloc_block_for_inode(in);
			if(block_nr1 < 0){
				return block_nr1;
			}
			
			((int*)nr2)[pg] = block_nr1;
			pg_check++;
		
		}
		else{
			read_blocks(in->sb, nr1,((int*)nr2)[pg], 1);
			block_nr1 = ((int*)nr2)[pg];
		}
		
		phy_block_nr = testfs_alloc_block_for_inode(in);
		if(phy_block_nr >= 0){
			((int*)nr1)[offset] = phy_block_nr;
			write_blocks(in->sb, nr1, block_nr1, 1);
			if(pg_check){
				write_blocks(in->sb, nr2, block_nr2, 1);

			}
		}
		else{
			if(block_check){
				testfs_free_block_from_inode(in, block_nr2);
				in->in.i_dindirect = 0;
			}
			if(pg_check){
				testfs_free_block_from_inode(in, block_nr1);
				((int*)nr2)[pg] = 0;
				write_blocks(in->sb, nr2, block_nr2, 1);
			
			}
			
		}
		return phy_block_nr;
	}

	if (in->in.i_indirect == 0) {	/* allocate an indirect block */
		bzero(indirect, BLOCK_SIZE);
		phy_block_nr = testfs_alloc_block_for_inode(in);
		if (phy_block_nr < 0)
			return phy_block_nr;
		indirect_allocated = 1;
		in->in.i_indirect = phy_block_nr;
	} else {	/* read indirect block */
		read_blocks(in->sb, indirect, in->in.i_indirect, 1);
	}

	/* allocate direct block */
	assert(((int *)indirect)[log_block_nr] == 0);	
	phy_block_nr = testfs_alloc_block_for_inode(in);

	if (phy_block_nr >= 0) {
		/* update indirect block */
		((int *)indirect)[log_block_nr] = phy_block_nr;
		write_blocks(in->sb, indirect, in->in.i_indirect, 1);
	} else if (indirect_allocated) {
		/* free the indirect block that was allocated */
		testfs_free_block_from_inode(in, in->in.i_indirect);
		in->in.i_indirect = 0;
	}
	return phy_block_nr;
}
int
testfs_write_data(struct inode *in, const char *buf, off_t start, size_t size)
{
	
	char block[BLOCK_SIZE];
	long block_nr = start / BLOCK_SIZE;
	long block_ix = start % BLOCK_SIZE;
	if(block_nr >= NR_MAX_BLOCK){
		return EFBIG;
	}
	int ret;
	int original_size = size;

	do{
		if (block_ix + size > BLOCK_SIZE) {
			//TBD();
			if(block_nr >= NR_MAX_BLOCK){
				in->i_flags |= I_FLAGS_DIRTY;
				in->in.i_size =  BLOCK_SIZE + block_nr;
				
				return -EFBIG;
			}
			
			ret = testfs_allocate_block(in, block_nr, block);
			if(ret < 0){
				return ret;
			}
			size_t byte_left = BLOCK_SIZE - block_ix;
			memcpy(block+block_ix, buf, byte_left);
			write_blocks(in->sb, block, ret, 1);
			buf  = buf + byte_left;
			size = size - byte_left;
			block_nr++;
			block_ix = 0;	
		}
	}while(size > BLOCK_SIZE);
	/* ret is the newly allocated physical block number */
	
	if(block_nr >= NR_MAX_BLOCK){
		in->in.i_size = BLOCK_SIZE * block_nr;
		in->i_flags |= I_FLAGS_DIRTY;
		return -EFBIG;
	}
	
	
	ret = testfs_allocate_block(in, block_nr, block);
	if (ret < 0)
		return ret;
	memcpy(block + block_ix, buf, size);
	write_blocks(in->sb, block, ret, 1);
	/* increment i_size by the number of bytes written. */
	if (size > 0)
		in->in.i_size = MAX(in->in.i_size, start + (off_t) original_size);
	in->i_flags |= I_FLAGS_DIRTY;
	/* return the number of bytes written or any error */
	return original_size;
}

int
testfs_free_blocks(struct inode *in)
{
	int i;
	int e_block_nr;
	/* last block number */
	e_block_nr = DIVROUNDUP(in->in.i_size, BLOCK_SIZE);

	/* remove direct blocks */
	for (i = 0; i < e_block_nr && i < NR_DIRECT_BLOCKS; i++) {
		if (in->in.i_block_nr[i] == 0)
			continue;
		testfs_free_block_from_inode(in, in->in.i_block_nr[i]);
		in->in.i_block_nr[i] = 0;
	}
	e_block_nr -= NR_DIRECT_BLOCKS;

	/* remove indirect blocks */
	if (in->in.i_indirect > 0) {
		char block[BLOCK_SIZE];
		read_blocks(in->sb, block, in->in.i_indirect, 1);
		for (i = 0; i < e_block_nr && i < NR_INDIRECT_BLOCKS; i++) {
			testfs_free_block_from_inode(in, ((int *)block)[i]);
			((int *)block)[i] = 0;
		}
		testfs_free_block_from_inode(in, in->in.i_indirect);
		in->in.i_indirect = 0;
	}

	e_block_nr -= NR_INDIRECT_BLOCKS;
    // remove dindirect blocks 
	if (e_block_nr >= 0) {
		//TBD();
		if(in->in.i_dindirect >0){
		    char nr1[BLOCK_SIZE];
		    read_blocks(in->sb, nr1, in->in.i_dindirect, 1);
		    int j = 0;
		    while(j < NR_INDIRECT_BLOCKS){
		        if (((int*)nr1)[j] == 0){
						j++;
		            continue;
					}
					
		        int n=0;
		        char nr2[BLOCK_SIZE];
		        read_blocks(in->sb,nr2,((int*)nr1)[j],1);
		        while(n < NR_INDIRECT_BLOCKS){
		            	if(((int*)nr2)[n] ==0){
					n++;
					continue;	
				}
		                
		            	testfs_free_block_from_inode(in, ((int*)nr2)[n]);
		            	((int*)nr2)[n]=0;
				n++;
		        }
		        testfs_free_block_from_inode(in,((int*)nr1)[j]);
		        ((int*)nr1)[j] = 0;
			j++;
		    }
		    testfs_free_block_from_inode(in, in->in.i_dindirect);
		    in->in.i_dindirect = 0;
		}
	}

	in->in.i_size = 0;
	in->i_flags |= I_FLAGS_DIRTY;
	return 0;
}
