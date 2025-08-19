/*
 * This file is part of rdiff (https://github.com/thradde/rdiff).
 * Copyright (c) 2025 Thorsten Radde.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
If you provide online updates for software, it might be important to provide as small downloads as possible to:

a) speed up downloads
b) minimize traffic to save cost

This can be achieved by creating delta files between an old version and a new version of a file, which only 
contains the differences. In addition the patch files are compressed using LZMA.

The past days i worked on a patch tool, which you give an old file and a new file and it computes a delta file.
So if your customer makes an update, the tool will take the old file, apply the delta file and generate the new file. 
Depending on the amount of differences, the delta file might be much smaller than shipping the full new file.

There are such tools available - bsdiff is the most prominent - but i wanted to see with what i can come up.

My first naive solution was very very slow. I consider blocks of 16 bytes and take the first block of the 
old file and iterate over the whole new file to see if I can find the block. If an identical block is found, 
I start comparing if there are further identical bytes behind the block, to extend the block.

for i = 0 to old_file_size
	for k = 0 to new_file_size
		compare block at old[i], new[k]

So for 2 files of 2 MB size, it makes 2 mio x 2 mio comparisions (16 bytes each compare). 
This is 4.000.000.000.000 comparisions, each 16 bytes. This took about an hour and was far too slow.

I also had to do housekeeping to not touch blocks that already had been identified as equal.

I then developed a 2nd solution, inspired by the rsync algorithm. I compute a checksum for each block of the old file.

for i = 0 to old_file_size
	compute block checksum old[i]

I store the checksums in a map for fast searching. 
The checksums are computed using the fast XXH3 hash algorithm. You need to adjust the #include of "xxHash\xxh3.h"
to point to your installation of the xxHash library.
It should be noted that there can be many collisions with identical checksums.
This is so, because there are identical regions in executables, for example 100 bytes 
only zeros at different positions, which all cause the same checksum to be computed.

Then I iterate over the new file and compute for each block a checksum and search in the map for an identical checksum.

for k = 0 to new_file_size
	compute block checksum new[k]
	search checksum in map

Instead of i * k, I have only i + k computations to perform.

If found, I compare the blocks byte for byte if they are really equal. 
If so, I extend to compare further successive bytes if they are equal. 
I then store such a block in a list with position in old file, position in new file and size.

From this list, I can construct a delta file (also called patch file).

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

Let me tell an observation: comparing 2 versions of my own software, which had only minor changes from 
one version to the next, I expected to identify huge similar blocks. But no, for identical blocks, block sizes 
are 30 - 100 bytes in average only. It seems that there are many jumps to absolute addresses, which are 
all changed in the new version. But even then, in my case the patch file is 268 KB in size and the 
original file 2,715 KB, so the patch file is only about 10% in size.
*/

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <list>
#include <map>

#include "\source_andere\xxHash\xxh3.h"
#include "utils.h"
#include "PatchFileHeader.h"

//#define VERBOSE

typedef uint64_t checksum_t;

class CSearchNode
{
public:
	std::list<TOffset>	m_listOffsets;

public:
	CSearchNode(TOffset offset)
	{
		m_listOffsets.push_back(offset);
	}

	CSearchNode()
	{
	}
};

std::map<checksum_t, CSearchNode *> gSearchMap;


class CBlock
{
public:
	TOffset	m_nNewOffset;		// offset in new file where this block is located
	TOffset	m_nSize;			// size of block
	TOffset	m_nOldOffset;		// offset in old file where this block is located

public:
	CBlock(TOffset new_off, TOffset size, TOffset old_off)
		: m_nNewOffset(new_off)
		, m_nSize(size)
		, m_nOldOffset(old_off)
	{
	}
};

std::list<CBlock> gBlockList;
typedef std::list<CBlock>::iterator TBlockListIter;

constexpr size_t BlockSize = 16;


// Compute Checksum for a block
// len in bytes
checksum_t ComputeChecksum(const char *buffer, size_t len)
{
	checksum_t hash = XXH3_64bits(buffer, len);
	//checksum_t hash = XXH3_128bits(buffer, len);

	return hash;
}


int wmain(int argc, const wchar_t **argv)
{
	const wchar_t *oldfile;
	const wchar_t *newfile;
	const wchar_t *patchfile;

#if 0
	if (argc != 4)
	{
		printf("usage: rdiff <oldfile> <newfile> <patchfile>\n");
		exit(1);
	}
	oldfile = argv[1];
	newfile = argv[2];
	patchfile = argv[3];
#else
	oldfile = L"F:\\tmp\\test rdiff\\vpee3270.dll";
	newfile = L"F:\\tmp\\test rdiff\\vpee3271.dll";
	patchfile = L"F:\\tmp\\test rdiff\\vpe.patch";
#endif

	uint64_t old_size, new_size;
	char *oldbuf = ReadFile(oldfile, old_size, BlockSize);
	char *newbuf = ReadFile(newfile, new_size, BlockSize);

	// compute search map for old file
	wprintf(L"pass 1, computing search map\n");
	TOffset i = 0;
	while (i < old_size - BlockSize)
	{
		checksum_t csum = ComputeChecksum(oldbuf + i, BlockSize);

		// check for collision, i.e. the checksum already exists
		// this can happen due to the nature of checksums, but
		// it can especially happen, because regions of a file may be identical,
		// for example blocks of zero-bytes at different offsets.
		auto it = gSearchMap.find(csum);
		if (it != gSearchMap.end())
		{
			it->second->m_listOffsets.push_back(i);	// collision
		}
		else
		{
			CSearchNode *node = new CSearchNode(i);
			gSearchMap[csum] = node;
		}

		i++;
	}

	wprintf(L"pass 2, search identical blocks in new file\n");
	uint64_t total_size_to_copy = 0;
	TOffset k = 0;
	while (k < new_size - BlockSize)
	{
		checksum_t csum = ComputeChecksum(newbuf + k, BlockSize);

		auto it = gSearchMap.find(csum);
		if (it != gSearchMap.end())
		{
			// found in search map, now compare each block byte for byte
			for (auto it_off = it->second->m_listOffsets.begin(); it_off != it->second->m_listOffsets.end(); it_off++)
			{
				i = *it_off;
				if (memcmp(oldbuf + i, newbuf + k, BlockSize) != 0)
					continue;

				// identical block found in new file
				// check, if there are additional equal bytes
				TOffset ii = i + BlockSize;
				TOffset kk = k + BlockSize;
				while (ii < old_size && kk < new_size)
				{
					if (*(oldbuf + ii) != *(newbuf + kk))
						break;
					ii++;
					kk++;
				}

				TOffset size = ii - i;
				gBlockList.emplace_back(k, size, i);
#ifdef VERBOSE
				wprintf(L"identical block found.\n"
					"old file offset %ld\n"
					"new file offset %ld\n"
					"size %ld\n\n",
					i, k, size);
#endif
				k += size;
				total_size_to_copy += size;

				// remove entry from search-map
				it->second->m_listOffsets.erase(it_off);
				break;
			}
		}

		k++;
	}

#ifdef VERBOSE
	i = 0;
	for (auto &it : gBlockList)
	{
		wprintf(L"identical block found.\n"
			"old file offset %ld\n"
			"new file offset %ld\n"
			"size %ld\n\n",
			it.m_nOldOffset, it.m_nNewOffset, it.m_nSize);
		i++;
		if (i == 10)
			break;
	}
#endif

	wprintf(L"pass 3, building patch file\n"
		"total_size_to_copy %lld\n", total_size_to_copy);

	FILE *fh = _wfopen(patchfile, L"wb");
	if (!fh)
	{
		wprintf(L"could not create file %s\n", patchfile);
		exit(1);
	}

	CPatchFileHeader header(new_size, sizeof(TOffset));
	fwrite(&header, 1, sizeof(header), fh);

	k = 0;
	char cmd;
	TBlockListIter it = gBlockList.begin();
	while (it != gBlockList.end())
	{
		if (k < it->m_nNewOffset)
		{
			cmd = BlockTypeInsert;
			fwrite(&cmd, 1, sizeof(cmd), fh);

			TOffset size = it->m_nNewOffset - k;
			fwrite(&size, 1, sizeof(size), fh);
			fwrite(newbuf + k, 1, size, fh);
			k += size;
		}
		else
		{
			cmd = BlockTypeCopy;
			fwrite(&cmd, 1, sizeof(cmd), fh);
			fwrite(&it->m_nOldOffset, 1, sizeof(it->m_nOldOffset), fh);
			fwrite(&it->m_nSize, 1, sizeof(it->m_nSize), fh);
			k += it->m_nSize;
			it++;
		}
	}

	// write final block
	if (k < new_size)
	{
		cmd = BlockTypeInsert;
		fwrite(&cmd, 1, sizeof(cmd), fh);

		TOffset size = (TOffset)new_size - k;
		fwrite(&size, 1, sizeof(size), fh);
		fwrite(newbuf + k, 1, size, fh);
	}

	fclose(fh);

	// use lzma.exe to compress
	std::wstring fname = patchfile;
	fname += L".tmp";

	std::wstring cmdline = L"lzma.exe e \"";
	cmdline += patchfile;
	cmdline += L"\" \"" + fname + L"\"";

	RunProcess(NULL, cmdline.c_str());
	_wunlink(patchfile);
	_wrename(fname.c_str(), patchfile);

	wprintf(L"patch file %s created\n", patchfile);

	return 0;
}
