// $Id$

#pragma warning(disable : 4786)

#define _WIN32_WINNT  0x500  // to support mouse wheel

#include "jumpdlg.h"
#include "LargeFileReader.h"
#include "strutils.h"
#include "resource.h"
#include <assert.h>

JumpDlg::JumpDlg(LF_Notifier& lfNotifier)
	: Dialog(IDD_JUMP),
	  m_lfNotifier(lfNotifier)
{
}

BOOL
JumpDlg::initDialog(HWND hDlg)
{
	LargeFileReader* pLFReader = NULL;
	m_lfNotifier.tryLockReader(&pLFReader, INFINITE);
	if (!pLFReader) {
		return FALSE;
	}

	filesize_t size = pLFReader->size();
	char str[32];
	::CopyMemory(str, "0 - 0x", 6);
	QwordToStr((UINT)size, (UINT)(size >> 32), str + 6);
	str[22] = '\0';
	::SetWindowText(::GetDlgItem(hDlg, IDC_JUMPINFO), str);

	m_lfNotifier.releaseReader(pLFReader);

	filesize_t pos = m_lfNotifier.getCursorPos();
	str[0] = '0';  str[1] = 'x';
	QwordToStr((UINT)pos, (UINT)(pos >> 32), str + 2);
	str[18] = '\0';
	::SetWindowText(::GetDlgItem(hDlg, IDC_JUMPADDRESS), str);

	return TRUE;
}

void
JumpDlg::destroyDialog()
{
}

BOOL
JumpDlg::dialogProcMain(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CALC_SIZE:
			if (HIWORD(wParam) == BN_CLICKED) {
				char szbuf[32], nbuf[32];
				::GetWindowText(::GetDlgItem(m_hwndDlg, IDC_BLOCKSIZE), szbuf, 32);
				::GetWindowText(::GetDlgItem(m_hwndDlg, IDC_BLOCKCOUNT), nbuf, 32);
				if (!lstrlen(szbuf) || !lstrlen(nbuf)) break;
				filesize_t size = ParseNumber(szbuf) * ParseNumber(nbuf);
				szbuf[0] = '0'; szbuf[1] = 'x';
				QwordToStr((int)size, (int)(size >> 32), szbuf + 2);
				szbuf[2 + 16] = '\0';
				::SetWindowText(::GetDlgItem(m_hwndDlg, IDC_JUMPADDRESS), szbuf);
			}
			break;
		case IDOK:
			if (HIWORD(wParam) == BN_CLICKED) {
				char buf[32];
				::GetWindowText(::GetDlgItem(m_hwndDlg, IDC_JUMPADDRESS), buf, 32);
				filesize_t pos = ParseNumber(buf);
				m_lfNotifier.setCursorPos(pos);
			}
			// through down
		case IDCANCEL:
			close();
			break;
		}
		break;

	case WM_CLOSE:
		::EndDialog(m_hwndDlg, 0);
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

