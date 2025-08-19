
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "..\rdiff\utils.h"
#include "..\rdiff\PatchFileHeader.h"


int wmain(int argc, const wchar_t **argv)
{
	const wchar_t *oldfile;
	const wchar_t *newfile;
	const wchar_t *patchfile;

#if 0
	if (argc != 4)
	{
		printf("usage: rpatch <oldfile> <newfile> <patchfile>\n");
		exit(1);
	}
	oldfile = argv[1];
	newfile = argv[2];
	patchfile = argv[3];
#else
	oldfile = L"F:\\tmp\\test rdiff\\vpee3270.dll";
	newfile = L"F:\\tmp\\test rdiff\\vpee3271.patch.dll";
	patchfile = L"F:\\tmp\\test rdiff\\vpe.patch";
#endif

	// use lzma.exe to decompress patchfile
	std::wstring fname = patchfile;
	fname += L".tmp";

	std::wstring cmdline = L"lzma.exe d \"";
	cmdline += patchfile;
	cmdline += L"\" \"" + fname + L"\"";

	RunProcess(NULL, cmdline.c_str());

	uint64_t old_size;
	char *oldbuf = ReadFile(oldfile, old_size, 0);

	// open uncompressed patchfile
	FILE *fh = _wfopen(fname.c_str(), L"rb");
	if (!fh)
	{
		wprintf(L"could not open file %s\n", patchfile);
		exit(1);
	}

	CPatchFileHeader header;
	fread(&header, 1, sizeof(header), fh);

	if (header.m_nMagic != PATCH_FILE_MAGIC)
	{
		wprintf(L"file is not a patch file\n");
		exit(1);
	}

	if (header.m_nVersion != PATCH_FILE_VERSION)
	{
		wprintf(L"patch file has higher version, use newer rpatch version\n");
		exit(1);
	}

	if (header.m_nOffsetSize != sizeof(TOffset))
	{
		if (sizeof(TOffset) == 4)
			wprintf(L"wrong size type, use rpatch.64\n");
		else
			wprintf(L"wrong size type, use rpatch.32\n");
		exit(1);
	}

	uint64_t new_size = header.m_nFileSize;
	char *newbuf = (char *)malloc(new_size);

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

	fclose(fh);

	fh = _wfopen(newfile, L"wb");
	if (!fh)
	{
		wprintf(L"could not create file %s\n", newfile);
		exit(1);
	}

	fwrite(newbuf, 1, new_size, fh);
	fclose(fh);
	_wunlink(fname.c_str());
	wprintf(L"file %s created\n", newfile);

	return 0;
}
