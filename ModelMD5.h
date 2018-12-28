#include <iostream>
#include <Windows.h>
#include <d3dx9.h>
#include <vector>
#include "Global.h"

#define MAX_PARSE_LINE		512		// ���Ͽ��� �о� ���� �� ���� �ִ� ����
#define MAX_KEYWORD_LEN		64		// �Ľ��� ���� Ű���� �ִ� ����
#define MAX_NAME_LEN		256		// ���� �̸����� �ִ� ����

#define MAX_JOINTS			100		// �� - �ִ� ���� ����	66
#define MAX_OBJECTS			10		// �� - �ִ� ��ü ����	1
#define MAX_MATERIALS		10		// �� - �ִ� ���� ����	1
#define MAX_MESHES			30		// �� - �ִ� �޽� ����	3
#define MAX_VERTICES		8000	// �� - �ִ� ���� ����	8635
#define MAX_INDICES			16000	// �� - �ִ� ���� ����	14238
#define MAX_WEIGHTS			20000	// �� - �ִ� ����ġ ����	18013

typedef char	AnyName[MAX_NAME_LEN];	// �̸� ����
typedef char	KeyWords[MAX_KEYWORD_LEN];	// Ű���� ����

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

// ���� ����ü ���� - ��ġ(Position), �ؽ�ó ��ǥ(Texture Coordinates), ���� ����(Normal) �� //
struct CUSTOMVERTEX
{
	D3DVECTOR pos;
	D3DVECTOR2 texCoord; //�ؽ�ó ��ǥ ��
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