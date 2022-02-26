#include "scene.hpp"
#include "cursorHelper.h"

class Scene : public GraphicsScene
{
	CComPtr<ID2D1SolidColorBrush> m_pFill;
	CComPtr<ID2D1SolidColorBrush> m_pStroke;
	CComPtr<ID2D1SolidColorBrush> m_stC;
	CComPtr<ID2D1SolidColorBrush> m_stCC;

	D2D1_ELLIPSE          m_ellipse;
	D2D1_ELLIPSE          m_ellipse2;

	HRESULT CreateDeviceIndependentResources() { return S_OK; }
	void    DiscardDeviceIndependentResources() { }
	HRESULT CreateDeviceDependentResources();
	void    DiscardDeviceDependentResources();
	void    CalculateLayout();
	void    RenderScene();
public:
	HWND hwnd;
};

HRESULT Scene::CreateDeviceDependentResources()
{
	HRESULT hr = m_pRenderTarget->CreateSolidColorBrush(
		D2D1::ColorF(1.f, .7f, 0),
		D2D1::BrushProperties(),
		&m_pFill
	);

	if (SUCCEEDED(hr))
	{
		hr = m_pRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(0, 1, 1),
			D2D1::BrushProperties(),
			&m_pStroke
		);
		if (SUCCEEDED(hr))
		{
			hr = m_pRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::DarkBlue),
				D2D1::BrushProperties(),
				&m_stC
			);
			if (SUCCEEDED(hr))
			{
				hr = m_pRenderTarget->CreateSolidColorBrush(
					D2D1::ColorF(D2D1::ColorF::Red),
					D2D1::BrushProperties(),
					&m_stCC
				);
			}
		}
	}
	return hr;
}

#include <math.h>

void Scene::RenderScene()
{
	m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));

	m_pRenderTarget->FillEllipse(m_ellipse, m_pFill);
	m_pRenderTarget->DrawEllipse(m_ellipse, m_pStroke);

	POINT h = cCalc(hwnd);
	float max = .8F * m_ellipse.radiusY,
		dist = sqrtf(powf(h.x - m_ellipse.point.x, 2) + powf(h.y - m_ellipse.point.y, 2)),
		min = m_ellipse2.radiusX;
	m_pRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Translation(0, -1.f * (max <= dist ? max : (min >= dist ? min : dist))) *
		D2D1::Matrix3x2F::Rotation(cCalc(h, m_ellipse.point) + 90, m_ellipse.point)
	);
	m_pRenderTarget->FillEllipse(m_ellipse2, m_stC);

	// Restore the identity transformation.
	m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	m_pRenderTarget->FillEllipse(m_ellipse2, m_stCC);
}

void Scene::CalculateLayout()
{
	D2D1_SIZE_F fSize = m_pRenderTarget->GetSize();

	float x = fSize.width / 2.0f;
	float y = fSize.height / 2.0f;
	float radius = min(x, y);

	m_ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius);

	// Calculate tick marks.
	auto pt1 = D2D1::Point2F(
		m_ellipse.point.x,
		m_ellipse.point.y
	);

	x = fSize.width / 20.0f;
	y = fSize.height / 20.0f;
	radius = min(x, y);
	m_ellipse2 = D2D1::Ellipse(pt1, radius, radius);
}


void Scene::DiscardDeviceDependentResources()
{
	m_pFill.Release();
	m_pStroke.Release();
}


class MainWindow : public BaseWindow<MainWindow>
{
	HANDLE  m_hTimer;
	Scene m_scene;

	BOOL    InitializeTimer();

public:

	MainWindow() : m_hTimer(NULL)
	{
	}

	void    WaitTimer();

	PCWSTR  ClassName() const { return L"RotDraw Window Class"; }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

// Constants 
const WCHAR WINDOW_NAME[] = L"Rotation Draw";


INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, INT nCmdShow)
{
	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
	{
		return 0;
	}

	MainWindow win;

	if (!win.Create(WINDOW_NAME, WS_OVERLAPPEDWINDOW))
	{
		return 0;
	}

	ShowWindow(win.Window(), nCmdShow);
	// Run the message loop.

	MSG msg = { };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}
		win.WaitTimer();
	}

	CoUninitialize();
	return 0;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hwnd = m_hwnd;

	switch (uMsg)
	{
	case WM_CREATE:
		if (FAILED(m_scene.Initialize()) || !InitializeTimer())
		{
			return -1;
		}
		m_scene.hwnd = hwnd;
		return 0;

	case WM_DESTROY:
		CloseHandle(m_hTimer);
		m_scene.CleanUp();
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	case WM_DISPLAYCHANGE:
	{
		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);
		m_scene.Render(m_hwnd);
		EndPaint(m_hwnd, &ps);
	}
	return 0;

	case WM_SIZE:
	{
		int x = (int)(short)LOWORD(lParam);
		int y = (int)(short)HIWORD(lParam);
		m_scene.Resize(x, y);
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
	return 0;

	case WM_ERASEBKGND:
		return 1;

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}


BOOL MainWindow::InitializeTimer()
{
	m_hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
	if (m_hTimer == NULL)
	{
		return FALSE;
	}

	LARGE_INTEGER li = { 0 };

	if (!SetWaitableTimer(m_hTimer, &li, (1000 / 60), NULL, NULL, FALSE))
	{
		CloseHandle(m_hTimer);
		m_hTimer = NULL;
		return FALSE;
	}

	return TRUE;
}

void MainWindow::WaitTimer()
{
	// Wait until the timer expires or any message is posted.
	if (MsgWaitForMultipleObjects(1, &m_hTimer, FALSE, INFINITE, QS_ALLINPUT)
		== WAIT_OBJECT_0)
	{
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
}
