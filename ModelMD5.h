#include <iostream>
#include <d3dx9.h>
#include <xnamath.h>
#include "Global.h"

#define MAX_PARSE_LINE		512			// ���Ͽ��� �о� ���� �� ���� �ִ� ����
#define MAX_KEYWORD_LEN		64			// �Ľ��� ���� Ű������ �ִ� ����
#define MAX_NAME_LEN		256			// ���� �̸����� �ִ� ����

#define MAX_JOINTS			100			// �� - �ִ� ���� ����			66
#define MAX_OBJECTS			5			// �� - �ִ� ��ü ����
#define MAX_MATERIALS		5			// �� - �ִ� ���� ����
#define MAX_MESHES			10			// �� - �ִ� �޽� ����
#define MAX_MESH_VERTICES	8000		// �� - �޽��� �ִ� ���� ����		8635
#define MAX_MESH_INDICES	9000		// �� - �޽��� �ִ� ���� ����		14238
#define MAX_MESH_WEIGHTS	10000		// �� - �޽��� �ִ� ����ġ ����	18013

#define MAX_ANIMATIONS		10			// �� - �ִϸ��̼� �ִ� ����
#define MAX_FRAMES			160			// �� - �ִϸ��̼� �ִ� ������

#define MAX_INSTANCES		100			// �� - �ִ� �ν��Ͻ� ����

typedef char ANYNAME[MAX_NAME_LEN];		// �̸� ����� ���� ����
typedef char KEYWORD[MAX_KEYWORD_LEN];	// Ű���� ����� ���� ����


// ���� ����ü ���� - ��ġ(Position), �ؽ�ó ��ǥ(Texture Coordinates), ���� ����(Normal) //
struct ANYVERTEX	// ��ġ -> ���� -> �ؽ�ó ������ �ݵ�� ���Ѿ� ��!�ڡڡ�
{
	XMFLOAT3 pos;
	XMFLOAT3 normal;
	XMFLOAT2 texCoord;
};

#define D3DFVF_ANYVERTEX (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

struct ANYINDEX
{
	WORD _0, _1, _2;
};

struct MD5Joint
{
	ANYNAME		Name;
	int			ParentID;
	XMFLOAT3	Position;
	XMFLOAT4	Orientation;
};

struct MD5Weight
{
	int			JointID;
	float		Bias;
	XMFLOAT3	Position;
	XMFLOAT3	Normal;
};

struct MD5Object	// �ֽ� ����
{
	ANYNAME		Name;
	int			ObjectID;
	int			numMeshes;
};

struct MD5Material	// �ֽ� ����
{
	ANYNAME		Name;
	ANYNAME		TextureFileName;
};

struct MD5Mesh
{
	ANYNAME		MaterialName;	// �ֽ� �����̸� Material�� �̸���, �������̸� Texture ���ϸ��� ��� �ִ�.

	ANYVERTEX	Vertices[MAX_MESH_VERTICES];
	int			VertexWeightStart[MAX_MESH_VERTICES];
	int			VertexNumWeights[MAX_MESH_VERTICES];

	ANYINDEX	Indices[MAX_MESH_INDICES];

	MD5Weight	Weights[MAX_MESH_WEIGHTS];
};

struct MD5BoundingBox
{
	XMFLOAT3	Min;
	XMFLOAT3	Max;
};

struct MD5Frame
{
	int			FrameID;
	float		JointData[MAX_JOINTS*6];
};

struct MD5FrameSkeleton
{
	MD5Joint	Skeleton[MAX_JOINTS];
};

struct MD5AnimationJointInfo
{
	ANYNAME		Name;
	int			ParentID;
	int			Flags;
	int			StartIndex;
};

struct MD5Animation
{
	int		numFrames;
	int		numJoints;
	int		FrameRate;
	int		numAnimatedComponents;

	float	FrameTime;
	float	TotalAnimTime;

	float	BasicAnimSpeed;

	MD5AnimationJointInfo	JointInfo[MAX_JOINTS];
	MD5BoundingBox			BoundingBoxes[MAX_FRAMES];
	MD5Joint				JointBaseFrame[MAX_JOINTS];
	MD5Frame				FrameData[MAX_FRAMES];
	MD5FrameSkeleton		FrameSekelton[MAX_FRAMES];
};

struct MD5Instance
{
	XMFLOAT3	Translation;
	XMFLOAT3	Rotation;
	XMFLOAT3	Scaling;

	int			CurAnimID;
	float		CurAnimTime;
};

// MD5 �� Ŭ����
class ModelMD5 {
public:
	int						MD5Version;

	int						numMeshes;
	int						numMeshVertices[MAX_MESHES];
	int						numMeshIndices[MAX_MESHES];
	int						numWeights[MAX_MESHES];

	int						TotalAnimCount;
	MD5Animation			ModelAnimation[MAX_ANIMATIONS];

	int						numInstances;
	MD5Instance				ModelInstances[MAX_INSTANCES];
	
	char					BaseDir[MAX_NAME_LEN];
	void ModelMD5::SetBaseDirection(char* Dir);

	LPDIRECT3DTEXTURE9		ModelTextures[MAX_MATERIALS];
	bool ModelMD5::CreateModel(LPDIRECT3DDEVICE9 D3DDevice, char* BaseDir, char* FileNameWithoutExtension);
		bool ModelMD5::OpenMeshFromFile(char* FileName);

	bool ModelMD5::CreateAnimation(int AnimationID, char* AnimationFileName, float AnimationSpeed);
		bool ModelMD5::OpenAnimationFromFile(int AnimationID, char* FileName);

	bool ModelMD5::InstanceSetAnimation(int InstanceID, int AnimationID, float StartAnimTime);
	void ModelMD5::InstanceAnimate(int InstanceID, float Speed);
	void ModelMD5::AddInstance(XMFLOAT3 Translation, XMFLOAT3 Rotation, XMFLOAT3 Scaling);
	void ModelMD5::SetInstance(int InstanceID, XMFLOAT3 Translation, XMFLOAT3 Rotation, XMFLOAT3 Scaling);

	void ModelMD5::DrawModel(LPDIRECT3DDEVICE9 D3DDevice);
	void ModelMD5::Destroy();

	HRESULT ModelMD5::SetTexture(LPDIRECT3DDEVICE9 D3DDevice, int MeshIndex);
	HRESULT ModelMD5::UpdateVertices(LPDIRECT3DDEVICE9 D3DDevice, int MeshIndex);

private:
	int	numJoints;
	int numObjects;
	int numMaterials;
};