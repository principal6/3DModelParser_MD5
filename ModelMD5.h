#include "Global.h"
#include "Parser.h"

#define MAX_MATERIALS_MD5		5			// 모델 - 최대 재질 개수
#define MAX_JOINTS_MD5			100			// 모델 - 최대 관절 개수
#define MAX_WEIGHTS_MD5			10000		// 모델 - 최대 가중치 개수

#define MAX_ANIMATIONS_MD5		20			// 모델 - 애니메이션 최대 개수
#define MAX_FRAMES_MD5			100			// 모델 - 애니메이션 최대 프레임

#define MAX_INSTANCES_MD5		1000		// 모델 - 최대 인스턴스 개수


// 정점 구조체 선언 - 위치(Position), 텍스처 좌표(Texture Coordinates), 법선 벡터(Normal) //
struct VERTEX_MD5	// 위치 -> 법선 -> 텍스처 순서를 반드시 지켜야 함!★★★
{
	// 정점 데이터
	XMFLOAT3	Position;
	XMFLOAT3	Normal;
	XMFLOAT2	Texture;

	// 애니메이션 데이터
	float	WeightStart;
	float	WeightNum;
};

struct INSTANCE_DATA_MD5	// HLSL용 인스턴스 데이터 (그래서 전부 float형)
{
	XMFLOAT4	matModelWorld[4];
	float	bAnimated;
	float	InstanceAnimtationID;
	float	CurFrameTime;
	float	Frame0;
	float	Frame1;
};

struct VERTEX_MD5_BB
{
	XMFLOAT3		Position;
	D3DCOLOR	Color;
};

struct VERTEX_MD5_NORMAL
{
	XMFLOAT3		Normal;
	D3DCOLOR	Color;
};

#define D3DFVF_VERTEX_MD5 (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)
#define D3DFVF_VERTEX_MD5_BB (D3DFVF_XYZ | D3DFVF_DIFFUSE)
#define D3DFVF_VERTEX_MD5_NORMAL (D3DFVF_XYZ | D3DFVF_DIFFUSE)

struct INDEX_MD5
{
	WORD _0, _1, _2;
};

struct INDEX_MD5_BB
{
	WORD _0, _1;
};

struct INDEX_MD5_NORMAL
{
	WORD _0, _1;
};


struct Joint_MD5
{
	ANYNAME	Name;
	int		ParentID;
	XMFLOAT3	Position;
	XMFLOAT4	Orientation;
};

struct Weight_MD5
{
	int		JointID;
	float	Bias;
	XMFLOAT3	Position;
	XMFLOAT3	Normal;
};

struct Object_MD5	// MD5 최신 버전에만 존재
{
	ANYNAME	Name;
	int		ObjectID;
	int		numMeshes;
};

struct Material_MD5	// MD5 최신 버전에만 존재
{
	ANYNAME	Name;
	ANYNAME	TextureFileName;
};

struct BoundingBox_MD5
{
	XMFLOAT3	Min;
	XMFLOAT3	Max;
};

struct Mesh_MD5
{
	ANYNAME	MaterialName;	// MD5 최신 버전이면 Material의 이름이, 구버전이면 Texture 파일명이 들어 있다.

	int		numMeshVertices;
	int		numMeshIndices;
	int		numMeshWeights;
};

struct Frame_MD5
{
	int		FrameID;
	float	JointData[MAX_JOINTS_MD5*6];
};

struct FrameSkeleton_MD5
{
	Joint_MD5	Skeleton[MAX_JOINTS_MD5];
};

struct AnimationJointInfo_MD5
{
	ANYNAME	Name;
	int		ParentID;
	int		Flags;
	int		StartIndex;
};

struct Animation_MD5
{
	int		numFrames;
	int		numJoints;
	int		FrameRate;
	int		numAnimatedComponents;

	float	FrameTime;
	float	TotalAnimTime;

	float	BasicAnimSpeed;

	AnimationJointInfo_MD5	JointInfo[MAX_JOINTS_MD5];
	BoundingBox_MD5			BoundingBoxes[MAX_FRAMES_MD5];
	Joint_MD5				JointBaseFrame[MAX_JOINTS_MD5];
	Frame_MD5				FrameData[MAX_FRAMES_MD5];
	FrameSkeleton_MD5		FrameSekelton[MAX_FRAMES_MD5];
};

struct Instance_MD5
{
	XMFLOAT3	Translation;
	XMFLOAT3	Rotation;
	XMFLOAT3	Scaling;

	int		CurAnimID;
	float	CurAnimTime;
	float	CurFrameTime;
	int		Frame0;
	int		Frame1;

	bool	BeingAnimated;
};

// MD5 모델 클래스
class ModelMD5 {
public:
	ModelMD5();
	~ModelMD5();

	HRESULT	CreateModel(LPDIRECT3DDEVICE9 D3DDevice, char* BaseDir, char* FileNameWithoutExtension, LPD3DXEFFECT HLSL = NULL);

	void	AddAnimation(int AnimationID, char* AnimationFileName, float AnimationSpeed);
	void	AddAnimationEnd();

	void	AddInstance(XMFLOAT3 Translation, XMFLOAT3 Rotation, XMFLOAT3 Scaling);
	void	AddInstanceEnd();
	void	SetInstance(int InstanceID, XMFLOAT3 Translation, XMFLOAT3 Rotation, XMFLOAT3 Scaling);
	bool	SetInstanceAnimation(int InstanceID, int AnimationID, float StartAnimTime);
	void	AnimateInstance(int InstanceID, float Speed);

	void	DrawNonHLSL(int InstanceID, D3DXMATRIX matView, D3DXMATRIX matProjection);
	void	DrawHLSL(int InstanceID, D3DXMATRIX matView, D3DXMATRIX matProjection);
	void	DrawHLSLInstancing(D3DXMATRIX matView, D3DXMATRIX matProjection);

	void	DrawBoundingBoxes();

	HRESULT	DrawNormalVecters(float LenFactor);

	PickingRay	GetPickingRay(int MouseX, int MouseY, int ScreenWidth, int ScreenHeight, D3DXMATRIX matView, D3DXMATRIX matProj);
	bool		CheckMouseOverPerInstance(int InstanceID, int MouseX, int MouseY, int ScreenWidth, int ScreenHeight, D3DXMATRIX matView, D3DXMATRIX matProj);
	void		CheckMouseOverFinal();

	int				numInstances;
	bool*			MouseOverPerInstances;
	XMFLOAT3*		PickedPosition;

	bool			HLSLInstancing;

private:
	// CreateModel 관련 함수
	void	SetBaseDirection(char* Dir);
	bool	ProofReadMeshFromFile(char* FileName);
	bool	OpenMeshFromFile(char* FileName);
	HRESULT	SetTexture(int MeshIndex);
	void	CreateModelBoundingBox();

	// AddAnimation 관련 함수
	bool	OpenAnimationFromFile(int AnimationID, char* FileName);
	HRESULT	CreateAnimationJointTexture();
	HRESULT	CreateAnimationWeightTexture();

	// AddInstance 관련 함수
	void	CreateInstanceBuffer();

	// DrawHLSL 관련 함수
	void	UpdateModel();
	void	UpdateInstanceBuffer();

	// DrawBoundingBoxes
	void	UpdateModelBoundingBox();

	
	LPDIRECT3DDEVICE9	pDevice;
	LPD3DXEFFECT		pHLSL;

	LPDIRECT3DVERTEXBUFFER9	pModelVB;
	LPDIRECT3DINDEXBUFFER9	pModelIB;
	LPDIRECT3DVERTEXBUFFER9	pModelInstanceVB;

	LPDIRECT3DVERTEXBUFFER9	pBBVB;
	LPDIRECT3DINDEXBUFFER9	pBBIB;

	LPDIRECT3DVERTEXBUFFER9	pNVVB;
	LPDIRECT3DINDEXBUFFER9	pNVIB;


	ANYNAME				BaseDir;

	int				Version_MD5;

	Joint_MD5*		ModelJoints;
	Object_MD5*		ModelObjects;
	Material_MD5*	ModelMaterials;
	Mesh_MD5*		ModelMeshes;
	Animation_MD5*	ModelAnimations;
	Instance_MD5*	ModelInstances;

	int				numJoints;
	int				numObjects;
	int				numMaterials;
	int				numMeshes;
	int				numAnimations;

	VERTEX_MD5*		TVertices;
	INDEX_MD5*		TIndices;
	Weight_MD5*		TWeights;

	int				numTVertices;
	int				numTIndices;
	int				numTWeights;

	D3DXMATRIXA16*	matModelWorld;
	float*			DistanceCmp;

	BoundingBox_MD5	TBB;
	BoundingBox_MD5	TBB_Animed;

	LPDIRECT3DTEXTURE9				ModelTextures[MAX_MATERIALS_MD5];
	LPDIRECT3DTEXTURE9				AnimationJointTexture;
	LPDIRECT3DTEXTURE9				AnimationWeightTexture;

	LPDIRECT3DVERTEXDECLARATION9	g_pVBDeclaration;
};