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
	View(LF_Notifier& lfNotifier,
		 HWND hwndParent, DWORD dwStyle, DWORD dwExStyle,
		 const RECT& rctWindow,
		 DC_Manager* pDCManager,
		 DrawInfo* pDrawInfo);
	virtual ~View();

	virtual void setPosition(filesize_t pos, bool bRedraw);

	// �E�B���h�E���w�肳�ꂽ�Z�`�ɕό`
	virtual void setFrameRect(const RECT& rctFrame, bool bRedraw);
	// �E�B���h�E�Z�`���擾
	virtual void getFrameRect(RECT& rctFrame);

	// �N���C�A���g�̈���w��T�C�Y�ɕύX
	virtual void setViewSize(int width, int height);

	virtual bool setDrawInfo(DrawInfo* pDrawInfo) = 0;

	// �E�B���h�E�T�C�Y���s���E�������̐����{�ɒ���
	void adjustWindowRect(RECT& rctFrame);

	void redrawView()
	{
		::InvalidateRect(m_hwndView, NULL, FALSE);
		::UpdateWindow(m_hwndView);
	}

	bool onLoadFile();
	void onUnloadFile();
	void onSetCursorPos(filesize_t pos);

	// View �N���X���������O�̊��N���X
	class ViewException {};
	// �E�B���h�E�N���X�̓o�^�Ɏ��s�����Ƃ��ɓ��������O
	class RegisterClassError : public ViewException {};
	// �E�B���h�E�̍쐬�Ɏ��s�����Ƃ��ɓ��������O
	class CreateWindowError : public ViewException {};

protected:
	DC_Manager* m_pDCManager;
	DrawInfo* m_pDrawInfo;
	int m_nBytesPerLine;
	filesize_t m_qYOffset;
	ScrollManager<int> m_smHorz;
	ScrollManager<filesize_t> m_smVert;
	HWND m_hwndView, m_hwndParent;

	void bitBlt(const RECT& rcPaint);

	void initScrollInfo();

	virtual void ensureVisible(filesize_t pos, bool bRedraw);
	virtual void setCurrentLine(filesize_t newline, bool bRedraw);

	virtual void onHScroll(WPARAM wParam, LPARAM lParam);
	virtual void onVScroll(WPARAM wParam, LPARAM lParam);
	virtual void onLButtonDown(WPARAM wParam, LPARAM lParam);

	virtual LRESULT viewWndProcMain(UINT, WPARAM, LPARAM);

	static LRESULT CALLBACK viewWndProc(HWND, UINT, WPARAM, LPARAM);
};

#endif
