#include <windows.h>
#include <mmsystem.h>
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )
#include "ModelMD5.h"

// D3D ���� ����
LPDIRECT3D9             g_pD3D			= NULL;
LPDIRECT3DDEVICE9       g_pd3dDevice	= NULL;

D3DXMATRIXA16			matWorld;
D3DXMATRIXA16			matView, matProj;

// ��Ÿ ���� ����
ModelMD5				MyMD5Model;
DWORD					MyTimer			= 0;
int						FPS				= 0;

// �Լ� ���� ����
VOID Cleanup();
HRESULT InitD3D( HWND hWnd );
HRESULT InitModel();


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

	if( FAILED( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice ) ) )
		return E_FAIL;

	//g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
	//g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CW );
	g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
	g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );

	InitModel();

	return S_OK;
}

HRESULT InitModel()
{
	MyMD5Model.OpenModelFromFile("Female.MD5MESH");
	MyMD5Model.OpenAnimationFromFile("Female.MD5ANIM");
	MyMD5Model.CreateModel(g_pd3dDevice);

	return S_OK;
}

VOID SetupMatrices()
{
	D3DXMatrixIdentity( &matWorld );

	D3DXVECTOR3 vEyePt( -5.0f, 4.0f, -5.0f );
	D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
	D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );

	D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
	g_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );

	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, 1.0f, 1.0f, 100.0f );
	g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );


	D3DXMATRIXA16 matTrans;
	D3DXMATRIXA16 matRot;
	D3DXMATRIXA16 matSize;

	D3DXMatrixTranslation(&matTrans, 0.0f, 0.0f, 0.0f);
	//D3DXMatrixTranslation(&matTrans, 0.0f, -500.0f, 0.0f);
	FLOAT fAngle = 0.0f;
	//fAngle = timeGetTime() / 1000.0f;
	//fAngle = D3DX_PI;							// 90�� = D3DX_PI * 3 / 4
	D3DXMatrixRotationY(&matRot, fAngle);		// Y���� �������� ȸ�� (��, X&Z�� ȸ����)
	//D3DXMatrixScaling(&matSize, 0.004f, 0.004f, 0.004f);
	D3DXMatrixScaling(&matSize, 0.08f, 0.08f, 0.08f);
	//D3DXMatrixScaling(&matSize, 1.0f, 1.0f, 1.0f);

	matWorld = matWorld * matTrans * matRot * matSize;

	g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
}

VOID Render()
{
	// �ĸ� ���ۿ� Z ���۸� û���Ѵ�.
	g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 0, 200, 100 ), 1.0f, 0 );

	if( SUCCEEDED( g_pd3dDevice->BeginScene() ) )
	{
		SetupMatrices();

		MyMD5Model.Animate(0.02f);
		MyMD5Model.DrawModel(g_pd3dDevice);

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

	HWND hWnd = CreateWindow( "D3DGAME", "GAME", WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, NULL, NULL, wc.hInstance, NULL );

	if( SUCCEEDED( InitD3D( hWnd ) ) )
	{
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

				if (timeGetTime() >= MyTimer + 1000)	// ���� Ÿ�̸�!�ڡ�
				{
					MyTimer = timeGetTime();

					char temp[20];
					_itoa_s(FPS, temp, 10);
					SetWindowText(hWnd, temp);

					FPS = 0;
				}
			}
		}
	}

	UnregisterClass( "D3DGAME", wc.hInstance );
	return 0;
}

VOID Cleanup()
{
	MyMD5Model.Destroy();

	if( g_pd3dDevice != NULL )
		g_pd3dDevice->Release();

	if( g_pD3D != NULL )
		g_pD3D->Release();
}