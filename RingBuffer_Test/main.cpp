// $Id$

#include "ringbuf.h"

#include <stdio.h>
#include <stdlib.h>

struct rb_element {
	char m_szText[80];
};

int
main(int ac, char** av)
{
	int i;
	RingBuffer<rb_element> rbuf;

	for (i = 0; i < 10; i++) {
		rb_element* pelem = new rb_element;
		sprintf(pelem->m_szText, "%d's element", i);
		rbuf.addElement(pelem, - 1);
	}

	for (i = 0; i < 10; i++) {
		printf("%d's element = %s\n", i, rbuf.elementAt(i)->m_szText);
	}

	rbuf.setTop(5);

	for (i = 0; i < 10; i++) {
		printf("%d's element = %s\n", i, rbuf.elementAt(i)->m_szText);
	}

#if 0
	for (i = 0; i < 30; i += 3) {
		printf("%d's element = %s\n", i, rbuf.elementAt(i)->m_szText);
	}

	for (i = -1; i > -30; i -= 3) {
		printf("%d's element = %s\n", i, rbuf.elementAt(i)->m_szText);
	}
#endif

	return 0;
}

