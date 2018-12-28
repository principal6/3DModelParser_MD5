#include <iostream>
#include <d3dx9.h>

#define MAX_PARSE_LINE		512		// ���Ͽ��� �о� ���� �� ���� �ִ� ����
#define MAX_KEYWORD_LEN		64		// �Ľ��� ���� Ű���� �ִ� ����
#define MAX_NAME_LEN		256		// ���� �̸����� �ִ� ����

#define MAX_JOINTS			100		// �� - �ִ� ���� ����	66
#define MAX_OBJECTS			5		// �� - �ִ� ��ü ����	1
#define MAX_MATERIALS		5		// �� - �ִ� ���� ����	1
#define MAX_MESHES			10		// �� - �ִ� �޽� ����	3
#define MAX_MESH_VERTICES	8000	// �� - �޽��� �ִ� ���� ����		8635
#define MAX_MESH_INDICES	16000	// �� - �޽��� �ִ� ���� ����		14238
#define MAX_MESH_WEIGHTS	20000	// �� - �޽��� �ִ� ����ġ ����	18013

#define MAX_ANIMATIONS		10		// �� - �ִϸ��̼� �ִ� ����		1
#define MAX_FRAMES			80		// �� - �ִϸ��̼� �ִ� ������	32

typedef char	ANYNAME[MAX_NAME_LEN];		// �̸� ����� ���� ����
typedef char	KEYWORD[MAX_KEYWORD_LEN];	// Ű���� ����� ���� ����

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

// ���� ����ü ���� - ��ġ(Position), �ؽ�ó ��ǥ(Texture Coordinates), ���� ����(Normal) �� //
struct ANYVERTEX
{
	ANY3DVECTOR3 pos;
	ANY3DVECTOR2 texCoord; //�ؽ�ó ��ǥ ��
	//ANY3DVECTOR3 normal;
};

//#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_NORMAL)
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_TEX1)

struct ANYINDEX
{
	WORD _0, _1, _2;
};

// MD5 �� Ŭ����
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