// $Id$

#ifndef VIEW_H_INC
#define VIEW_H_INC

/*
	DC_Manager �𗘗p����E�B���h�E��\�����ۊ��N���X

	�E��ʂ̕`��i�o�b�N�O���E���h�o�b�t�@����̃r�b�g�}�b�v�]���j
	�E�`��̈�̃T�C�Y�Ǘ�
	�E�J�[�\���ʒu�̊Ǘ�
	�E�}�E�X�C�x���g�̃n���h�����O
	�E�L�[����̃n���h�����O

						+-----------------------+
						|						|
	+-----------+		|	+-----------+		|
	|			|		|	|			|		|
	|			|		|	|			|		|
	+-----------+		|	+-----------+		|
						|						|
						+-----------------------+

 */

#include "auto_ptr.h"
#include "scroll.h"
#include "dc_manager.h"

class BGB_Manager;

struct Metrics {
	DWORD width;
	DWORD height;
};

class View : public LF_Acceptor {
public:
	View(HWND hwndParent, DWORD dwStyle, DWORD dwExStyle,
		 const RECT& rctWindow,
		 DC_Manager* pDCManager,
		 int nWidth, int nHeight,
		 int nPixelsPerLine,
		 COLORREF crBkColor);
	virtual ~View();

	virtual void ensureVisible(filesize_t pos, bool bRedraw);
	virtual void setCurrentLine(filesize_t newline, bool bRedraw);
	virtual void setPosition(filesize_t pos, bool bRedraw);

	virtual void adjustWindowRect(RECT& rctFrame);
	virtual void setFrameRect(const RECT& rctFrame);

	// View �N���X���������O�̊��N���X
	class ViewException {};
	// �E�B���h�E�N���X�̓o�^�Ɏ��s�����Ƃ��ɓ��������O
	class RegisterClassError : public ViewException {};
	class CreateWindowError : public ViewException {};

protected:
	DC_Manager* m_pDCManager;
	int m_nPixelsPerLine;
	int m_nBytesPerLine;
	filesize_t m_qYOffset;
	ScrollManager<int> m_smHorz;
	ScrollManager<filesize_t> m_smVert;
	HWND m_hwndView, m_hwndParent;
	HDC m_hdcView;
	HBRUSH m_hbrBackground;

	bool onLoadFile();
	void onUnloadFile();

	void bitBlt(const RECT& rcPaint);

	virtual void onHScroll(WPARAM wParam, LPARAM lParam);
	virtual void onVScroll(WPARAM wParam, LPARAM lParam);

	virtual LRESULT viewWndProcMain(UINT, WPARAM, LPARAM);

	static LRESULT CALLBACK viewWndProc(HWND, UINT, WPARAM, LPARAM);
};

#endif
