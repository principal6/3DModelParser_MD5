#include <iostream>
#include <Windows.h>
#include <d3dx9.h>
#include <vector>
#include "Global.h"

#define MAX_PARSE_LINE		512		// 파일에서 읽어 들일 한 줄의 최대 길이
#define MAX_KEYWORD_LEN		64		// 파싱을 위한 키워드 최대 길이
#define MAX_NAME_LEN		256		// 각종 이름들의 최대 길이

#define MAX_JOINTS			100		// 모델 - 최대 관절 개수	66
#define MAX_OBJECTS			10		// 모델 - 최대 물체 개수	1
#define MAX_MATERIALS		10		// 모델 - 최대 재질 개수	1
#define MAX_MESHES			30		// 모델 - 최대 메쉬 개수	3
#define MAX_VERTICES		8000	// 모델 - 최대 정점 개수	8635
#define MAX_INDICES			16000	// 모델 - 최대 색인 개수	14238
#define MAX_WEIGHTS			20000	// 모델 - 최대 가중치 개수	18013

typedef char	AnyName[MAX_NAME_LEN];	// 이름 저장
typedef char	KeyWords[MAX_KEYWORD_LEN];	// 키워드 저장

struct D3DVECTOR2
{
	float x;
	float y;
};

struct D3DVECTOR4
{
	float x;
	float y;
	float z;
	float w;
};

// 정점 구조체 선언 - 위치(Position), 텍스처 좌표(Texture Coordinates), 법선 벡터(Normal) ★ //
struct CUSTOMVERTEX
{
	D3DVECTOR pos;
	D3DVECTOR2 texCoord; //텍스처 좌표 ★
	//D3DVECTOR normal;
};

//#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_NORMAL)
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_TEX1)

struct CUSTOMINDEX
{
	WORD _0, _1, _2;
};

class ModelMD5 {
public:
	int	MD5Version;
	int	numMeshes;
	int	numMeshVertices[MAX_MESHES];
	int	numMeshIndices[MAX_MESHES];
	int	numWeights[MAX_MESHES];
	
	bool ModelMD5::OpenModelFromFile(char* FileName);
	bool ModelMD5::SaveParsedFile(char* FileName);
	HRESULT ModelMD5::LoadMeshToDraw(int MeshIndex);
	HRESULT ModelMD5::Animate(char* AnimationName);

private:
	int	numJoints;
	int numObjects;
	int numMaterials;
};