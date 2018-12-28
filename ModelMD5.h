#include "Global.h"
#include "Parser.h"

#define MAX_JOINTS			100			// �� - �ִ� ���� ����			66
#define MAX_OBJECTS			5			// �� - �ִ� ��ü ����
#define MAX_MATERIALS		5			// �� - �ִ� ���� ����
#define MAX_MESHES			10			// �� - �ִ� �޽� ����
#define MAX_MESH_VERTICES	8000		// �� - �޽��� �ִ� ���� ����		8635
#define MAX_MESH_INDICES	9000		// �� - �޽��� �ִ� ���� ����		14238
#define MAX_MESH_WEIGHTS	10000		// �� - �޽��� �ִ� ����ġ ����	18013

#define MAX_VERTICES		30000		// �� - �ִ� ���� ����
#define MAX_INDICES			40000		// �� - �ִ� ���� ����
#define MAX_WEIGHTS			50000		// �� - �ִ� ����ġ ����

#define MAX_ANIMATIONS		10			// �� - �ִϸ��̼� �ִ� ����
#define MAX_FRAMES			160			// �� - �ִϸ��̼� �ִ� ������

#define MAX_INSTANCES		100			// �� - �ִ� �ν��Ͻ� ����


// ���� ����ü ���� - ��ġ(Position), �ؽ�ó ��ǥ(Texture Coordinates), ���� ����(Normal) //
struct VERTEX_MD5	// ��ġ -> ���� -> �ؽ�ó ������ �ݵ�� ���Ѿ� ��!�ڡڡ�
{
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT2 Texture;
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

struct Object_MD5	// �ֽ� ����
{
	ANYNAME		Name;
	int			ObjectID;
	int			numMeshes;
};

struct Material_MD5	// �ֽ� ����
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
	ANYNAME		MaterialName;	// �ֽ� �����̸� Material�� �̸���, �������̸� Texture ���ϸ��� ��� �ִ�.

	VERTEX_MD5	Vertices[MAX_MESH_VERTICES];
	int			VertexWeightStart[MAX_MESH_VERTICES];
	int			VertexNumWeights[MAX_MESH_VERTICES];

	INDEX_MD5	Indices[MAX_MESH_INDICES];

	Weight_MD5	Weights[MAX_MESH_WEIGHTS];

	BoundingBox_MD5	BB;
	BoundingBox_MD5	BB_Animed;
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
	bool		BeingAnimated;
};

// MD5 �� Ŭ����
class ModelMD5 {
public:
	ModelMD5();
	~ModelMD5();

	int						Version_MD5;

	int						numMeshes;
	int						numMeshVertices[MAX_MESHES];
	int						numMeshIndices[MAX_MESHES];
	int						numWeights[MAX_MESHES];

	int						TotalAnimCount;
	Animation_MD5			ModelAnimation[MAX_ANIMATIONS];

	int						numInstances;
	Instance_MD5			ModelInstances[MAX_INSTANCES];
	bool					MouseOverPerInstances[MAX_INSTANCES];
	float					DistanceCmp[MAX_INSTANCES];
	XMFLOAT3				PickedPosition[MAX_INSTANCES];
	
	char					BaseDir[MAX_NAME_LEN];
	void ModelMD5::SetBaseDirection(char* Dir);

	LPDIRECT3DTEXTURE9		ModelTextures[MAX_MATERIALS];
	bool ModelMD5::CreateModel(LPDIRECT3DDEVICE9 D3DDevice, char* BaseDir, char* FileNameWithoutExtension);
		bool ModelMD5::OpenMeshFromFile(char* FileName);
	void ModelMD5::CreateMeshBoundingBoxes();

	bool ModelMD5::CreateAnimation(int AnimationID, char* AnimationFileName, float AnimationSpeed);
		bool ModelMD5::OpenAnimationFromFile(int AnimationID, char* FileName);

	bool ModelMD5::InstanceSetAnimation(int InstanceID, int AnimationID, float StartAnimTime);
	void ModelMD5::InstanceAnimate(int InstanceID, float Speed);
	void ModelMD5::AddInstance(XMFLOAT3 Translation, XMFLOAT3 Rotation, XMFLOAT3 Scaling);
	void ModelMD5::SetInstance(int InstanceID, XMFLOAT3 Translation, XMFLOAT3 Rotation, XMFLOAT3 Scaling);

	void ModelMD5::DrawModel(LPDIRECT3DDEVICE9 D3DDevice);
	void ModelMD5::DrawBoundingBoxes(LPDIRECT3DDEVICE9 D3DDevice, int InstanceID);
	HRESULT ModelMD5::DrawNormalVecters(LPDIRECT3DDEVICE9 D3DDevice, float LenFactor);
	PickingRay ModelMD5::GetPickingRay(LPDIRECT3DDEVICE9 D3DDevice, int MouseX, int MouseY,
		int ScreenWidth, int ScreenHeight, D3DXMATRIX matView, D3DXMATRIX matProj);
	bool ModelMD5::CheckMouseOverPerInstance(LPDIRECT3DDEVICE9 D3DDevice, int InstanceID, int MouseX, int MouseY,
		int ScreenWidth, int ScreenHeight, D3DXMATRIX matView, D3DXMATRIX matProj);

	HRESULT ModelMD5::SetTexture(LPDIRECT3DDEVICE9 D3DDevice, int MeshIndex);
	HRESULT ModelMD5::UpdateVertices(LPDIRECT3DDEVICE9 D3DDevice, int MeshIndex);

private:
	int	numJoints;
	int numObjects;
	int numMaterials;
};