#include <windows.h>
#include "ModelMD5.h"


// ��� ����
const int				MAX_MODEL_NUM	= 10;


// D3D ���� ����
LPDIRECT3D9             g_pD3D			= NULL;
LPDIRECT3DDEVICE9       g_pd3dDevice	= NULL;

LPDIRECT3DVERTEXBUFFER9	g_pModelVB		= NULL;
LPDIRECT3DINDEXBUFFER9	g_pModelIB		= NULL;

D3DLIGHT9				Light_Directional;

D3DXMATRIXA16			matModelWorld;
D3DXMATRIXA16			matView, matProj;


// ��Ÿ ���� ����
ModelMD5				MyMD5Model[MAX_MODEL_NUM];
DWORD					SecondTimer		= 0;
int						FPS				= 0;
const int				ScreenWidth		= 1024;
const int				ScreenHeight	= 768;


// �Լ� ���� ����
VOID Cleanup();
HRESULT InitD3D( HWND hWnd );
HRESULT InitModel();
VOID SetupCameraMatrices();
VOID SetupModelMatrix(float Tx, float Ty, float Tz, float Rx, float Ry, float Rz, float Sx, float Sy, float Sz);
VOID SetupLights();
VOID Render();
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT );


HRESULT InitD3D( HWND hWnd )
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
	//d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;	// �̰� ������ 60FPS�� �Ѿ �׷�����

	if( FAILED( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice ) ) )
		return E_FAIL;

	g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
	//g_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );

	InitModel();

	return S_OK;
}

HRESULT InitModel()
{
	MyMD5Model[0].CreateModel(g_pd3dDevice, "Model\\", "Ezreal");
	MyMD5Model[0].CreateAnimation(0, "EzrealWalk", 0.02f);
	MyMD5Model[0].CreateAnimation(1, "EzrealPunching", 0.02f);
	MyMD5Model[0].CreateAnimation(2, "EzrealStanding1HMagicAttack01", 0.02f);

	MyMD5Model[0].AddInstance( XMFLOAT3(0.0f, -40.f, 0.0f), XMFLOAT3(0.0f, 0.f, 0.0f), XMFLOAT3(0.04f, 0.04f, 0.04f) );
	MyMD5Model[0].InstanceSetAnimation(0, 0, 0.0f);

	MyMD5Model[0].AddInstance( XMFLOAT3(40.0f, -40.f, 0.0f), XMFLOAT3(0.0f, 0.f, 0.0f), XMFLOAT3(0.04f, 0.04f, 0.04f) );
	MyMD5Model[0].InstanceSetAnimation(1, 1, 0.0f);

	MyMD5Model[0].AddInstance( XMFLOAT3(80.0f, -40.f, 0.0f), XMFLOAT3(0.0f, 0.f, 0.0f), XMFLOAT3(0.04f, 0.04f, 0.04f) );
	MyMD5Model[0].InstanceSetAnimation(2, 2, 0.0f);

	return S_OK;
}

VOID SetupCameraMatrices()
{
	// �� ���(ī�޶� ����)
	D3DXVECTOR3 vEyePt( -5.0f, 4.0f, -5.0f );
	D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
	D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
	D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
	g_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );
	
	// ���� ���(���ٰ� ����)
	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, 1.0f, 1.0f, 100.0f );
	g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );
}

VOID SetupModelMatrix(XMFLOAT3 Translation, XMFLOAT3 Rotation, XMFLOAT3 Scaling)
{
	// ���� ���(��ġ, ȸ��, ũ�� ����)
	D3DXMatrixIdentity( &matModelWorld );

		D3DXMATRIXA16 matTrans;
		D3DXMATRIXA16 matRotX;
		D3DXMATRIXA16 matRotY;
		D3DXMATRIXA16 matRotZ;
		D3DXMATRIXA16 matSize;

		D3DXMatrixTranslation(&matTrans, Translation.x, Translation.y, Translation.z);
		D3DXMatrixRotationX(&matRotX, Rotation.x);
		D3DXMatrixRotationY(&matRotY, Rotation.y);				// Y���� �������� ȸ�� (��, X&Z�� ȸ����)
		D3DXMatrixRotationZ(&matRotZ, Rotation.z);
		D3DXMatrixScaling(&matSize, Scaling.x, Scaling.y, Scaling.z);

	matModelWorld = matModelWorld * matTrans * matRotX * matRotY * matRotZ * matSize;
	g_pd3dDevice->SetTransform(D3DTS_WORLD, &matModelWorld);
}

VOID SetupLights()
{
	// �Ͼ���� ���⼺ ����(Directional Light)�� �����Ѵ�.
	D3DXVECTOR3 vecDir;
	ZeroMemory( &Light_Directional, sizeof( D3DLIGHT9 ) );
	Light_Directional.Type = D3DLIGHT_DIRECTIONAL;
	Light_Directional.Diffuse.r = 1.0f;
	Light_Directional.Diffuse.g = 1.0f;
	Light_Directional.Diffuse.b = 1.0f;
	Light_Directional.Range = 1000.0f;
	vecDir = D3DXVECTOR3( cosf( timeGetTime() / 350.0f ), 1.0f, sinf( timeGetTime() / 350.0f ) );
	D3DXVec3Normalize( ( D3DXVECTOR3* )&Light_Directional.Direction, &vecDir );

	g_pd3dDevice->SetLight( 0, &Light_Directional );
	g_pd3dDevice->LightEnable( 0, TRUE );

	// ȯ�汤�� ����Ѵ�.
	g_pd3dDevice->SetRenderState( D3DRS_AMBIENT, 0x00202020 );

	// ���� ����� �Ҵ�
	g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );
}

VOID Render()
{
	// �ĸ� ���ۿ� Z ���۸� û���Ѵ�.
	g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 0, 200, 100 ), 1.0f, 0 );

	if( SUCCEEDED( g_pd3dDevice->BeginScene() ) )
	{
		SetupCameraMatrices();
		SetupLights();

		for (int i = 0; i < MyMD5Model[0].numInstances; i++)
		{
			SetupModelMatrix(MyMD5Model[0].ModelInstances[i].Translation,
				MyMD5Model[0].ModelInstances[i].Rotation,
				MyMD5Model[0].ModelInstances[i].Scaling);
			MyMD5Model[0].InstanceAnimate(i, 0.0f);
			MyMD5Model[0].DrawModel(g_pd3dDevice);
		}
		
		g_pd3dDevice->EndScene();
	}

	g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
}


LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_DESTROY:
			Cleanup();
			PostQuitMessage( 0 );
			return 0;
	}

	return DefWindowProc( hWnd, msg, wParam, lParam );
}


INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "D3DGAME", NULL };
	RegisterClassEx( &wc );

	HWND hWnd = CreateWindow( "D3DGAME", "GAME", WS_OVERLAPPEDWINDOW, 0, 0, ScreenWidth, ScreenHeight, NULL, NULL, wc.hInstance, NULL );

	if( FAILED( InitD3D( hWnd ) ) )
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
			Render();

			if (GetTickCount() >= SecondTimer + 1000)	// �ʽð�
			{
				SecondTimer = GetTickCount();

				char temp[20];
				_itoa_s(FPS, temp, 10);
				SetWindowText(hWnd, temp);

				FPS = 0;
			}
		}
	}

	UnregisterClass( "D3DGAME", wc.hInstance );
	return 0;
}

VOID Cleanup()
{
	for (int i = 0; i < MAX_MODEL_NUM; i++)
	{
		MyMD5Model[i].Destroy();
	}

	if( g_pModelVB != NULL )
		g_pModelVB->Release();

	if( g_pModelIB != NULL )
		g_pModelIB->Release();
		
	if( g_pd3dDevice != NULL )
		g_pd3dDevice->Release();

	if( g_pD3D != NULL )
		g_pD3D->Release();
}