#include <Windows.h>
#include <mmsystem.h>
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )
#include "Global.h"
#include "ModelMD5.h"

LPDIRECT3D9             g_pD3D			= NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9       g_pd3dDevice	= NULL; // Our rendering device

LPDIRECT3DVERTEXBUFFER9 g_pVB			= NULL; // Buffer to hold vertices
LPDIRECT3DINDEXBUFFER9  g_pIB			= NULL; // Buffer to hold vertices
LPDIRECT3DTEXTURE9		g_pTexture		= NULL;

LPDIRECT3DVERTEXBUFFER9 g_pModelVB		= NULL; // Buffer to hold vertices
LPDIRECT3DINDEXBUFFER9  g_pModelIB		= NULL; // Buffer to hold vertices
LPDIRECT3DTEXTURE9		g_pModelTexture	= NULL;

D3DXMATRIXA16			g_pModelMatrix;
D3DXMATRIXA16			matView, matProj;

ModelMD5				MyMD5Model;

DWORD					MyTimer			= 0;
DWORD					MyTimerCount	= 0;
int						FPS				= 0;

D3DXVECTOR3				GridPoint[2];

// 함수 원형 선언!
VOID Cleanup();
HRESULT InitModel();


HRESULT InitD3D( HWND hWnd )
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
	
    if( FAILED( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice ) ) )
        return E_FAIL;

    //g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
	//g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CW );
	g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
	g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );

	
	InitModel();
	
	MyTimerCount = 100;

    return S_OK;
}

VOID DrawGrid()
{
	ID3DXLine *Line;
	D3DXCreateLine(g_pd3dDevice, &Line);
	Line->SetWidth(0.6f);
	Line->SetAntialias(true);
	Line->Begin();

	D3DXCOLOR AxisXColor = D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f);
	D3DXCOLOR AxisZColor = D3DXCOLOR(1.0f, 0.0f, 1.0f, 1.0f);
	D3DXCOLOR GridColor = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	D3DXCOLOR CurrentColor;

	for (int i = -10; i < 10; i++)
	{
		CurrentColor = GridColor;
		if (i == 0)
			CurrentColor = AxisXColor;

		GridPoint[0].x =-3.0f; GridPoint[0].y = 0.0f; GridPoint[0].z = (float)i;
		GridPoint[1].x =10.0f; GridPoint[1].y = 0.0f; GridPoint[1].z = (float)i;
		Line->DrawTransform(GridPoint, 2, &(g_pModelMatrix*matView*matProj), CurrentColor);
	}

	for (int i = -10; i < 10; i++)
	{
		CurrentColor = GridColor;
		if (i == 0)
			CurrentColor = AxisZColor;

		GridPoint[0].x =(float)i; GridPoint[0].y = 0.0f; GridPoint[0].z = -3.0f;
		GridPoint[1].x =(float)i; GridPoint[1].y = 0.0f; GridPoint[1].z = 10.0f;
		Line->DrawTransform(GridPoint, 2, &(g_pModelMatrix*matView*matProj), CurrentColor);
	}

	Line->End();
	Line->Release();
}

HRESULT InitModel()
{
	MyMD5Model.OpenModelFromFile("dreyar.MD5MESH");
	//MyMD5Model.OpenModelFromFile("SA.MD5MESH");
	//MyMD5Model.SaveParsedFile("SA.MD5MESH");

	return S_OK;
}

VOID SetupMatrices()
{
	D3DXMatrixIdentity( &g_pModelMatrix );

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
	//fAngle = D3DX_PI;							// 90도 = D3DX_PI * 3 / 4
	D3DXMatrixRotationY(&matRot, fAngle);		// Y축을 기준으로 회전 (즉, X&Z가 회전함)
	D3DXMatrixScaling(&matSize, 0.004f, 0.004f, 0.004f);
	//D3DXMatrixScaling(&matSize, 0.04f, 0.04f, 0.04f);
	//D3DXMatrixScaling(&matSize, 1.0f, 1.0f, 1.0f);

	g_pModelMatrix = g_pModelMatrix * matTrans * matRot * matSize;

	g_pd3dDevice->SetTransform(D3DTS_WORLD, &g_pModelMatrix);
}

VOID DrawModels()
{
	// 모델의 메쉬들을 그린다!
	for (int i = 0; i < MyMD5Model.numMeshes; i++)
	{
		MyMD5Model.LoadMeshToDraw(i);

		g_pd3dDevice->SetTexture(0, g_pModelTexture);
		g_pd3dDevice->SetStreamSource(0, g_pModelVB, 0, sizeof(CUSTOMVERTEX));
		g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
		g_pd3dDevice->SetIndices(g_pModelIB);

		g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, MyMD5Model.numMeshVertices[i], 0, MyMD5Model.numMeshIndices[i]);
	}

}

VOID Render()
{
	// 후면 버퍼와 Z 버퍼를 청소한다.
	g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 0, 200, 100 ), 1.0f, 0 );

    if( SUCCEEDED( g_pd3dDevice->BeginScene() ) )
    {
		SetupMatrices();

		DrawGrid();

		//MyXModel.Animate("Rotation");
		//MyXModel.Animate("Walking");
		//MyXModel.Animate("ArmUp");
		
		//DrawModels();

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

		SetupMatrices();

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

				if (timeGetTime() >= MyTimer + 1000)	// 게임 타이머!★★
				{
					MyTimer = timeGetTime();

					char temp[20];
					_itoa(FPS, temp, 10);
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
	if( g_pModelTexture != NULL)
		g_pModelTexture->Release();

	if( g_pModelIB != NULL )
		g_pModelIB->Release();

    if( g_pModelVB != NULL )
        g_pModelVB->Release();

	if( g_pTexture != NULL)
		g_pTexture->Release();

	if( g_pIB != NULL )
		g_pIB->Release();

    if( g_pVB != NULL )
        g_pVB->Release();

    if( g_pd3dDevice != NULL )
        g_pd3dDevice->Release();

    if( g_pD3D != NULL )
        g_pD3D->Release();
}