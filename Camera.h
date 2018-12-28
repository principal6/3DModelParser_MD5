#include "Global.h"

class DirectCamera9
{
public:
	DirectCamera9();
	~DirectCamera9(){};

	int				CameraType;
	float			CamYaw;
	float			CamRoll;

	D3DXVECTOR3		CamPosition;
	D3DXVECTOR3		CamDefaultPosition;

	VOID SetCamera_FirstPerson(float CameraY);
	VOID SetCamera_FreeLook(float CameraX, float CameraY, float CameraZ);
	VOID DirectCamera9::SetCamera_ThirdPerson(float DistanceHigh, float DistanceFar, float TargetY);
	VOID UseCamera_FirstPerson(LPDIRECT3DDEVICE9 D3DDevice, D3DXMATRIXA16* matView);
	VOID UseCamera_FreeLook(LPDIRECT3DDEVICE9 D3DDevice, D3DXMATRIXA16* matView);
	VOID UseCamera_ThirdPerson(LPDIRECT3DDEVICE9 D3DDevice, D3DXMATRIXA16* matView, D3DXVECTOR3 SpritePosition);

	VOID ZoomCamera(bool ZoomIn, float MoveDistance);

	VOID MoveCamera_BackForth(bool MoveBack, float MoveDistance);
	VOID MoveCamera_LeftRight(bool MoveLeft, float MoveDistance);

	VOID RotateCamera_LeftRight(float MouseMovedX, float SpeedFactor);
	VOID RotateCamera_LeftRightRegular(bool RotateLeft, float RotateAngle);
	VOID RotateCamera_UpDown(float MouseMovedY, float SpeedFactor);
	VOID RotateCamera_UpDownRegular(bool RotateUp, float RotateAngle);

private:
};