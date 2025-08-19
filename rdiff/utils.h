
#if 1
	typedef uint32_t TOffset;	// sufficient, if processing files < 4 GB
#else
	typedef uint64_t TOffset;
#endif

char *ReadFile(const wchar_t *file_name, uint64_t &size, uint64_t min_size);
uint32_t RunProcess(const wchar_t *application_name, const wchar_t *command_line);
