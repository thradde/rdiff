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

#if 1
	typedef uint32_t TOffset;	// sufficient, if processing files < 4 GB
#else
	typedef uint64_t TOffset;
#endif

typedef uint64_t checksum_t;

char *ReadFile(const wchar_t *file_name, uint64_t &size, uint64_t min_size);
checksum_t ComputeChecksum(const char *buffer, size_t len);
uint32_t RunProcess(const wchar_t *application_name, const wchar_t *command_line);
