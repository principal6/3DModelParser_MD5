#include <windows.h>
#include "ModelMD5.h"
#include "DirectInput.h"
#include "Camera.h"
#include "Font.h"
#include <crtdbg.h>

#ifdef _DEBUG	// 메모리 누수 검사
#define new new( _CLIENT_BLOCK, __FILE, __LINE__)
#endif


// 상수 선언
const int				ScreenWidth			= 800;
const int				ScreenHeight		= 600;
const char				WindowName[]		= "D3DGAME";
const char				WindowTitle[]		= "GAME";
const int				MAX_MODEL_NUM		= 10;


// D3D 변수 선언
LPDIRECT3D9             g_pD3D				= NULL;
LPDIRECT3DDEVICE9       g_pd3dDevice		= NULL;
LPD3DXEFFECT			g_pHLSL				= NULL;	// HLSL Shader

LPDIRECT3DVERTEXBUFFER9	g_pModelVB			= NULL;
LPDIRECT3DINDEXBUFFER9	g_pModelIB			= NULL;
LPDIRECT3DVERTEXBUFFER9	g_pModelInstanceVB	= NULL;

LPDIRECT3DVERTEXBUFFER9	g_pBBVB				= NULL;
LPDIRECT3DINDEXBUFFER9	g_pBBIB				= NULL;

LPDIRECT3DVERTEXBUFFER9	g_pNVVB				= NULL;
LPDIRECT3DINDEXBUFFER9	g_pNVIB				= NULL;

DirectCamera9			g_Camera;					// Direct Camera
DirectFont9				g_Font;						// Direct Font

DirectInput*			g_pDI				= NULL;	// Direct Input
D3DXVECTOR3				MouseState;					// Direct Input - 마우스 이동, 휠
POINT					MouseScreen;				// Direct Input - 마우스 좌표
int						MouseButtonState	= 0;	// Direct Input - 마우스 버튼 눌림

D3DXMATRIXA16			matView, matProj;
D3DXMATRIXA16			matVP;


// 기타 변수 선언
DWORD					Timer_App			= 0;
DWORD					Timer_Animation		= 0;
int						FPS					= 0;
int						FPS_Shown			= 0;

ModelMD5				MyMD5Model[MAX_MODEL_NUM];
bool					bDrawBoundingBoxes	= false;
bool					bDrawNormalVectors	= false;
bool					bWireFrame			= false;


// 함수 원형 선언
VOID Cleanup();
HRESULT InitD3D( HWND hWnd, HINSTANCE hInst );
HRESULT InitModel();
VOID SetupCameraMatrices();
VOID SetupLights();
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
	d3dpp.EnableAutoDepthStencil = TRUE;		// 자동 깊이 공판(Depth Stencil)을 사용한다. (그래야 컴퓨터가 알아서 멀리 있는 걸 먼저 그려줌★)
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;	// 깊이 공판 형식: D3DFMT_D16
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;	// 후면 버퍼 형식: D3DFMT_UNKNOWN는 현재 바탕화면 포맷과 일치
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	if( FAILED( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice ) ) )
		return E_FAIL;

	g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );

	// 셰이더 컴파일
	D3DXCreateEffectFromFile(g_pd3dDevice, "HLSLMain.fx", NULL, NULL, NULL, NULL, &g_pHLSL, NULL);
	if (!g_pHLSL)
		return E_FAIL;

	InitModel();

	g_pDI = DirectInput::GetInstance();		// Direct Input 초기화
	g_pDI->InitDirectInput(hInst, hWnd);
	g_pDI->CreateKeyboardDevice(DISCL_BACKGROUND|DISCL_NONEXCLUSIVE);
	g_pDI->CreateMouseDevice(DISCL_BACKGROUND|DISCL_NONEXCLUSIVE);

	g_Camera.SetCamera_FreeLook(-5.0f, 4.0f, -5.0f);
	
	g_Font.CreateFontA(g_pd3dDevice, "Arial", 20, false, ScreenWidth, ScreenHeight);

	return S_OK;
}

HRESULT InitModel()
{
	// 모델 생성
	MyMD5Model[0].CreateModel(g_pd3dDevice, "Model\\", "Ezreal");
		MyMD5Model[0].AddAnimation(0, "EzrealIdle", 0.02f);
		MyMD5Model[0].AddAnimation(1, "EzrealWalk", 0.02f);
		MyMD5Model[0].AddAnimation(2, "EzrealPunching", 0.02f);
		MyMD5Model[0].AddAnimation(3, "EzrealStanding1HMagicAttack01", 0.02f);
	MyMD5Model[0].CreateAnimationJointTexture(g_pd3dDevice, g_pHLSL);
	MyMD5Model[0].CreateAnimationWeightTexture(g_pd3dDevice, g_pHLSL);

	// 모델 인스턴스 설정
	for (int i = 0; i < 30; i++)
	{
		MyMD5Model[0].AddInstance( XMFLOAT3((float)((int)(i % 10) * 6), -4.0f, (float)(int)(i / 10) * 8),
			XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.1f, 0.1f, 0.1f) );
		MyMD5Model[0].SetInstanceAnimation(i, i % 4, 0.0f);	// 애니메이션 시작 시간 0으로 초기화
	}
	MyMD5Model[0].CreateInstanceBuffer(g_pd3dDevice);

	return S_OK;
}

VOID SetupCameraMatrices()
{
	// 뷰 행렬(카메라 설정)
	g_Camera.UseCamera_FreeLook( g_pd3dDevice, &matView );
	
	// 투영 행렬(원근감 설정)
	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, 1.0f, 1.0f, 10000.0f );
	g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );
}

void DetectInput(HWND hWnd)
{
	if(GetFocus())	// Direct Input에 입력이 들어오면
	{
		MouseState = g_pDI->DIMouseHandler();

		GetCursorPos(&MouseScreen);			// 윈도우 상 마우스 좌표 얻어오기
		ScreenToClient(hWnd, &MouseScreen);

		MouseButtonState = 0;

		if (g_pDI->DIMouseButtonHandler(0))	// 마우스 왼쪽 버튼
		{
			MouseButtonState = 1;
		}

		if (g_pDI->DIMouseButtonHandler(1))	// 마우스 오른쪽 버튼
		{
			MouseButtonState = 2;
		}

		if (MouseState.x < 0)	// 마우스 왼쪽으로 이동
		{
			g_Camera.RotateCamera_LeftRight(MouseState.x, 250.0f);
		}

		if (MouseState.x > 0)	// 마우스 오른쪽으로 이동
		{
			g_Camera.RotateCamera_LeftRight(MouseState.x, 250.0f);
		}

		if (MouseState.y < 0)	// 마우스 위로 이동
		{
			g_Camera.RotateCamera_UpDown(MouseState.y, 250.0f);
		}

		if (MouseState.y > 0)	// 마우스 아래로 이동
		{
			g_Camera.RotateCamera_UpDown(MouseState.y, 250.0f);
		}

		if(g_pDI->DIKeyboardHandler(DIK_W))	// W: 앞으로 이동
		{
			g_Camera.MoveCamera_BackForth(false, 1.0f);
		}

		if(g_pDI->DIKeyboardHandler(DIK_S))	// S: 뒤로 이동
		{
			g_Camera.MoveCamera_BackForth(true, 1.0f);
		}

		if(g_pDI->DIKeyboardHandler(DIK_A))	// A: 왼쪽으로 이동
		{
			g_Camera.MoveCamera_LeftRight(true, 1.0f);
		}
		
		if(g_pDI->DIKeyboardHandler(DIK_D))	// D: 오른쪽으로 이동
		{
			g_Camera.MoveCamera_LeftRight(false, 1.0f);
		}

		if(g_pDI->DIKeyboardHandler(DIK_B))	// B: 바운딩 박스 그리기
		{
			bDrawBoundingBoxes = !bDrawBoundingBoxes;
			Sleep(140);
		}

		if(g_pDI->DIKeyboardHandler(DIK_N))	// N: 법선 벡터 그리기
		{
			bDrawNormalVectors = !bDrawNormalVectors;
			Sleep(140);
		}

		if(g_pDI->DIKeyboardHandler(DIK_R))	// R: 그리기 모드
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

		if(g_pDI->DIKeyboardHandler(DIK_SPACE))	// Spacebar: 카메라 위치, 방향 초기화
		{
			g_Camera.SetCamera_FreeLook(0.0f, 0.0f, 0.0f);
			g_Camera.CamYaw = 0;
			g_Camera.CamRoll = 0;
			Sleep(140);
		}

		if(g_pDI->DIKeyboardHandler(DIK_ESCAPE))	// ESC: 종료
			DestroyWindow(hWnd);
	}
}

VOID Render()
{
	// 후면 버퍼와 Z 버퍼를 청소한다.
	g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 0, 200, 100 ), 1.0f, 0 );

	if( SUCCEEDED( g_pd3dDevice->BeginScene() ) )
	{
		SetupCameraMatrices();

		matVP	= matView * matProj;
		g_pHLSL->SetMatrix("matVP", &matVP);

		// 모델 인스턴스 정보 갱신
		for (int i = 0; i < MyMD5Model[0].numInstances; i++)
		{
			MyMD5Model[0].InstanceAnimate(i, 0.0f);
			MyMD5Model[0].UpdateModelBoundingBox();
		}
		MyMD5Model[0].UpdateInstanceBuffer(g_pd3dDevice);

		// HLSL 그리기
		MyMD5Model[0].DrawModel_HLSL(g_pd3dDevice, g_pHLSL);

		if (bDrawBoundingBoxes == true)
			MyMD5Model[0].DrawBoundingBoxes_HLSL(g_pd3dDevice, g_pHLSL);

		if (g_pDI->OnMouseButtonDown(0))
		{
			for (int i = 0; i < MyMD5Model[0].numInstances; i++)
			{
				MyMD5Model[0].CheckMouseOverPerInstance(g_pd3dDevice, i, MouseScreen.x, MouseScreen.y, ScreenWidth, ScreenHeight, matView, matProj);
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
	#ifdef _DEBUG	// 메모리 누수 검사
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
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
			DetectInput(hWnd);	// Direct Input 키보드&마우스 입력 감지!
			Render();

			if (GetTickCount() >= Timer_App + 1000)			// 초시계
			{
				Timer_App = GetTickCount();
				FPS_Shown = FPS;
				FPS = 0;
			}

			if (GetTickCount() >= Timer_Animation + 80)
			{
				Timer_Animation = GetTickCount();

				/*
				for (int i = 0; i < MyMD5Model[0].numInstances; i++)
				{
					MyMD5Model[0].InstanceAnimate(i, 0.0f);
				}
				*/

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

	SAFE_RELEASE(g_pModelVB);
	SAFE_RELEASE(g_pModelIB);
	SAFE_RELEASE(g_pModelInstanceVB);

	SAFE_RELEASE(g_pBBVB);
	SAFE_RELEASE(g_pBBIB);

	SAFE_RELEASE(g_pNVVB);
	SAFE_RELEASE(g_pNVIB);
	
	SAFE_RELEASE(g_pHLSL);
	SAFE_RELEASE(g_pd3dDevice);
	SAFE_RELEASE(g_pD3D);
}