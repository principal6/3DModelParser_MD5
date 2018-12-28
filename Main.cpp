#include <windows.h>
#include "ModelMD5.h"
#include "DirectInput.h"
#include "Camera.h"
#include "Font.h"
#include "Grid.h"
#include <crtdbg.h>

#ifdef _DEBUG		// ����� ��� �޸� ���� �˻��
#define new new( _CLIENT_BLOCK, __FILE, __LINE__)
#endif


// ��� ����
const int				ScreenWidth			= 800;
const int				ScreenHeight		= 600;
const char				WindowName[]		= "D3DGAME";
const char				WindowTitle[]		= "GAME";
const int				MAX_MODEL_NUM		= 10;


// D3D ���� ����
LPDIRECT3D9             g_pD3D				= NULL;
LPDIRECT3DDEVICE9       g_pd3dDevice		= NULL;
LPD3DXEFFECT			g_pHLSL				= NULL;	// HLSL Shader

DirectCamera9			g_Camera;					// Direct Camera
DirectFont9				g_Font;						// Direct Font
DirectGrid9				g_Grid;						// Direct Grid

DirectInput*			g_pDI				= NULL;	// Direct Input
D3DXVECTOR3				MouseState;					// Direct Input - ���콺 �̵�, ��
POINT					MouseScreen;				// Direct Input - ���콺 ��ǥ
int						MouseButtonState	= 0;	// Direct Input - ���콺 ��ư ����

// ��Ÿ ���� ����
ULONGLONG				Timer_App			= 0;
int						FPS					= 0;
int						FPS_Shown			= 0;

ModelMD5				MyMD5Model[MAX_MODEL_NUM];
bool					bDrawBoundingBoxes	= false;
bool					bDrawNormalVectors	= false;
bool					bWireFrame			= false;


// �Լ� ���� ����
VOID Cleanup();
HRESULT InitD3D( HWND hWnd, HINSTANCE hInst );
HRESULT InitModel();
VOID Render();
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT );


HRESULT InitD3D( HWND hWnd, HINSTANCE hInst )
{
	if( NULL == ( g_pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) )
		return E_FAIL;

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.EnableAutoDepthStencil = TRUE;		// �ڵ� ���� ����(Depth Stencil)�� ����Ѵ�. (�׷��� ��ǻ�Ͱ� �˾Ƽ� �ָ� �ִ� �� ���� �׷��ܡ�)
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;	// ���� ���� ����: D3DFMT_D16
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;	// �ĸ� ���� ����: D3DFMT_UNKNOWN�� ���� ����ȭ�� ���˰� ��ġ
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	if( FAILED( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice ) ) )
		return E_FAIL;

	g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );

	// ���̴� ������
	D3DXCreateEffectFromFile(g_pd3dDevice, "HLSLMain.fx", NULL, NULL, NULL, NULL, &g_pHLSL, NULL);
	if (!g_pHLSL)
		return E_FAIL;

	InitModel();

	g_Grid.SetDevice(g_pd3dDevice);
	g_Grid.AddGridXZ(1, 50, 100);
	g_Grid.CreateGrid();

	g_pDI = DirectInput::GetInstance();		// Direct Input �ʱ�ȭ
	g_pDI->InitDirectInput(hInst, hWnd);
	g_pDI->CreateKeyboardDevice(DISCL_BACKGROUND|DISCL_NONEXCLUSIVE);
	g_pDI->CreateMouseDevice(DISCL_BACKGROUND|DISCL_NONEXCLUSIVE);

	g_Camera.SetDevice(g_pd3dDevice);
	g_Camera.SetCamera_FreeLook(-5.0f, 4.0f, -5.0f);
	
	g_Font.CreateFontA(g_pd3dDevice, "Arial", 20, false, ScreenWidth, ScreenHeight);

	return S_OK;
}

HRESULT InitModel()
{
	// �� ����
	MyMD5Model[0].CreateModel(g_pd3dDevice, "Model\\", "Ezreal", g_pHLSL);
	MyMD5Model[0].HLSLInstancing = false;

	MyMD5Model[0].AddAnimation(0, "EzrealIdle", 0.02f);
	MyMD5Model[0].AddAnimation(1, "EzrealWalk", 0.02f);
	MyMD5Model[0].AddAnimation(2, "EzrealPunching", 0.02f);
	MyMD5Model[0].AddAnimation(3, "EzrealStanding1HMagicAttack01", 0.02f);
	MyMD5Model[0].AddAnimationEnd();

	// �� �ν��Ͻ� ����
	MyMD5Model[0].AddInstance( XMFLOAT3(1, -4.0f, 0), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.1f, 0.1f, 0.1f) );
	MyMD5Model[0].AddInstance( XMFLOAT3(10, -4.0f, 0), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.1f, 0.1f, 0.1f) );
	MyMD5Model[0].SetInstanceAnimation(0, 1, 0.0f);	// �ִϸ��̼� ���� �ð� 0���� �ʱ�ȭ
	MyMD5Model[0].SetInstanceAnimation(1, 2, 0.0f);	// �ִϸ��̼� ���� �ð� 0���� �ʱ�ȭ
	MyMD5Model[0].AddInstanceEnd();

	return S_OK;
}

void DetectInput(HWND hWnd)
{
	if(GetFocus())	// Direct Input�� �Է��� ������
	{
		MouseState = g_pDI->DIMouseHandler();

		GetCursorPos(&MouseScreen);			// ������ �� ���콺 ��ǥ ������
		ScreenToClient(hWnd, &MouseScreen);

		MouseButtonState = 0;

		if (g_pDI->DIMouseButtonHandler(0))	// ���콺 ���� ��ư
		{
			MouseButtonState = 1;
		}

		if (g_pDI->DIMouseButtonHandler(1))	// ���콺 ������ ��ư
		{
			MouseButtonState = 2;
		}

		if (MouseState.x < 0)	// ���콺 �������� �̵�
		{
			g_Camera.RotateCamera_LeftRight(MouseState.x, 250.0f);
		}

		if (MouseState.x > 0)	// ���콺 ���������� �̵�
		{
			g_Camera.RotateCamera_LeftRight(MouseState.x, 250.0f);
		}

		if (MouseState.y < 0)	// ���콺 ���� �̵�
		{
			g_Camera.RotateCamera_UpDown(MouseState.y, 250.0f);
		}

		if (MouseState.y > 0)	// ���콺 �Ʒ��� �̵�
		{
			g_Camera.RotateCamera_UpDown(MouseState.y, 250.0f);
		}

		if(g_pDI->DIKeyboardHandler(DIK_W))	// W: ������ �̵�
		{
			g_Camera.MoveCamera_BackForth(false, 1.0f);
		}

		if(g_pDI->DIKeyboardHandler(DIK_S))	// S: �ڷ� �̵�
		{
			g_Camera.MoveCamera_BackForth(true, 1.0f);
		}

		if(g_pDI->DIKeyboardHandler(DIK_A))	// A: �������� �̵�
		{
			g_Camera.MoveCamera_LeftRight(true, 1.0f);
		}
		
		if(g_pDI->DIKeyboardHandler(DIK_D))	// D: ���������� �̵�
		{
			g_Camera.MoveCamera_LeftRight(false, 1.0f);
		}

		if(g_pDI->DIKeyboardHandler(DIK_B))	// B: �ٿ�� �ڽ� �׸���
		{
			bDrawBoundingBoxes = !bDrawBoundingBoxes;
			Sleep(140);
		}

		if(g_pDI->DIKeyboardHandler(DIK_N))	// N: ���� ���� �׸���
		{
			bDrawNormalVectors = !bDrawNormalVectors;
			Sleep(140);
		}

		if(g_pDI->DIKeyboardHandler(DIK_R))	// R: �׸��� ���
		{
			bWireFrame = !bWireFrame;
			Sleep(140);

			if (bWireFrame == true)
			{
				g_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
			}
			else
			{
				g_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
			}
		}

		if(g_pDI->DIKeyboardHandler(DIK_SPACE))	// Spacebar: ī�޶� ��ġ, ���� �ʱ�ȭ
		{
			g_Camera.SetCamera_FreeLook(0.0f, 0.0f, 0.0f);
			g_Camera.CamYaw = 0;
			g_Camera.CamRoll = 0;
			Sleep(140);
		}

		if(g_pDI->DIKeyboardHandler(DIK_ESCAPE))	// ESC: ����
			DestroyWindow(hWnd);
	}
}

VOID Render()
{
	// �ĸ� ���ۿ� Z ���۸� û���Ѵ�.
	g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 0, 200, 100 ), 1.0f, 0 );

	if( SUCCEEDED( g_pd3dDevice->BeginScene() ) )
	{
		// ī�޶� ����
		g_Camera.UseCamera_FreeLook();
		g_Camera.SetProjection(10000.0f);
		
		// ���� ��� (HLSL �� �� ���� ������ �־�� ��!!!)
		g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );

		g_Grid.DrawGrid();

		// �� �ν��Ͻ� ���� ����
		for (int i = 0; i < MyMD5Model[0].numInstances; i++)
		{
			MyMD5Model[0].AnimateInstance(i, 0.0f);
			//MyMD5Model[0].DrawNonHLSL(i, g_Camera.GetViewMatrix(), g_Camera.GetProjectionMatrix());
			MyMD5Model[0].DrawHLSL(i, g_Camera.GetViewMatrix(), g_Camera.GetProjectionMatrix());
		}
		//MyMD5Model[0].DrawHLSLInstancing(g_Camera.GetViewMatrix(), g_Camera.GetProjectionMatrix());
		
		if (bDrawBoundingBoxes)
			MyMD5Model[0].DrawBoundingBoxes();

		if (bDrawNormalVectors)
			MyMD5Model[0].DrawNormalVecters(2.0f);

		if (g_pDI->OnMouseButtonDown(0))
		{
			for (int i = 0; i < MyMD5Model[0].numInstances; i++)
			{
				MyMD5Model[0].CheckMouseOverPerInstance(i, MouseScreen.x, MouseScreen.y, ScreenWidth, ScreenHeight,
					g_Camera.GetViewMatrix(), g_Camera.GetProjectionMatrix());
			}
			MyMD5Model[0].CheckMouseOverFinal();
		}

		g_Font.SetFontColor(0xFFFFFFFF);
		g_Font.DrawTextA(0, 0, "FPS: ");
		g_Font.DrawTextA(40, 0, FPS_Shown);
		g_Font.DrawTextA(0, 20, MyMD5Model[0].MouseOverPerInstances[0]);
		g_Font.DrawTextA(0, 40, MyMD5Model[0].PickedPosition[0].x);
		g_Font.DrawTextA(0, 60, MyMD5Model[0].PickedPosition[0].y);
		g_Font.DrawTextA(0, 80, MyMD5Model[0].PickedPosition[0].z);
		g_Font.DrawTextA(100, 20, MyMD5Model[0].MouseOverPerInstances[1]);
		g_Font.DrawTextA(100, 40, MyMD5Model[0].PickedPosition[1].x);
		g_Font.DrawTextA(100, 60, MyMD5Model[0].PickedPosition[1].y);
		g_Font.DrawTextA(100, 80, MyMD5Model[0].PickedPosition[1].z);
		g_Font.DrawTextA(200, 20, MyMD5Model[0].MouseOverPerInstances[2]);
		g_Font.DrawTextA(200, 40, MyMD5Model[0].PickedPosition[2].x);
		g_Font.DrawTextA(200, 60, MyMD5Model[0].PickedPosition[2].y);
		g_Font.DrawTextA(200, 80, MyMD5Model[0].PickedPosition[2].z);

		g_pd3dDevice->EndScene();
	}

	g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
}

LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_DESTROY:
			PostQuitMessage( 0 );
			return 0;
	}

	return DefWindowProc( hWnd, msg, wParam, lParam );
}

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
	#ifdef _DEBUG	// �޸� ���� �˻��
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, WindowName, NULL };
	RegisterClassEx( &wc );

	RECT WndRect = {0, 30, ScreenWidth, 30 + ScreenHeight};
	AdjustWindowRect(&WndRect, WS_OVERLAPPEDWINDOW, false);

	HWND hWnd = CreateWindow( WindowName, WindowTitle, WS_OVERLAPPEDWINDOW,
		WndRect.left, WndRect.top, WndRect.right-WndRect.left, WndRect.bottom-WndRect.top, NULL, NULL, wc.hInstance, NULL );

	if( FAILED( InitD3D( hWnd, hInst ) ) )
		return 0;

	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			FPS++;
			DetectInput(hWnd);	// Direct Input Ű����&���콺 �Է� ����!
			Render();

			if (GetTickCount64() >= Timer_App + 1000)			// �ʽð�
			{
				Timer_App = GetTickCount64();
				FPS_Shown = FPS;
				FPS = 0;
			}
		}
	}

	Cleanup();
	UnregisterClass( WindowName, wc.hInstance );
	return 0;
}

VOID Cleanup()
{
	g_pDI->ShutdownDirectInput();
	g_pDI->DeleteInstance();
	
	SAFE_RELEASE(g_pHLSL);
	SAFE_RELEASE(g_pd3dDevice);
	SAFE_RELEASE(g_pD3D);
}