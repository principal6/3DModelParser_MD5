#include "Global.h"
#include "Parser.h"

#define MAX_JOINTS			100			// 모델 - 최대 관절 개수			34
#define MAX_OBJECTS			5			// 모델 - 최대 물체 개수
#define MAX_MATERIALS		5			// 모델 - 최대 재질 개수
#define MAX_MESHES			10			// 모델 - 최대 메쉬 개수

#define MAX_VERTICES		6000		// 모델 - 최대 정점 개수			2412
#define MAX_INDICES			8000		// 모델 - 최대 색인 개수			3243
#define MAX_WEIGHTS			10000		// 모델 - 최대 가중치 개수			4398

#define MAX_ANIMATIONS		20			// 모델 - 애니메이션 최대 개수
#define MAX_FRAMES			100			// 모델 - 애니메이션 최대 프레임

#define MAX_INSTANCES		1000		// 모델 - 최대 인스턴스 개수


// 정점 구조체 선언 - 위치(Position), 텍스처 좌표(Texture Coordinates), 법선 벡터(Normal) //
struct VERTEX_MD5	// 위치 -> 법선 -> 텍스처 순서를 반드시 지켜야 함!★★★
{
	// 정점 데이터
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT2 Texture;

	// 애니메이션 데이터
	float		WeightStart;
	float		WeightNum;
};

struct INSTANCE_DATA_MD5	// HLSL용 인스턴스 데이터
{
	XMFLOAT4	matModelWorld[4];
	float		bAnimated;
	float		InstanceAnimtationID;
	float		CurFrameTime;
	float		Frame0;
	float		Frame1;
};

struct VERTEX_MD5_BB
{
	XMFLOAT3 Position;
};

struct VERTEX_MD5_NORMAL
{
	XMFLOAT3 Normal;
};

#define D3DFVF_VERTEX_MD5 (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)
#define D3DFVF_VERTEX_MD5_BB (D3DFVF_XYZ)
#define D3DFVF_VERTEX_MD5_NORMAL (D3DFVF_XYZ)

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
	ANYNAME		Name;
	int			ParentID;
	XMFLOAT3	Position;
	XMFLOAT4	Orientation;
};

struct Weight_MD5
{
	int			JointID;
	float		Bias;
	XMFLOAT3	Position;
	XMFLOAT3	Normal;
};

struct Object_MD5	// 최신 버전
{
	ANYNAME		Name;
	int			ObjectID;
	int			numMeshes;
};

struct Material_MD5	// 최신 버전
{
	ANYNAME		Name;
	ANYNAME		TextureFileName;
};

struct BoundingBox_MD5
{
	XMFLOAT3	Min;
	XMFLOAT3	Max;
};

struct Mesh_MD5
{
	ANYNAME		MaterialName;	// 최신 버전이면 Material의 이름이, 구버전이면 Texture 파일명이 들어 있다.
};

struct Frame_MD5
{
	int			FrameID;
	float		JointData[MAX_JOINTS*6];
};

struct FrameSkeleton_MD5
{
	Joint_MD5	Skeleton[MAX_JOINTS];
};

struct AnimationJointInfo_MD5
{
	ANYNAME		Name;
	int			ParentID;
	int			Flags;
	int			StartIndex;
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

	AnimationJointInfo_MD5	JointInfo[MAX_JOINTS];
	BoundingBox_MD5			BoundingBoxes[MAX_FRAMES];
	Joint_MD5				JointBaseFrame[MAX_JOINTS];
	Frame_MD5				FrameData[MAX_FRAMES];
	FrameSkeleton_MD5		FrameSekelton[MAX_FRAMES];
};

struct Instance_MD5
{
	XMFLOAT3	Translation;
	XMFLOAT3	Rotation;
	XMFLOAT3	Scaling;

	int			CurAnimID;
	float		CurAnimTime;
	float		CurFrameTime;
	int			Frame0;
	int			Frame1;

	bool		BeingAnimated;

	Joint_MD5	InterpolatedJoints[MAX_JOINTS];	// 추가!★★
};

// MD5 모델 클래스
class ModelMD5 {
public:
	ModelMD5();
	~ModelMD5();

	int				numInstances;
	Instance_MD5	ModelInstances[MAX_INSTANCES];
	D3DXMATRIXA16	matModelWorld[MAX_INSTANCES];

	bool			MouseOverPerInstances[MAX_INSTANCES];
	float			DistanceCmp[MAX_INSTANCES];
	XMFLOAT3		PickedPosition[MAX_INSTANCES];

	char			BaseDir[MAX_NAME_LEN];
	void			SetBaseDirection(char* Dir);

	HRESULT			CreateModel(LPDIRECT3DDEVICE9 D3DDevice, char* BaseDir, char* FileNameWithoutExtension);
	bool			OpenMeshFromFile(char* FileName);
	HRESULT			SetTexture(LPDIRECT3DDEVICE9 D3DDevice, int MeshIndex);
	void			CreateModelBoundingBox(LPDIRECT3DDEVICE9 D3DDevice);
	void			UpdateModelBoundingBox();

	bool	AddAnimation(int AnimationID, char* AnimationFileName, float AnimationSpeed);
	bool	OpenAnimationFromFile(int AnimationID, char* FileName);

	void	AddInstance(XMFLOAT3 Translation, XMFLOAT3 Rotation, XMFLOAT3 Scaling);
	bool	SetInstanceAnimation(int InstanceID, int AnimationID, float StartAnimTime);
	void	SetInstance(int InstanceID, XMFLOAT3 Translation, XMFLOAT3 Rotation, XMFLOAT3 Scaling);
	void	InstanceAnimate(int InstanceID, float Speed);

	void	CreateInstanceBuffer(LPDIRECT3DDEVICE9 D3DDevice);
	void	UpdateInstanceBuffer(LPDIRECT3DDEVICE9 D3DDevice);

	HRESULT	CreateAnimationJointTexture(LPDIRECT3DDEVICE9 D3DDevice, LPD3DXEFFECT HLSL);
	HRESULT	CreateAnimationWeightTexture(LPDIRECT3DDEVICE9 D3DDevice, LPD3DXEFFECT HLSL);

	void	DrawModel(LPDIRECT3DDEVICE9 D3DDevice);
	void	DrawModel_HLSL(LPDIRECT3DDEVICE9 D3DDevice, LPD3DXEFFECT HLSL);

	void	DrawBoundingBoxes(LPDIRECT3DDEVICE9 D3DDevice);
	void	DrawBoundingBoxes_HLSL(LPDIRECT3DDEVICE9 D3DDevice, LPD3DXEFFECT HLSL);

	HRESULT	DrawNormalVecters(LPDIRECT3DDEVICE9 D3DDevice, float LenFactor);

	PickingRay	GetPickingRay(LPDIRECT3DDEVICE9 D3DDevice, int MouseX, int MouseY,
		int ScreenWidth, int ScreenHeight, D3DXMATRIX matView, D3DXMATRIX matProj);
	bool		CheckMouseOverPerInstance(LPDIRECT3DDEVICE9 D3DDevice, int InstanceID, int MouseX, int MouseY,
		int ScreenWidth, int ScreenHeight, D3DXMATRIX matView, D3DXMATRIX matProj);
	void		CheckMouseOverFinal();

private:
	Joint_MD5		ModelJoints[MAX_JOINTS];
	Object_MD5		ModelObjects[MAX_OBJECTS];
	Material_MD5	ModelMaterials[MAX_MATERIALS];
	Mesh_MD5		ModelMeshes[MAX_MESHES];
	Animation_MD5	ModelAnimation[MAX_ANIMATIONS];

	int				Version_MD5;
	int				numMeshes;
	int				numMeshVertices[MAX_MESHES];
	int				numMeshIndices[MAX_MESHES];
	int				numMeshWeights[MAX_MESHES];

	int				numJoints;
	int				numObjects;
	int				numMaterials;
	int				TotalAnimCount;

	VERTEX_MD5		TVertices[MAX_VERTICES];
	int				TVertexWeightStart[MAX_VERTICES];
	int				TVertexNumWeights[MAX_VERTICES];
	INDEX_MD5		TIndices[MAX_INDICES];
	Weight_MD5		TWeights[MAX_WEIGHTS];

	int				numTVertices;
	int				numTIndices;
	int				numTWeights;

	BoundingBox_MD5	TBB;
	BoundingBox_MD5	TBB_Animed;

	LPDIRECT3DTEXTURE9				ModelTextures[MAX_MATERIALS];
	LPDIRECT3DTEXTURE9				AnimationJointTexture;
	LPDIRECT3DTEXTURE9				AnimationWeightTexture;

	LPDIRECT3DVERTEXDECLARATION9	g_pVBDeclaration;
};