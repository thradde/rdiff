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

#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX 
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "utils.h"

char *ReadFile(const wchar_t *file_name, uint64_t &size, uint64_t min_size)
{
	struct _stat32i64 stbuf;
	if (_wstat32i64(file_name, &stbuf) != 0)
	{
		wprintf(L"can not stat file %s\n", file_name);
		exit(1);
	}

	size = stbuf.st_size;
	if (stbuf.st_size < (long long)min_size)
	{
		wprintf(L"file %s is too small for rdiff algorithm\n", file_name);
		exit(1);
	}

	FILE *fh = _wfopen(file_name, L"rb");
	if (!fh)
	{
		wprintf(L"could not open file %s\n", file_name);
		exit(1);
	}

	char *buf = (char *)malloc(stbuf.st_size);
	if (!buf)
	{
		wprintf(L"out of memory\n");
		fclose(fh);
		exit(1);
	}

	size_t n = fread(buf, 1, stbuf.st_size, fh);
	if (n != stbuf.st_size) 
	{
		wprintf(L"fread() error on file %s\n", file_name);
		fclose(fh);
		exit(1);
	}

	fclose(fh);
	return buf;
}


// returns exit code of called process
// if something fails, 0x7fffffff is returned
// replaces system() call, which has quotation mark hell
uint32_t RunProcess(const wchar_t *application_name, const wchar_t *command_line)
{
	PROCESS_INFORMATION processInformation = { 0 };
	STARTUPINFO startupInfo = { 0 };
	startupInfo.cb = sizeof(startupInfo);

	// cmdline must be non-const and can be modified according to documentation
	LPTSTR cmdline = NULL;
	if (command_line)
		cmdline = _tcsdup(command_line);

	// Create the process
	BOOL result = CreateProcess(application_name, cmdline,
		NULL, NULL, FALSE,
		NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
		NULL, NULL, &startupInfo, &processInformation);

	free(cmdline);

	if (!result)
	{
		// CreateProcess() failed
		// Get the error from the system
		LPVOID lpMsgBuf;
		DWORD dw = GetLastError();
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

		// Display the error
		// TRACE(_T("::executeCommandLine() failed at CreateProcess()\nCommand=%s\nMessage=%s\n\n"), cmdLine, strError);

		// Free resources created by the system
		LocalFree(lpMsgBuf);

		// We failed.
		return 0x7fffffff;
	}
	else
	{
		// Successfully created the process.  Wait for it to finish.
		WaitForSingleObject(processInformation.hProcess, INFINITE);

		// Get the exit code.
		DWORD exitCode;
		result = GetExitCodeProcess(processInformation.hProcess, &exitCode);

		// Close the handles.
		CloseHandle(processInformation.hProcess);
		CloseHandle(processInformation.hThread);

		if (!result)
		{
			// Could not get exit code.
			// TRACE(_T("Executed command but couldn't get exit code.\nCommand=%s\n"), cmdLine);
			return 0x7fffffff;
		}

		// We succeeded.
		return exitCode;
	}
}
