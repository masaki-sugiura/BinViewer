// $Id$

#define _WIN32_WINNT  0x500  // to support mouse wheel

#include "jumpdlg.h"
#include "strutils.h"
#include "resource.h"
#include <assert.h>

HWND JumpDlg::m_hwndParent;
ViewFrame* JumpDlg::m_pViewFrame;

void
JumpDlg::doModal(HWND hwndParent, ViewFrame* pViewFrame)
{
	m_hwndParent = hwndParent;
	m_pViewFrame = pViewFrame;
	HINSTANCE hInst = (HINSTANCE)::GetWindowLong(hwndParent, GWL_HINSTANCE);
	::DialogBox(hInst, MAKEINTRESOURCE(IDD_JUMP),
				hwndParent,
				(DLGPROC)JumpDlg::JumpDlgProc);
}

BOOL
JumpDlg::JumpDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			filesize_t size = m_pViewFrame->getFileSize();
			char str[32];
			::CopyMemory(str, "0 - 0x", 6);
			QuadToStr((UINT)size, (UINT)(size >> 32), str + 6);
			str[22] = '\0';
			::SetWindowText(::GetDlgItem(hDlg, IDC_JUMPINFO), str);
			filesize_t pos = m_pViewFrame->getPosition();
			str[0] = '0';  str[1] = 'x';
			QuadToStr((UINT)pos, (UINT)(pos >> 32), str + 2);
			str[18] = '\0';
			::SetWindowText(::GetDlgItem(hDlg, IDC_JUMPADDRESS), str);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CALC_SIZE:
			{
				char szbuf[32], nbuf[32];
				::GetWindowText(::GetDlgItem(hDlg, IDC_BLOCKSIZE), szbuf, 32);
				::GetWindowText(::GetDlgItem(hDlg, IDC_BLOCKCOUNT), nbuf, 32);
				if (!lstrlen(szbuf) || !strlen(nbuf)) break;
				filesize_t size = ParseNumber(szbuf) * ParseNumber(nbuf);
				szbuf[0] = '0'; szbuf[1] = 'x';
				QuadToStr((int)size, (int)(size >> 32), szbuf + 2);
				szbuf[2 + 16] = '\0';
				::SetWindowText(::GetDlgItem(hDlg, IDC_JUMPADDRESS), szbuf);
			}
			break;
		case IDOK:
			{
				char buf[32];
				::GetWindowText(::GetDlgItem(hDlg, IDC_JUMPADDRESS), buf, 32);
				filesize_t pos = ParseNumber(buf);
				m_pViewFrame->onJump(pos + m_pViewFrame->getPageLineNum() * 8);
				m_pViewFrame->onJump(pos);
			}
			// through down
		case IDCANCEL:
			::EndDialog(hDlg, 0);
			break;
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

