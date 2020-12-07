/* fs/truncate.c */

/* Reset the inode zones and the bits of zones in zonemap */

/* free level-1 indirect block */
static void free_ind(int dev, int block) {
	struct buffer_head * bh;
	unsigned short * p;
	int i;

	if (!block)
		return;
	if (bh = bread(dev, block)) {
		p = (unsigned short *)bh->b_data;
		for (i=0; i<512; i++, p++)  // 512 zones per zone(1K)
			if (*p)
				free_block(dev, *p);
		brelse(bh);
	}
	// free the level-1 block itself
	free_block(dev, block);
}

/* free level-2 indirect block */
static void free_dind(int dev, int block) {
	struct buffer_head * bh;
	unsigned short * p;
	int i;

	if (!block)
		return;
	if (bh = bread(dev, block)) {
		for (i=0; i<512; i++, p++)
			if (*p)
				free_ind(dev, *p);  // free level-1 indirect block
		brelse(bh);
	}
	free_block(dev, block);
}

void truncate(struct m_inode * inode) {
	int i;

	if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode)))
		return;
	// truncate direct zone
	for (i=0; i<7; i++)
		if (inode->i_zone[i]) {
			free_block(inode->i_dev, inode->i_zone[i]);
			inode->i_zone[i] = 0;
		}
	// truncate level-1 indirect zone
	free_ind(inode->i_dev, inode->i_zone[7]);
	// truncate level-2 indirect zone
	free_dind(inode->i_dev, inode->i_zone[8]);
	inode->i_zone[7] = inode->i_zone[8] = 0;
	inode->i_size = 0;
	inode->i_dirt = 1;
	inode->i_mtime = inode->i_ctime = CURRENT_TIME;
}
