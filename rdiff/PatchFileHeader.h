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

#define PATCH_FILE_MAGIC	0x20241118
#define PATCH_FILE_VERSION	1

class CPatchFileHeader
{
public:
	uint32_t	m_nMagic;		// magic header
	uint32_t	m_nVersion;		// version of patch file
	uint32_t	m_nOffsetSize;	// 4 = offsets and sizes are 4 byte, 8 otherwise
	uint64_t	m_nFileSize;	// size of file to create
	checksum_t	m_nOldChecksum;	// checksum of old file
	checksum_t	m_nNewChecksum;	// checksum of new file

public:
	CPatchFileHeader(uint64_t file_size, uint32_t offset_size, checksum_t chk_old, checksum_t chk_new)
		: m_nMagic(PATCH_FILE_MAGIC)
		, m_nVersion(PATCH_FILE_VERSION)
		, m_nFileSize(file_size)
		, m_nOffsetSize(offset_size)
		, m_nOldChecksum(chk_old)
		, m_nNewChecksum(chk_new)
	{
	}

	CPatchFileHeader()
		: m_nMagic(0)
		, m_nVersion(0)
		, m_nFileSize(0)
		, m_nOffsetSize(0)
		, m_nOldChecksum(0)
		, m_nNewChecksum(0)
	{
	}
};


// Block header
//
// BlockTypeCopy
// old offset
// size
//
// or
//
// BlockTypeInsert
// size
// ... data ...
enum
{
	BlockTypeCopy,		// copy from old file
	BlockTypeInsert,	// insert new
};
