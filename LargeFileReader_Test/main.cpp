// $Id$

#include "LargeFileReader.h"

#include <stdio.h>

int
main(int ac, char** av)
{
	if (ac < 2) {
		fprintf(stderr, "usage: %s filename\n", *av);
		return 1;
	}

	WIN32_FIND_DATA fd;
	HANDLE hFindFile = ::FindFirstFile(*++av, &fd);
	filesize_t size = fd.nFileSizeLow | ((filesize_t)fd.nFileSizeHigh << 32);
	::FindClose(hFindFile);

	try {
		LargeFileReader lfr(*av);

		const char* hex = "0123456789ABCDEF";
		BYTE buf[16];
		for (int i = 0; i < 1024; i++) {
			filesize_t offset = (filesize_t)i * 1024 * 1024 * 128;
			if (offset >= size) {
				fputs("offset overflow\n", stdout);
			}
			if (lfr.seek(offset) < 0) break;
			int len = lfr.read(buf, 16);
			if (len <= 0) break;
			for (int j = 0; j < len; j++) {
				putchar(' ');
				putchar(hex[(buf[j] >> 4) & 0x0F]);
				putchar(hex[buf[j] & 0x0F]);
			}
			putchar('\n');
		}
		printf("output %d lines\n", i);
	} catch (FileOpenError& e) {
		fprintf(stderr, "cannot open a file: %s\n",
				e.filename().c_str());
	} catch (FileSeekError& e) {
		fprintf(stderr, "cannot seek a file: %s\n",
				e.filename().c_str());
	} catch (...) {
		fprintf(stderr, "unknown fatal error has occured.\n");
	}

	return 0;
}

#if 0
// create a large data file
int
main(int ac, char** av)
{
	if (ac < 2) {
		fprintf(stderr, "usage: %s filename\n", *av);
		return 1;
	}

	FILE* fp = fopen(*++av, "wb");
	if (!fp) {
		fprintf(stderr, "cannot open the file: %s\n", *av);
		return 1;
	}

	BYTE buf[0x20];
	for (int ii = 0; ii < 0x20; ii++) {
		buf[ii] = (BYTE)ii;
	}
	for (filesize_t i = 0; i < 0x10000000; i++) {
		if (fwrite(buf, 0x20, 1, fp) < 0) break;
	}
	fclose(fp);
	return 0;
}
#endif

