// $Id$

#include "bgb_manager.h"

#include <stdio.h>

static void
putline(const BYTE* buf, int size)
{
	const char* hex = "0123456789ABCDEF";

	for (int k = 0; k < size; k++) {
		putchar(' ');
		putchar(hex[(buf[k] >> 4) & 0x0F]);
		putchar(hex[buf[k] & 0x0F]);
	}

	putchar('\n');
}

int
main(int ac, char** av)
{
	if (ac < 2) {
		fprintf(stderr, "usage: %s filename\n", *av);
		return 1;
	}

	try {
		BGB_Manager bgb_mngr(*++av);

		int total_lines = 0;
		for (int i = 0; i < 1024 * 1024; ) {
			if (i >= bgb_mngr.getFileSize()) {
				fputs("offset overflow\n", stdout);
				break;
			}
			BGBuffer* pbgb = bgb_mngr.getBuffer((filesize_t)i);
			BYTE* buf = pbgb->m_DataBuf;
			int size = pbgb->m_nDataSize;
			total_lines += (size + 15) / 16;
			int line_num = size / 16;
			size -= line_num * 16;
			for (int n = 0; n < line_num; n++) {
				putline(buf, 16);
				buf += 16;
			}
			putline(buf, size);
			i += pbgb->m_nDataSize;
		}
		printf("output %d lines\n", total_lines);
	} catch (FileOpenError& e) {
		fprintf(stderr, "cannot open a file: %s\n",
				e.filename().c_str());
	} catch (FileSeekError& e) {
		fprintf(stderr, "cannot seek a file: %s\n",
				e.filename().c_str());
	} catch (...) {
		fprintf(stderr, "unknown fatal error has occured.\n");
		throw;
	}

	return 0;
}

