

#define PATCH_FILE_MAGIC	0x20241118
#define PATCH_FILE_VERSION	1

class CPatchFileHeader
{
public:
	uint32_t	m_nMagic;		// magic header
	uint32_t	m_nVersion;		// version of patch file
	uint32_t	m_nOffsetSize;	// 4 = offsets and sizes are 4 byte, 8 otherwise
	uint64_t	m_nFileSize;	// size of file to create

public:
	CPatchFileHeader(uint64_t file_size, uint32_t offset_size)
		: m_nMagic(PATCH_FILE_MAGIC)
		, m_nVersion(PATCH_FILE_VERSION)
		, m_nFileSize(file_size)
		, m_nOffsetSize(offset_size)
	{
	}

	CPatchFileHeader()
		: m_nMagic(0)
		, m_nVersion(0)
		, m_nFileSize(0)
		, m_nOffsetSize(0)
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
