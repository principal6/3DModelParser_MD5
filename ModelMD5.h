#include <iostream>
#include <d3dx9.h>

#define MAX_PARSE_LINE		512		// 파일에서 읽어 들일 한 줄의 최대 길이
#define MAX_KEYWORD_LEN		64		// 파싱을 위한 키워드 최대 길이
#define MAX_NAME_LEN		256		// 각종 이름들의 최대 길이

#define MAX_JOINTS			100		// 모델 - 최대 관절 개수	66
#define MAX_OBJECTS			5		// 모델 - 최대 물체 개수	1
#define MAX_MATERIALS		5		// 모델 - 최대 재질 개수	1
#define MAX_MESHES			10		// 모델 - 최대 메쉬 개수	3
#define MAX_MESH_VERTICES	8000	// 모델 - 메쉬별 최대 정점 개수		8635
#define MAX_MESH_INDICES	16000	// 모델 - 메쉬별 최대 색인 개수		14238
#define MAX_MESH_WEIGHTS	20000	// 모델 - 메쉬별 최대 가중치 개수	18013

#define MAX_ANIMATIONS		10		// 모델 - 애니메이션 최대 개수		1
#define MAX_FRAMES			80		// 모델 - 애니메이션 최대 프레임	32

typedef char	ANYNAME[MAX_NAME_LEN];		// 이름 저장용 변수 유형
typedef char	KEYWORD[MAX_KEYWORD_LEN];	// 키워드 저장용 변수 유형

struct ANY3DVECTOR2
{
	float x;
	float y;
};

struct ANY3DVECTOR3
{
	float x;
	float y;
	float z;
};

struct ANY3DVECTOR4
{
	float x;
	float y;
	float z;
	float w;
};

// 정점 구조체 선언 - 위치(Position), 텍스처 좌표(Texture Coordinates), 법선 벡터(Normal) ★ //
struct ANYVERTEX
{
	ANY3DVECTOR3 pos;
	ANY3DVECTOR2 texCoord; //텍스처 좌표 ★
	//ANY3DVECTOR3 normal;
};

//#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_NORMAL)
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_TEX1)

struct ANYINDEX
{
	WORD _0, _1, _2;
};

// MD5 모델 클래스
class ModelMD5 {
public:
	int		MD5Version;
	int		numMeshes;
	int		numMeshVertices[MAX_MESHES];
	int		numMeshIndices[MAX_MESHES];
	int		numWeights[MAX_MESHES];

	LPDIRECT3DVERTEXBUFFER9 ModelVB;
	LPDIRECT3DINDEXBUFFER9  ModelIB;
	LPDIRECT3DTEXTURE9		ModelTextures[MAX_MATERIALS];
	
	bool ModelMD5::OpenModelFromFile(char* FileName);
	bool ModelMD5::OpenAnimationFromFile(char* FileName);

	void ModelMD5::Animate(float Speed);
	void ModelMD5::CreateModel(LPDIRECT3DDEVICE9 D3DDevice);
	void ModelMD5::DrawModel(LPDIRECT3DDEVICE9 D3DDevice);
	void ModelMD5::Destroy();

	HRESULT ModelMD5::LoadMeshToDraw(LPDIRECT3DDEVICE9 D3DDevice, int MeshIndex);
	HRESULT ModelMD5::UpdateVertices(LPDIRECT3DDEVICE9 D3DDevice, int MeshIndex);

private:
	int	numJoints;
	int numObjects;
	int numMaterials;
};