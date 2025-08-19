# Creating patch files - rdiff

If you provide online updates for software, it might be important to provide as small downloads as possible to:

- speed up downloads
- minimize traffic to save cost

This can be achieved by creating delta files between an old version and a new version of a file, which only contains the differences. In addition the patch files are compressed using LZMA.

The past days i worked on a patch tool, which you give an old file and a new file and it computes a delta file. So if your customer makes an update, the tool will take the old file, apply the delta file and generate the new file. 
Depending on the amount of differences, the delta file might be much smaller than shipping the full new file.

There are such tools available - bsdiff is the most prominent - but i wanted to see with what i can come up.

My first naive solution was very very slow. I consider blocks of 16 bytes and take the first block of the old file and iterate over the whole new file to see if I can find the block. If an identical block is found, I start comparing if there are further identical bytes behind the block, to extend the block.

    for i = 0 to old_file_size
        for k = 0 to new_file_size
            compare block at old[i], new[k]

So for 2 files of 2 MB size, it makes 2 mio x 2 mio comparisions (16 bytes each compare). This is 4.000.000.000.000 comparisions, each 16 bytes. This took about an hour and was far too slow.

I also had to do housekeeping to not touch blocks that already had been identified as equal.

I then developed a 2nd solution, inspired by the rsync algorithm. I compute a checksum for each block of the old file.

    for i = 0 to old_file_size
        compute block checksum old[i]

I store the checksums in a map for fast searching. The checksums are computed using the fast XXH3 hash algorithm. You need to adjust the #include of "xxHash\xxh3.h" to point to your installation of the xxHash library. It should be noted that there can be many collisions with identical checksums. This is so, because there are identical regions in executables, for example 100 bytes only zeros at different positions, which all cause the same checksum to be computed.

Then I iterate over the new file and compute for each block a checksum and search in the map for an identical checksum.

    for k = 0 to new_file_size
        compute block checksum new[k]
        search checksum in map

Instead of i * k, I have only i + k computations to perform.


If found, I compare the blocks byte for byte if they are really equal. If so, I extend to compare further successive bytes if they are equal. I then store such a block in a list with position in old file, position in new file and size. From this list, I can construct a delta file (also called patch file).

The code to create the new file from the old file and the patch file is very simple:

	char cmd;
	TOffset size;
	TOffset oldoffset;
	uint64_t k = 0;
	while (k < new_size)
	{
		fread(&cmd, 1, sizeof(cmd), fh);

		if (cmd == BlockTypeInsert)
		{
			fread(&size, 1, sizeof(size), fh);
			fread(newbuf + k, 1, size, fh);
		}
		else
		{
			fread(&oldoffset, 1, sizeof(oldoffset), fh);
			fread(&size, 1, sizeof(size), fh);
			memcpy(newbuf + k, oldbuf + oldoffset, size);
		}

		k += size;
	}

The whole code of rpatch.exe is only 120 lines.

Let me tell an observation: comparing 2 versions of my own software, which had only minor changes from one version to the next, I expected to identify huge similar blocks. But no, for identical blocks, block sizes are 30 - 100 bytes in average only. It seems that there are many jumps to absolute addresses, which are all changed in the new version. But even then, in my case the patch file is 268 KB in size and the 
original file 2,715 KB, so the patch file is only about 10% in size.
