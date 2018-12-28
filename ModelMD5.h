#include <iostream>
#include <d3dx9.h>
#include <xnamath.h>
#include "Global.h"

#define MAX_PARSE_LINE		512			// 파일에서 읽어 들일 한 줄의 최대 길이
#define MAX_KEYWORD_LEN		64			// 파싱을 위한 키워드의 최대 길이
#define MAX_NAME_LEN		256			// 각종 이름들의 최대 길이

#define MAX_JOINTS			100			// 모델 - 최대 관절 개수			66
#define MAX_OBJECTS			5			// 모델 - 최대 물체 개수
#define MAX_MATERIALS		5			// 모델 - 최대 재질 개수
#define MAX_MESHES			10			// 모델 - 최대 메쉬 개수
#define MAX_MESH_VERTICES	8000		// 모델 - 메쉬별 최대 정점 개수		8635
#define MAX_MESH_INDICES	9000		// 모델 - 메쉬별 최대 색인 개수		14238
#define MAX_MESH_WEIGHTS	10000		// 모델 - 메쉬별 최대 가중치 개수	18013

#define MAX_ANIMATIONS		10			// 모델 - 애니메이션 최대 개수
#define MAX_FRAMES			160			// 모델 - 애니메이션 최대 프레임

#define MAX_INSTANCES		100			// 모델 - 최대 인스턴스 개수

typedef char ANYNAME[MAX_NAME_LEN];		// 이름 저장용 변수 유형
typedef char KEYWORD[MAX_KEYWORD_LEN];	// 키워드 저장용 변수 유형


// 정점 구조체 선언 - 위치(Position), 텍스처 좌표(Texture Coordinates), 법선 벡터(Normal) //
struct ANYVERTEX	// 위치 -> 법선 -> 텍스처 순서를 반드시 지켜야 함!★★★
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

struct MD5Object	// 최신 버전
{
	ANYNAME		Name;
	int			ObjectID;
	int			numMeshes;
};

struct MD5Material	// 최신 버전
{
	ANYNAME		Name;
	ANYNAME		TextureFileName;
};

struct MD5Mesh
{
	ANYNAME		MaterialName;	// 최신 버전이면 Material의 이름이, 구버전이면 Texture 파일명이 들어 있다.

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

// MD5 모델 클래스
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