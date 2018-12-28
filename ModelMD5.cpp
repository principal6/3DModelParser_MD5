#include "ModelMD5.h"
#include "Parser.h"

KEYWORD	KeyWords_MD5[] =
{
	// MD5MESH
	"MD5Version"	,	// 0
	"commandline"	,	// 1
	"numJoints"		,	// 2
	"numMeshes"		,	// 3
	"numObjects"	,	// 4
	"numMaterials"	,	// 5
	"joints"		,	// 6
	"objects"		,	// 7
	"materials"		,	// 8
	"mesh "			,	// 9 (띄어쓰기가 없으면 meshindex와 혼동됨)★
	"//"			,	// 10
	"meshindex"		,	// 11
	"shader"		,	// 12
	"numverts"		,	// 13
	"vert"			,	// 14
	"numtris"		,	// 15
	"tri"			,	// 16
	"numweights"	,	// 17
	"weight"		,	// 18

	// MD5ANIM
	"numFrames"				,	// 19
	"frameRate"				,	// 20
	"numAnimatedComponents"	,	// 21
	"hierarchy"				,	// 22
	"bounds"				,	// 23
	"baseframe"				,	// 24
	"frame "				,	// 25 (띄어쓰기가 없으면 frameRate와 혼동됨)★
};

// 기타 변수
char		TabChar[] = {'\t'};


D3DVERTEXELEMENT9 g_VBElements[] =
{
	// 정점 정보
	{ 0, 4 * 0,		D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0 },
	{ 0, 4 * 3,		D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_NORMAL,		0 },
	{ 0, 4 * 6,		D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,		0 },
    { 0, 4 * 8,		D3DDECLTYPE_FLOAT1,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_PSIZE,			0 },
	{ 0, 4 * 9,		D3DDECLTYPE_FLOAT1,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_PSIZE,			1 },

	// 인스턴스 정보
	{ 1, 4 * 0,		D3DDECLTYPE_FLOAT4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		1 },	// XMFLOAT4	matModelWorld[4];
	{ 1, 4 * 4,		D3DDECLTYPE_FLOAT4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		2 },
	{ 1, 4 * 8,		D3DDECLTYPE_FLOAT4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		3 },
	{ 1, 4 * 12,	D3DDECLTYPE_FLOAT4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		4 },
	{ 1, 4 * 16,	D3DDECLTYPE_FLOAT1,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_PSIZE,			2 },	// float		bAnimated;
	{ 1, 4 * 17,	D3DDECLTYPE_FLOAT1,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_PSIZE,			3 },	// float		InstanceAnimtationID;
	{ 1, 4 * 18,	D3DDECLTYPE_FLOAT1,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_PSIZE,			4 },	// float		CurFrameTime;
	{ 1, 4 * 19,	D3DDECLTYPE_FLOAT1,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_PSIZE,			5 },	// float		Frame0;
	{ 1, 4 * 20,	D3DDECLTYPE_FLOAT1,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_PSIZE,			6 },	// float		Frame1;

	D3DDECL_END()
};

ModelMD5::ModelMD5()
{
	// public 멤버 변수 초기화
	numInstances	= 0;
	HLSLInstancing	= false;

	// private 멤버 변수 초기화
	pDevice			= NULL;
	pHLSL			= NULL;
	memset(BaseDir, 0, sizeof(BaseDir));

	Version_MD5		= 0;

	numJoints		= 0;
	numObjects		= 0;
	numMaterials	= 0;
	numMeshes		= 0;
	numAnimations	= 0;

	numTVertices	= 0;
	numTIndices		= 0;
	numTWeights		= 0;
}

ModelMD5::~ModelMD5()
{
	SAFE_DELETEARRAY(ModelJoints);
	SAFE_DELETEARRAY(ModelObjects);
	SAFE_DELETEARRAY(ModelMaterials);
	SAFE_DELETEARRAY(ModelMeshes);
	SAFE_DELETEARRAY(ModelAnimations);
	SAFE_DELETEARRAY(ModelInstances);

	SAFE_DELETEARRAY(TVertices);
	SAFE_DELETEARRAY(TIndices);
	SAFE_DELETEARRAY(TWeights);

	SAFE_DELETEARRAY(matModelWorld);
	SAFE_DELETEARRAY(MouseOverPerInstances);
	SAFE_DELETEARRAY(DistanceCmp);
	SAFE_DELETEARRAY(PickedPosition);

	for (int i = 0; i < MAX_MATERIALS_MD5; i++)
	{
		SAFE_RELEASE(ModelTextures[i]);
	}
	SAFE_RELEASE(AnimationJointTexture);
	SAFE_RELEASE(AnimationWeightTexture);
	
	SAFE_RELEASE(g_pVBDeclaration);

	pDevice = NULL;
	pHLSL = NULL;

	SAFE_RELEASE(pModelVB);
	SAFE_RELEASE(pModelIB);
	SAFE_RELEASE(pModelInstanceVB);

	SAFE_RELEASE(pBBVB);
	SAFE_RELEASE(pBBIB);

	SAFE_RELEASE(pNVVB);
	SAFE_RELEASE(pNVIB);
}


HRESULT ModelMD5::CreateModel(LPDIRECT3DDEVICE9 D3DDevice, char* BaseDir, char* FileNameWithoutExtension, LPD3DXEFFECT HLSL)
{
	// 포인터 설정 ★★
	pDevice	= D3DDevice;
	pHLSL	= HLSL;

	// 변수 동적 할당 #1
	ModelInstances = new Instance_MD5[MAX_INSTANCES_MD5];
	matModelWorld = new D3DXMATRIXA16[MAX_INSTANCES_MD5];
	MouseOverPerInstances = new bool[MAX_INSTANCES_MD5];
	DistanceCmp = new float[MAX_INSTANCES_MD5];
	PickedPosition = new XMFLOAT3[MAX_INSTANCES_MD5];

	memset(ModelInstances, 0, sizeof(Instance_MD5)*MAX_INSTANCES_MD5);
	memset(matModelWorld, 0, sizeof(D3DXMATRIXA16)*MAX_INSTANCES_MD5);
	memset(MouseOverPerInstances, 0, sizeof(bool)*MAX_INSTANCES_MD5);
	memset(DistanceCmp, 0, sizeof(float)*MAX_INSTANCES_MD5);
	memset(PickedPosition, 0, sizeof(XMFLOAT3)*MAX_INSTANCES_MD5);


	// 모델 파일이 있는 폴더 설정
	SetBaseDirection(BaseDir);

	// 모델 메쉬 불러오기
	char	NewFileName[MAX_NAME_LEN] = {0};
	strcpy_s(NewFileName, FileNameWithoutExtension);
	strcat_s(NewFileName, ".MD5MESH");
	
	ProofReadMeshFromFile(NewFileName);


	// 변수 동적 할당 #2
	ModelJoints = new Joint_MD5[numJoints];
	ModelObjects = new Object_MD5[numObjects];
	ModelMaterials = new Material_MD5[numMaterials];
	ModelMeshes = new Mesh_MD5[numMeshes];
	ModelAnimations = new Animation_MD5[MAX_ANIMATIONS_MD5];

	TVertices = new VERTEX_MD5[numTVertices];
	TIndices = new INDEX_MD5[numTIndices];
	TWeights = new Weight_MD5[numTWeights];

	memset(ModelJoints, 0, sizeof(Joint_MD5)*numJoints);
	memset(ModelObjects, 0, sizeof(Object_MD5)*numObjects);
	memset(ModelMaterials, 0, sizeof(Material_MD5)*numMaterials);
	memset(ModelMeshes, 0, sizeof(Mesh_MD5)*numMeshes);
	memset(ModelAnimations, 0, sizeof(Animation_MD5)*MAX_ANIMATIONS_MD5);

	memset(TVertices, 0, sizeof(VERTEX_MD5)*numTVertices);
	memset(TIndices, 0, sizeof(INDEX_MD5)*numTIndices);
	memset(TWeights, 0, sizeof(Weight_MD5)*numTWeights);

	OpenMeshFromFile(NewFileName);


	// 모델용 정점 및 색인 버퍼 생성
	SAFE_RELEASE(pModelVB);
	SAFE_RELEASE(pModelIB);

	int SizeOfVertices = sizeof(VERTEX_MD5)*numTVertices;
	if (FAILED(pDevice->CreateVertexBuffer(SizeOfVertices, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
		D3DFVF_VERTEX_MD5, D3DPOOL_DEFAULT, &pModelVB, NULL)))
		return E_FAIL;

	int SizeOfIndices = sizeof(INDEX_MD5)*numTIndices;
	if (FAILED(pDevice->CreateIndexBuffer(SizeOfIndices, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16, D3DPOOL_DEFAULT, &pModelIB, NULL)))
		return E_FAIL;

	// 노멀 벡터용 정점 및 색인 버퍼 생성
	SAFE_RELEASE(pNVVB);
	SAFE_RELEASE(pNVIB);

	int numVertices = numTVertices * 2;
	int numIndices = numVertices / 2;

	int SizeOfNVertices = sizeof(VERTEX_MD5_NORMAL)*numVertices;
	if (FAILED(pDevice->CreateVertexBuffer(SizeOfNVertices, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
		D3DFVF_VERTEX_MD5_NORMAL, D3DPOOL_DEFAULT, &pNVVB, NULL)))
		return E_FAIL;
	
	int SizeOfNIndices = sizeof(INDEX_MD5_NORMAL)*numIndices;
	if (FAILED(pDevice->CreateIndexBuffer(SizeOfNIndices, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16, D3DPOOL_DEFAULT, &pNVIB, NULL)))
		return E_FAIL;

	// 모델 텍스처 불러오기
	if (FAILED(SetTexture(0)))
		return E_FAIL;

	// 바운딩 박스 생성하기
	CreateModelBoundingBox();

	return S_OK;
}

void ModelMD5::SetBaseDirection(char* Dir)
{
	strcpy_s(BaseDir, Dir);
	return;
}

bool ModelMD5::ProofReadMeshFromFile(char* FileName)
{
	FILE*	fp;									// 불러올 파일
	char	sLine[MAX_PARSE_LINE] = { 0 };		// 파일에서 읽어올 한 줄
	char	NewFileName[MAX_NAME_LEN] = { 0 };

	strcpy_s(NewFileName, BaseDir);
	strcat_s(NewFileName, FileName);

	if (fopen_s(&fp, NewFileName, "rt"))
		return false;

	char	CurKey[MAX_KEYWORD_LEN] = { 0 };

	while (!feof(fp))
	{
		fgets(sLine, MAX_PARSE_LINE, fp);	// 파일에서 한 줄을 읽어온다.
		strcpy_s(sLine, StringTrim(sLine));	// 앞 뒤 공백 제거
		int iLen = strlen(sLine);

		if (iLen <= 1)						// 빈 줄이면 다음 줄로 넘어가자! (개행문자 때문에 최소 길이가 1이다.)★
			continue;


		// 중괄호 열기 전
		if (FindString_MD5(sLine, KeyWords_MD5[2]))	// numJoints ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			numJoints = (int)ParseFloats[1];
			continue;
		}

		if (FindString_MD5(sLine, KeyWords_MD5[3]))	// numMeshes ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			numMeshes = (int)ParseFloats[1];
			continue;
		}

		if (FindString_MD5(sLine, KeyWords_MD5[4]))	// numObjects ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			numObjects = (int)ParseFloats[1];
			continue;
		}

		if (FindString_MD5(sLine, KeyWords_MD5[5]))	// numMaterials ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			numMaterials = (int)ParseFloats[1];
			continue;
		}

		if (FindString_MD5(sLine, KeyWords_MD5[9]))	// mesh {
		{
			strcpy_s(CurKey, KeyWords_MD5[9]);			// 중괄호 열림!
			continue;
		}


		// 중괄호 열린 상태
		if (FindString_MD5(CurKey, KeyWords_MD5[9]))		// mesh {
		{
			if (FindString_MD5(sLine, KeyWords_MD5[13]))	// numverts
			{
				GetFloatFromLine(sLine, " ", TabChar);
				numTVertices += (int)ParseFloats[1];
				continue;
			}
			if (FindString_MD5(sLine, KeyWords_MD5[15]))	// numtris
			{
				GetFloatFromLine(sLine, " ", TabChar);
				numTIndices += (int)ParseFloats[1];
				continue;
			}
			if (FindString_MD5(sLine, KeyWords_MD5[17]))	// numweights
			{
				GetFloatFromLine(sLine, " ", TabChar);
				numTWeights += (int)ParseFloats[1];
				continue;
			}
			if (FindChar(sLine, "}"))
				memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기
			continue;
		}


	} // WHILE문 종료!

	fclose(fp);

	return true;
}

bool ModelMD5::OpenMeshFromFile(char* FileName)
{
	FILE*	fp;									// 불러올 파일
	char	sLine[MAX_PARSE_LINE] = {0};		// 파일에서 읽어올 한 줄
	char	NewFileName[MAX_NAME_LEN] = {0};

	strcpy_s(NewFileName, BaseDir);
	strcat_s(NewFileName, FileName);

	if( fopen_s(&fp, NewFileName, "rt") )
		return false;

	char	CurKey[MAX_KEYWORD_LEN] = {0};
	int		CurKeyCount = 0;
	int		MeshCount = 0;

	while(!feof(fp))
	{
		fgets(sLine, MAX_PARSE_LINE, fp);	// 파일에서 한 줄을 읽어온다.
		strcpy_s(sLine, StringTrim(sLine));	// 앞 뒤 공백 제거
		int iLen = strlen(sLine);

		if (iLen <= 1)						// 빈 줄이면 다음 줄로 넘어가자! (개행문자 때문에 최소 길이가 1이다.)★
			continue;


		// 중괄호 열기 전
		if ( FindString_MD5(sLine, KeyWords_MD5[0]) )	// Version_MD5 4843 & Version_MD5 10 지원 ★★
		{
			GetFloatFromLine(sLine, " ", TabChar);
			Version_MD5 = (int)ParseFloats[1];
			continue;
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[1]) )	// commandline "~"
		{
			continue;									// 주석은 건너뛰자
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[2]) )	// numJoints ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			numJoints = (int)ParseFloats[1];
			continue;
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[3]) )	// numMeshes ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			numMeshes = (int)ParseFloats[1];
			continue;
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[4]) )	// numObjects ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			numObjects = (int)ParseFloats[1];
			continue;
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[5]) )	// numMaterials ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			numMaterials = (int)ParseFloats[1];
			continue;
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[6]) )	// joints {
		{
			strcpy_s(CurKey, KeyWords_MD5[6]);			// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[7]) )	// objects {
		{
			strcpy_s(CurKey, KeyWords_MD5[7]);			// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[8]) )	// materials {
		{
			strcpy_s(CurKey, KeyWords_MD5[8]);			// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[9]) )	// mesh {
		{
			MeshCount++;
			strcpy_s(CurKey, KeyWords_MD5[9]);			// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}


		// 중괄호 열린 상태
		if ( FindString_MD5(CurKey, KeyWords_MD5[6]) )	// joints {
		{
			CurKeyCount++;

			Joint_MD5	TempJoint;

			// joint의 이름에 공백이 들어가 있는 경우에 대비해 이름 읽어온 후 지우고 float값을 얻어온다.
			char tempName[MAX_NAME_LEN];
			memset(tempName, 0, sizeof(tempName));
			strcpy_s( tempName, GetNameBetweenQuotes(sLine) );
			strcpy_s( TempJoint.Name, tempName );
			int	tempLen = strlen(tempName) + 2;			// 이름 & 앞뒤 쌍따옴표 제거하기 위함

			char tempLine[MAX_PARSE_LINE];
			memset(tempLine, 0, sizeof(tempLine));
			int	tempLineLen = strlen(sLine);

			for (int i = 0; i < tempLineLen-tempLen; i++)
			{
				tempLine[i] = sLine[i+tempLen];
			}

			GetFloatFromLine(tempLine, " ", TabChar);

			TempJoint.ParentID = (int)ParseFloats[0];
			TempJoint.Position.x = ParseFloats[2];
			TempJoint.Position.z = ParseFloats[3];		// 3DSMAX -> DIRECTX 좌표계!★
			TempJoint.Position.y = ParseFloats[4];
			TempJoint.Orientation.x = ParseFloats[7];
			TempJoint.Orientation.z = ParseFloats[8];	// 3DSMAX -> DIRECTX 좌표계!★
			TempJoint.Orientation.y = ParseFloats[9];

			// 사원수(Quaternion) w값 생성 (회전용 사원수는 x^2 + y^2+ z^2 + w^2 == 1 이어야 한다!)
			float t = 1.0f - ( TempJoint.Orientation.x * TempJoint.Orientation.x )
				- ( TempJoint.Orientation.y * TempJoint.Orientation.y )
				- ( TempJoint.Orientation.z * TempJoint.Orientation.z );
			if ( t < 0.0f )
			{
				TempJoint.Orientation.w = 0.0f;
			}
			else
			{
				TempJoint.Orientation.w = -sqrtf(t);
			}

			ModelJoints[CurKeyCount-1] = TempJoint;

			if (CurKeyCount >= numJoints)
				memset(CurKey, 0, sizeof(CurKey));		// 중괄호 닫기

			continue;
		}

		if ( FindString_MD5(CurKey, KeyWords_MD5[7]) )		// objects {
		{
			CurKeyCount++;
			strcpy_s( ModelObjects[CurKeyCount-1].Name, GetNameBetweenQuotes(sLine) );
			GetFloatFromLine(sLine, " ", TabChar);

			ModelObjects[CurKeyCount-1].ObjectID = (int)ParseFloats[1];
			ModelObjects[CurKeyCount-1].numMeshes = (int)ParseFloats[2];

			if (CurKeyCount >= numObjects)
				memset(CurKey, 0, sizeof(CurKey));			// 중괄호 닫기

			continue;
		}

		if ( FindString_MD5(CurKey, KeyWords_MD5[8]) )		// materials {
		{
			CurKeyCount++;
			strcpy_s( ModelMaterials[CurKeyCount-1].Name, SplitString(sLine, "\"", 0) );
			strcpy_s( ModelMaterials[CurKeyCount-1].TextureFileName, SplitString(sLine, "\"", 2) );

			if (CurKeyCount >= numMaterials)
				memset(CurKey, 0, sizeof(CurKey));			// 중괄호 닫기

			continue;
		}

		if ( FindString_MD5(CurKey, KeyWords_MD5[9]) )		// mesh {
		{
			if ( FindString_MD5(sLine, KeyWords_MD5[10]) )	// '//' 주석 줄
				continue;									// 건너뛰자!

			if ( FindString_MD5(sLine, KeyWords_MD5[11]) )	// meshindex
				continue;

			if ( FindString_MD5(sLine, KeyWords_MD5[12]) )	// shader
			{
				if ( Version_MD5 == 4843 )	// 신버전이면 material의 이름이 나옴!★
				{
					strcpy_s( ModelMeshes[MeshCount-1].MaterialName, SplitString(sLine, "\"", 1) );
					for (int i = 0; i < numMaterials; i++)
					{
						if ( FindString_MD5( ModelMeshes[MeshCount-1].MaterialName, ModelMaterials[i].Name) )
							strcpy_s( ModelMaterials[MeshCount-1].TextureFileName,  ModelMaterials[i].TextureFileName );
					}
				}
				else						// 구버전이면 바로 텍스처 파일 이름이 나옴!
				{
					numMaterials++;
					strcpy_s( ModelMeshes[MeshCount-1].MaterialName, SplitString(sLine, "\"", 1) );
					strcpy_s( ModelMaterials[MeshCount-1].TextureFileName, SplitString(sLine, "\"", 1) );
				}

				continue;
			}
			if ( FindString_MD5(sLine, KeyWords_MD5[13]) )	// numverts
			{
				GetFloatFromLine(sLine, " ", TabChar);
				ModelMeshes[MeshCount-1].numMeshVertices = (int)ParseFloats[1];
				continue;
			}
			if ( FindString_MD5(sLine, KeyWords_MD5[14]) )	// vert
			{
				GetFloatFromLine(sLine, " ", TabChar);
				int VIndex = (int)ParseFloats[1];
				int TVIndex = 0;
				int	VWeightBase = 0;

				if (MeshCount > 1)
				{
					for (int i = 0; i < MeshCount-1; i++)
					{
						TVIndex += ModelMeshes[i].numMeshVertices;
						VWeightBase += ModelMeshes[i].numMeshWeights;
					}
				}

				TVIndex += VIndex;

				TVertices[TVIndex].Texture.x	= ParseFloats[3];
				TVertices[TVIndex].Texture.y	= ParseFloats[4];
				TVertices[TVIndex].WeightStart	= (float)(VWeightBase + (int)ParseFloats[6]);
				TVertices[TVIndex].WeightNum	= (float)ParseFloats[7];
				continue;
			}
			if ( FindString_MD5(sLine, KeyWords_MD5[15]) )	// numtris
			{
				GetFloatFromLine(sLine, " ", TabChar);
				ModelMeshes[MeshCount-1].numMeshIndices = (int)ParseFloats[1];
				continue;
			}
			if ( FindString_MD5(sLine, KeyWords_MD5[16]) )	// tri
			{
				GetFloatFromLine(sLine, " ", TabChar);
				int TIndex = (int)ParseFloats[1];
				int TTIndex = 0;
				int	IndexBase = 0;

				if (MeshCount > 1)
				{
					for (int i = 0; i < MeshCount-1; i++)
					{
						TTIndex += ModelMeshes[i].numMeshIndices;
						IndexBase += ModelMeshes[i].numMeshVertices;
					}
				}

				TTIndex += TIndex;

				TIndices[TTIndex]._0	= IndexBase + (int)ParseFloats[2];
				TIndices[TTIndex]._1	= IndexBase + (int)ParseFloats[3];
				TIndices[TTIndex]._2	= IndexBase + (int)ParseFloats[4];

				continue;
			}
			if ( FindString_MD5(sLine, KeyWords_MD5[17]) )	// numweights
			{
				GetFloatFromLine(sLine, " ", TabChar);
				ModelMeshes[MeshCount-1].numMeshWeights = (int)ParseFloats[1];
				continue;
			}
			if ( FindString_MD5(sLine, KeyWords_MD5[18]) )	// weight
			{
				GetFloatFromLine(sLine, " ", TabChar);
				int WIndex = (int)ParseFloats[1];
				int TWIndex = 0;

				if (MeshCount > 1)
				{
					for (int i = 0; i < MeshCount-1; i++)
					{
						TWIndex += ModelMeshes[i].numMeshWeights;
					}
				}

				TWIndex += WIndex;

				TWeights[TWIndex].JointID		= (int)ParseFloats[2];
				TWeights[TWIndex].Bias			= ParseFloats[3];
				TWeights[TWIndex].Position.x	= ParseFloats[5];
				TWeights[TWIndex].Position.z	= ParseFloats[6];		// 3DSMAX -> DIRECTX 좌표계!★
				TWeights[TWIndex].Position.y	= ParseFloats[7];

				continue;
			}

			if (FindChar(sLine, "}"))
				memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기
			continue;
		}


	} // WHILE문 종료!


	// 정점 정보 갱신
	for (int j = 0; j < numTVertices; j++)
	{
		int WeightStart	= (int)TVertices[j].WeightStart;
		int nWeights	= (int)TVertices[j].WeightNum;

		XMFLOAT3 Result	= XMFLOAT3(0.0f, 0.0f, 0.0f);

		for (int k = 0; k < nWeights; k++)
		{
			int CurJointID = TWeights[WeightStart+k].JointID;
			Joint_MD5	TempJoint	= ModelJoints[CurJointID];
			Weight_MD5	TempWeight	= TWeights[WeightStart+k];

			// 사원수(Quaternion)을 사용해 정점을 회전시킨다. (사원수 * 정점 * 사원수의 역)
			XMVECTOR	QUAT, POS;
			XMFLOAT3	Rotated;

			QUAT = XMVectorSet(TempJoint.Orientation.x, TempJoint.Orientation.y, TempJoint.Orientation.z, TempJoint.Orientation.w);
			POS	= XMVectorSet(TempWeight.Position.x, TempWeight.Position.y, TempWeight.Position.z, 0.0f);
			
			XMStoreFloat3(&Rotated, XMQuaternionMultiply(XMQuaternionMultiply(QUAT, POS), XMQuaternionInverse(QUAT)));
			//Rotated = QuaternionRotate(QUAT, POS);
			
			Result.x += (TempJoint.Position.x + Rotated.x) * TempWeight.Bias;
			Result.y += (TempJoint.Position.y + Rotated.y) * TempWeight.Bias;
			Result.z += (TempJoint.Position.z + Rotated.z) * TempWeight.Bias;
		}

		TVertices[j].Position = Result;
	}


	// 법선 계산 ★★★
	for(int j = 0; j < numTVertices; ++j)
	{
		TVertices[j].Normal.x = 0.0f;
		TVertices[j].Normal.y = 0.0f;
		TVertices[j].Normal.z = 0.0f;
	}

	XMFLOAT3* tempNormal = new XMFLOAT3[numTIndices];
	XMFLOAT3 unnormalized = XMFLOAT3(0.0f, 0.0f, 0.0f);

	float vecX, vecY, vecZ;

	XMVECTOR edge1;
	XMVECTOR edge2;

	for(int j = 0; j < numTIndices; ++j)
	{
		// 0-2 모서리 벡터
		vecX = TVertices[TIndices[j]._0].Position.x - TVertices[TIndices[j]._2].Position.x;
		vecY = TVertices[TIndices[j]._0].Position.y - TVertices[TIndices[j]._2].Position.y;
		vecZ = TVertices[TIndices[j]._0].Position.z - TVertices[TIndices[j]._2].Position.z;		
		edge1 = XMVectorSet(vecX, vecY, vecZ, 0.0f);

		// 2-1 모서리 벡터
		vecX = TVertices[TIndices[j]._2].Position.x - TVertices[TIndices[j]._1].Position.x;
		vecY = TVertices[TIndices[j]._2].Position.y - TVertices[TIndices[j]._1].Position.y;
		vecZ = TVertices[TIndices[j]._2].Position.z - TVertices[TIndices[j]._1].Position.z;
		edge2 = XMVectorSet(vecX, vecY, vecZ, 0.0f);

		// 두 모서리 벡터의 외적으로 법선을 구한다.
		XMStoreFloat3(&unnormalized,(XMVector3Cross(edge1, edge2)));
		
		tempNormal[j] = unnormalized;
	}

	//Compute vertex normals (normal Averaging)
	XMVECTOR normalSum = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	int facesUsing = 0;
	float tX, tY, tZ;	//temp axis variables

	for(int j = 0; j < numTVertices; ++j)
	{
		for(int k = 0; k < numTIndices; ++k)
		{
			if( TIndices[k]._0 == j || TIndices[k]._1 == j || TIndices[k]._2 == j )
			{
				tX = XMVectorGetX(normalSum) + tempNormal[k].x;
				tY = XMVectorGetY(normalSum) + tempNormal[k].y;
				tZ = XMVectorGetZ(normalSum) + tempNormal[k].z;

				normalSum = XMVectorSet(tX, tY, tZ, 0.0f);	//If a face is using the vertex, add the unormalized face normal to the normalSum

				facesUsing++;
			}
		}

		normalSum = normalSum / (float)facesUsing;
		normalSum = XMVector3Normalize(normalSum);

		TVertices[j].Normal.x = -XMVectorGetX(normalSum);
		TVertices[j].Normal.y = -XMVectorGetY(normalSum);
		TVertices[j].Normal.z = -XMVectorGetZ(normalSum);

		VERTEX_MD5 tempVert = TVertices[j];				// Get the current vertex
		XMVECTOR normal = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);	// Clear normal

		int WeightStart	= (int)TVertices[j].WeightStart;
		int nWeights	= (int)TVertices[j].WeightNum;

		for ( int k = 0; k < nWeights; k++)											// Loop through each of the vertices weights
		{
			Joint_MD5 tempJoint = ModelJoints[TWeights[WeightStart + k].JointID];	// Get the joints orientation
			XMVECTOR jointOrientation = XMVectorSet(tempJoint.Orientation.x, tempJoint.Orientation.y, tempJoint.Orientation.z, tempJoint.Orientation.w);

			normal = XMQuaternionMultiply(XMQuaternionMultiply(jointOrientation, normalSum), XMQuaternionInverse(jointOrientation));
			XMStoreFloat3(&TWeights[WeightStart + k].Normal, XMVector3Normalize(normal));
		}				

		normalSum = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
		facesUsing = 0;
	}

	SAFE_DELETEARRAY(tempNormal);

	fclose(fp);

	return true;
}

HRESULT ModelMD5::SetTexture(int MeshIndex)
{
	HRESULT hr;
	SAFE_RELEASE(ModelTextures[MeshIndex]);

	// 텍스처 불러오기
	char	NewFileName[MAX_NAME_LEN] = {0};
	strcpy_s(NewFileName, BaseDir);
	strcat_s(NewFileName, ModelMaterials[MeshIndex].TextureFileName);

	hr = D3DXCreateTextureFromFile(pDevice, NewFileName, &ModelTextures[MeshIndex]);

	return hr;
}

void ModelMD5::CreateModelBoundingBox()
{
	UpdateModelBoundingBox();

	// 바운딩 박스용 정점 및 색인 버퍼 생성
	SAFE_RELEASE(pBBVB);
	SAFE_RELEASE(pBBIB);

	int numVertices = 8;	// 상자니까 점 8개 필요
	int SizeOfVertices = sizeof(VERTEX_MD5_BB)*numVertices;
	if (FAILED(pDevice->CreateVertexBuffer(SizeOfVertices, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
		D3DFVF_VERTEX_MD5_BB, D3DPOOL_DEFAULT, &pBBVB, NULL)))
		return;

	int numIndices = 12;	// 상자니까 모서리 12개 필요
	int SizeOfIndices = sizeof(INDEX_MD5_BB)*numIndices;
	if (FAILED(pDevice->CreateIndexBuffer(SizeOfIndices, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16, D3DPOOL_DEFAULT, &pBBIB, NULL)))
		return;

	return;
}


void ModelMD5::AddAnimation(int AnimationID, char* AnimationFileName, float AnimationSpeed)
{
	// 모델 애니메이션 불러오기
	char	NewFileName[MAX_NAME_LEN] = {0};
	memset(NewFileName, 0, sizeof(NewFileName));
	strcat_s(NewFileName, AnimationFileName);
	strcat_s(NewFileName, ".MD5ANIM");

	OpenAnimationFromFile(AnimationID, NewFileName);

	if (AnimationSpeed <= 0.0f)	// 애니메이션 속도 미지정 시
		AnimationSpeed = 0.02f;	// 기본 애니메이션 속도 지정

	ModelAnimations[AnimationID].BasicAnimSpeed = AnimationSpeed;

	// 애니메이션 개수 1개 증가
	numAnimations++;

	return;
}

bool ModelMD5::OpenAnimationFromFile(int AnimationID, char* FileName)
{
	FILE*	fp;									// 불러올 파일
	char	sLine[MAX_PARSE_LINE] = {0};		// 파일에서 읽어올 한 줄
	char	NewFileName[MAX_NAME_LEN] = {0};

	strcpy_s(NewFileName, BaseDir);
	strcat_s(NewFileName, FileName);

	if( fopen_s(&fp, NewFileName, "rt") )
		return false;

	char	CurKey[MAX_KEYWORD_LEN] = {0};
	int		CurKeyCount = 0;
	int		FrameCount = 0;
	int		TempFrameID = 0;

	while(!feof(fp))
	{
		fgets(sLine, MAX_PARSE_LINE, fp);	// 파일에서 한 줄을 읽어온다.
		strcpy_s(sLine, StringTrim(sLine));					// 앞 뒤 공백 제거
		int iLen = strlen(sLine);

		if (iLen <= 1)						// 빈 줄이면 다음 줄로 넘어가자! (개행문자 때문에 최소 길이가 1이다.)★
			continue;

		// 중괄호 열기 전
		if ( FindString_MD5(sLine, KeyWords_MD5[0]) )	// Version_MD5 4843 & 10
		{
			GetFloatFromLine(sLine, " ", TabChar);
			// if ( Version_MD5 != (int)ParseFloats[1]);	// 모델과 애니메이션의 버전이 달라도 그냥 불러와 보자..
			continue;
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[1]) )	// commandline "~"
		{
			continue;									// 주석은 건너뛰자
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[19]) )	// numFrames ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			ModelAnimations[AnimationID].numFrames = (int)ParseFloats[1];
			continue;
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[2]) )	// numJoints ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			ModelAnimations[AnimationID].numJoints = (int)ParseFloats[1];
			if (ModelAnimations[AnimationID].numJoints != numJoints)	// 애니메이션의 numJoints가 불러온 모델의 numJoints와 다를 경우
				return false;											// 불러오기를 중단한다. ★★
			continue;
		}
		if ( FindString_MD5(sLine, KeyWords_MD5[20]) )	// frameRate ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			ModelAnimations[AnimationID].FrameRate = (int)ParseFloats[1];
			continue;
		}
		if ( FindString_MD5(sLine, KeyWords_MD5[21]) )	// numAnimatedComponents ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			ModelAnimations[AnimationID].numAnimatedComponents = (int)ParseFloats[1];
			continue;
		}
		if ( FindString_MD5(sLine, KeyWords_MD5[22]) )	// hierarchy {
		{
			strcpy_s(CurKey, KeyWords_MD5[22]);			// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}
		if ( FindString_MD5(sLine, KeyWords_MD5[23]) )	// bounds {
		{
			strcpy_s(CurKey, KeyWords_MD5[23]);			// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}
		if ( FindString_MD5(sLine, KeyWords_MD5[24]) )	// baseframe {
		{
			strcpy_s(CurKey, KeyWords_MD5[24]);			// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}
		if ( FindString_MD5(sLine, KeyWords_MD5[25]) )	// frame ~ {
		{
			FrameCount++;
			GetFloatFromLine(sLine, " ", TabChar);
			TempFrameID = (int)ParseFloats[1];

			strcpy_s(CurKey, KeyWords_MD5[25]);			// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}


		// 중괄호 열린 상태
		if ( FindString_MD5(CurKey, KeyWords_MD5[22]) )	// hierarchy {
		{
			CurKeyCount++;

			// hierarchy(즉, joint)의 이름에 공백이 들어가 있는 경우에 대비해 이름을 읽어온 후 지워준 후 float값을 얻어온다.
			char tempName[MAX_NAME_LEN];
			memset(tempName, 0, sizeof(tempName));
			strcpy_s( tempName, SplitString(sLine, "\"", 0) );
			strcpy_s( ModelAnimations[AnimationID].JointInfo[CurKeyCount-1].Name, tempName );
			int	tempLen = strlen(tempName) + 2;			// 이름 & 앞뒤 쌍따옴표 제거하기 위함

			char tempLine[MAX_PARSE_LINE];
			memset(tempLine, 0, sizeof(tempLine));
			int	tempLineLen = strlen(sLine);
			for (int i = 0; i < tempLineLen-tempLen; i++)
			{
				tempLine[i] = sLine[i+tempLen];
			}

			GetFloatFromLine(tempLine, " ", TabChar);
			ModelAnimations[AnimationID].JointInfo[CurKeyCount-1].ParentID	= (int)ParseFloats[0];
			ModelAnimations[AnimationID].JointInfo[CurKeyCount-1].Flags		= (int)ParseFloats[1];
			ModelAnimations[AnimationID].JointInfo[CurKeyCount-1].StartIndex	= (int)ParseFloats[2];

			if (CurKeyCount >= ModelAnimations[AnimationID].numJoints)
				memset(CurKey, 0, sizeof(CurKey));		// 중괄호 닫기

			continue;
		}

		if ( FindString_MD5(CurKey, KeyWords_MD5[23]) )	// bounds {
		{
			CurKeyCount++;
			GetFloatFromLine(sLine, " ", TabChar);

			BoundingBox_MD5	TempBB;

			TempBB.Min.x	= ParseFloats[1];
			TempBB.Min.z	= ParseFloats[2];	// 3DSMAX -> DIRECTX 좌표계!★
			TempBB.Min.y	= ParseFloats[3];
			TempBB.Max.x	= ParseFloats[6];
			TempBB.Max.z	= ParseFloats[7];	// 3DSMAX -> DIRECTX 좌표계!★
			TempBB.Max.y	= ParseFloats[8];

			ModelAnimations[AnimationID].BoundingBoxes[CurKeyCount-1] = TempBB;

			if (CurKeyCount >= ModelAnimations[AnimationID].numFrames)
				memset(CurKey, 0, sizeof(CurKey));		// 중괄호 닫기

			continue;
		}

		if ( FindString_MD5(CurKey, KeyWords_MD5[24]) )	// baseframe {
		{
			// BASEFRAME: Joint 개수만큼 존재, 각 Frame의 기반이 되는 위치!★
			CurKeyCount++;

			GetFloatFromLine(sLine, " ", TabChar);

			Joint_MD5	TempBaseFrame;

			TempBaseFrame.Position.x		= ParseFloats[1];
			TempBaseFrame.Position.z		= ParseFloats[2];	// 3DSMAX -> DIRECTX 좌표계!★
			TempBaseFrame.Position.y		= ParseFloats[3];
			TempBaseFrame.Orientation.x		= ParseFloats[6];
			TempBaseFrame.Orientation.z		= ParseFloats[7];	// 3DSMAX -> DIRECTX 좌표계!★
			TempBaseFrame.Orientation.y		= ParseFloats[8];

			ModelAnimations[AnimationID].JointBaseFrame[CurKeyCount-1] = TempBaseFrame;

			if (CurKeyCount >= ModelAnimations[AnimationID].numJoints)
				memset(CurKey, 0, sizeof(CurKey));		// 중괄호 닫기

			continue;
		}

		if ( FindString_MD5(CurKey, KeyWords_MD5[25]) )	// frame ~ {
		{
			// FRAME: 각 Frame이 Baseframe에서 변화한 위치!★
			Frame_MD5	TempFrameData;
			memset(&TempFrameData, 0, sizeof(TempFrameData));
			TempFrameData.FrameID = TempFrameID;

			for(int i = 0; i < ModelAnimations[AnimationID].numJoints; i++)
			{
				if (ModelAnimations[AnimationID].JointInfo[i].Flags == 0)	 // 플래그가 0이면 건너뛰자!★
					continue;

				GetFloatFromLine(sLine, " ", TabChar);

				int k = ModelAnimations[AnimationID].JointInfo[i].StartIndex;

				if (ModelAnimations[AnimationID].numAnimatedComponents < 6)
				{
					for (int j = 0; j < ModelAnimations[AnimationID].numAnimatedComponents; j++)
					{
						TempFrameData.JointData[k++] = ParseFloats[j];
					}
				}
				else
				{
					TempFrameData.JointData[k++] = ParseFloats[0];
					TempFrameData.JointData[k++] = ParseFloats[1];
					TempFrameData.JointData[k++] = ParseFloats[2];
					TempFrameData.JointData[k++] = ParseFloats[3];
					TempFrameData.JointData[k++] = ParseFloats[4];
					TempFrameData.JointData[k] = ParseFloats[5];
				}

				fgets(sLine, MAX_PARSE_LINE, fp);	// 파일에서 한 줄을 읽어온다.
				strcpy_s(sLine, StringTrim(sLine));	// 앞 뒤 공백 제거
			}

			ModelAnimations[AnimationID].FrameData[FrameCount-1] = TempFrameData;


			for (int i = 0; i < ModelAnimations[AnimationID].numJoints; i++)
			{
				Joint_MD5 TempFrameJoint = ModelAnimations[AnimationID].JointBaseFrame[i];	// 기본적으로 좌표는 BaseFrame 값에서 출발!
				TempFrameJoint.ParentID = ModelAnimations[AnimationID].JointInfo[i].ParentID;

				int tempJointIDStart = ModelAnimations[AnimationID].JointInfo[i].StartIndex;

				if(ModelAnimations[AnimationID].JointInfo[i].Flags & 1)	//	Position.x	( 000001 )
					TempFrameJoint.Position.x		= TempFrameData.JointData[tempJointIDStart++];

				if(ModelAnimations[AnimationID].JointInfo[i].Flags & 2)	//	Position.y	( 000010 )
					TempFrameJoint.Position.z		= TempFrameData.JointData[tempJointIDStart++];	// 3DSMAX -> DIRECTX 좌표계!★

				if(ModelAnimations[AnimationID].JointInfo[i].Flags & 4)	//	Position.z	( 000100 )
					TempFrameJoint.Position.y		= TempFrameData.JointData[tempJointIDStart++];

				if(ModelAnimations[AnimationID].JointInfo[i].Flags & 8)	//	Orientation.x	( 001000 )
					TempFrameJoint.Orientation.x	= TempFrameData.JointData[tempJointIDStart++];

				if(ModelAnimations[AnimationID].JointInfo[i].Flags & 16)	//	Orientation.y	( 010000 )
					TempFrameJoint.Orientation.z	= TempFrameData.JointData[tempJointIDStart++];	// 3DSMAX -> DIRECTX 좌표계!★

				if(ModelAnimations[AnimationID].JointInfo[i].Flags & 32)	//	Orientation.z	( 100000 )
					TempFrameJoint.Orientation.y	= TempFrameData.JointData[tempJointIDStart];

				// 회전을 위한 사원수(Quaternion) w값 생성
				float t = 1.0f - ( TempFrameJoint.Orientation.x * TempFrameJoint.Orientation.x )
					- ( TempFrameJoint.Orientation.y * TempFrameJoint.Orientation.y )
					- ( TempFrameJoint.Orientation.z * TempFrameJoint.Orientation.z );
				if ( t < 0.0f )
				{
					TempFrameJoint.Orientation.w = 0.0f;
				}
				else
				{
					TempFrameJoint.Orientation.w = -sqrtf(t);
				}

				if (TempFrameJoint.ParentID >= 0)	// 루트 프레임(ParentID = -1)이 아닌 경우, 부모->자식 순으로 모든 계산을 적용해야 한다!★★★
				{
					Joint_MD5	ParentJoint = ModelAnimations[AnimationID].FrameSekelton[FrameCount-1].Skeleton[TempFrameJoint.ParentID];

					XMVECTOR	QUAT, POS;
					XMFLOAT3	RotatedPos;

					QUAT = XMVectorSet(ParentJoint.Orientation.x, ParentJoint.Orientation.y, ParentJoint.Orientation.z, ParentJoint.Orientation.w);
					POS	= XMVectorSet(TempFrameJoint.Position.x, TempFrameJoint.Position.y, TempFrameJoint.Position.z, 0.0f);
					
					XMStoreFloat3(&RotatedPos, XMQuaternionMultiply(XMQuaternionMultiply(QUAT, POS), XMQuaternionInverse(QUAT)));

					TempFrameJoint.Position.x = RotatedPos.x + ParentJoint.Position.x;
					TempFrameJoint.Position.y = RotatedPos.y + ParentJoint.Position.y;
					TempFrameJoint.Position.z = RotatedPos.z + ParentJoint.Position.z;

					XMVECTOR	TempJointOrient;
					TempJointOrient = XMVectorSet(TempFrameJoint.Orientation.x, TempFrameJoint.Orientation.y, TempFrameJoint.Orientation.z, TempFrameJoint.Orientation.w);
					TempJointOrient = XMQuaternionMultiply(QUAT, TempJointOrient);
					TempJointOrient = XMQuaternionNormalize(TempJointOrient);

					XMStoreFloat4(&TempFrameJoint.Orientation, TempJointOrient);
				}

				ModelAnimations[AnimationID].FrameSekelton[FrameCount-1].Skeleton[i] = TempFrameJoint;
			}

			memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기
			continue;
		}
	}

	// 변수 초기화
	ModelAnimations[AnimationID].FrameTime = 1.0f / ModelAnimations[AnimationID].FrameRate;		// 각 프레임(Frame) 당 시간 설정 ★
	ModelAnimations[AnimationID].TotalAnimTime =
		ModelAnimations[AnimationID].numFrames * ModelAnimations[AnimationID].FrameTime;		// 애니메이션 전체 길이

	return true;
}

HRESULT ModelMD5::CreateAnimationJointTexture()
{
	// ★ Width: 800 = (4 = XMFLOAT4 JointPosition + 4 = XMFLOAT4 JointOrientation) * (100 = MAX_JOINTS_MD5)
	// ★ D3DFMT_R32F: 1 Texel = 1 float & D3DFMT_A32B32G32R32F: 1 Texel = 4 float
	if (FAILED(pDevice->CreateTexture(8 * MAX_JOINTS_MD5, MAX_FRAMES_MD5 * MAX_ANIMATIONS_MD5, 1, 0, D3DFMT_R32F, D3DPOOL_MANAGED, &AnimationJointTexture, 0)))
		return E_FAIL;

	D3DLOCKED_RECT	LockedRect = {0,};
	AnimationJointTexture->LockRect(0, &LockedRect, NULL, 0);

	// 텍스처에 애니메이션 정보 담기
	int		tempCount = 0;
	float*	tempFloats = new float[8 * MAX_JOINTS_MD5 * MAX_FRAMES_MD5 * MAX_ANIMATIONS_MD5];	// 800 = 8(Position xyzw + Orientation xyzw) * 100(numJoints) => * MAX_INSTANCES_MD5
	int		tempSize = sizeof(float) * 8 * MAX_JOINTS_MD5 * MAX_FRAMES_MD5 * MAX_ANIMATIONS_MD5;	// 4(size of float) * 800(size of array) * MAX_INSTANCES_MD5
	memset(tempFloats, 0, tempSize);

	for (int k = 0; k < MAX_ANIMATIONS_MD5; k++)
	{
		for (int i = 0; i < MAX_FRAMES_MD5; i++)
		{
			for (int j = 0; j < MAX_JOINTS_MD5; j++)
			{
				tempFloats[0 + tempCount] = ModelAnimations[k].FrameSekelton[i].Skeleton[j].Position.x;
				tempFloats[1 + tempCount] = ModelAnimations[k].FrameSekelton[i].Skeleton[j].Position.y;
				tempFloats[2 + tempCount] = ModelAnimations[k].FrameSekelton[i].Skeleton[j].Position.z;
				tempFloats[3 + tempCount] = 1;
				tempFloats[4 + tempCount] = ModelAnimations[k].FrameSekelton[i].Skeleton[j].Orientation.x;
				tempFloats[5 + tempCount] = ModelAnimations[k].FrameSekelton[i].Skeleton[j].Orientation.y;
				tempFloats[6 + tempCount] = ModelAnimations[k].FrameSekelton[i].Skeleton[j].Orientation.z;
				tempFloats[7 + tempCount] = ModelAnimations[k].FrameSekelton[i].Skeleton[j].Orientation.w;
			
				tempCount += 8;
			}
		}
	}

	memcpy(LockedRect.pBits, tempFloats, tempSize);
	AnimationJointTexture->UnlockRect(0);

	SAFE_DELETEARRAY(tempFloats);

	pHLSL->SetTexture("AnimationJoint_Tex", AnimationJointTexture);

	return S_OK;
}

HRESULT ModelMD5::CreateAnimationWeightTexture()
{
	if (FAILED(pDevice->CreateTexture(8 * 100, MAX_WEIGHTS_MD5 / 100, 1, 0, D3DFMT_R32F, D3DPOOL_MANAGED, &AnimationWeightTexture, 0)))
		return E_FAIL;

	D3DLOCKED_RECT	LockedRect = {0,};
	AnimationWeightTexture->LockRect(0, &LockedRect, NULL, 0);

	// 텍스처에 애니메이션 정보 담기
	int		tempCount = 0;
	float*	tempFloats = new float[8 * numTWeights];
	int		tempSize = sizeof(float) * 8 * numTWeights;
	memset(tempFloats, 0, tempSize);

	for (int i = 0; i < numTWeights; i++)
	{
			tempFloats[0 + tempCount] = (float)TWeights[i].JointID;
			tempFloats[1 + tempCount] = TWeights[i].Bias;
			tempFloats[2 + tempCount] = TWeights[i].Position.x;
			tempFloats[3 + tempCount] = TWeights[i].Position.y;
			tempFloats[4 + tempCount] = TWeights[i].Position.z;
			tempFloats[5 + tempCount] = TWeights[i].Normal.x;
			tempFloats[6 + tempCount] = TWeights[i].Normal.y;
			tempFloats[7 + tempCount] = TWeights[i].Normal.z;
			
			tempCount += 8;
	}

	memcpy(LockedRect.pBits, tempFloats, tempSize);
	AnimationWeightTexture->UnlockRect(0);

	SAFE_DELETEARRAY(tempFloats);

	pHLSL->SetTexture("AnimationWeight_Tex", AnimationWeightTexture);

	return S_OK;
}

void ModelMD5::AddAnimationEnd()
{
	if (HLSLInstancing)
	{
		// HLSL 인스턴싱 사용
		CreateAnimationJointTexture();
		CreateAnimationWeightTexture();
	}
	else
	{

	}

	return;
}


void ModelMD5::AddInstance(XMFLOAT3 Translation, XMFLOAT3 Rotation, XMFLOAT3 Scaling)
{
	numInstances++;

	memset(&ModelInstances[numInstances-1], 0, sizeof(ModelInstances[numInstances-1]));

	SetInstance(numInstances-1, Translation, Rotation, Scaling);

	return;
}

void ModelMD5::CreateInstanceBuffer()
{
	SAFE_RELEASE(g_pVBDeclaration);
	SAFE_RELEASE(pModelInstanceVB);

	pDevice->CreateVertexDeclaration( g_VBElements, &g_pVBDeclaration );
	pDevice->CreateVertexBuffer( numInstances * sizeof( INSTANCE_DATA_MD5 ), 0, 0, D3DPOOL_MANAGED, &pModelInstanceVB, 0 );
}

void ModelMD5::AddInstanceEnd()
{
	if (HLSLInstancing)
	{
		// HLSL 인스턴싱 사용
		CreateInstanceBuffer();
	}
	else
	{
		
	}

	return;
}


void ModelMD5::SetInstance(int InstanceID, XMFLOAT3 Translation, XMFLOAT3 Rotation, XMFLOAT3 Scaling)
{
	ModelInstances[InstanceID].Translation = Translation;
	ModelInstances[InstanceID].Rotation = Rotation;
	ModelInstances[InstanceID].Scaling = Scaling;

	// 월드 행렬(위치, 회전, 크기 설정)
	D3DXMatrixIdentity( &matModelWorld[InstanceID] );

		D3DXMATRIXA16 matTrans;
		D3DXMATRIXA16 matRotX;
		D3DXMATRIXA16 matRotY;
		D3DXMATRIXA16 matRotZ;
		D3DXMATRIXA16 matSize;

		D3DXMatrixTranslation(&matTrans, Translation.x, Translation.y, Translation.z);
		D3DXMatrixRotationX(&matRotX, Rotation.x);
		D3DXMatrixRotationY(&matRotY, Rotation.y);				// Y축을 기준으로 회전 (즉, X&Z가 회전함)
		D3DXMatrixRotationZ(&matRotZ, Rotation.z);
		D3DXMatrixScaling(&matSize, Scaling.x, Scaling.y, Scaling.z);

	matModelWorld[InstanceID] = matModelWorld[InstanceID] * matRotX * matRotY * matRotZ * matSize * matTrans;

	return;
}

bool ModelMD5::SetInstanceAnimation(int InstanceID, int AnimationID, float StartAnimTime)
{
	if (AnimationID > numAnimations)
		AnimationID = 0;

	ModelInstances[InstanceID].CurAnimID = AnimationID;

	if (StartAnimTime > ModelAnimations[AnimationID].TotalAnimTime)
		StartAnimTime = 0;

	if (StartAnimTime < 0)	// (-)값이 들어오면 시작 시간은 초기화하지 않는다.
		return true;

	ModelInstances[InstanceID].CurAnimTime = StartAnimTime;

	return true;
}

void ModelMD5::AnimateInstance(int InstanceID, float Speed)
{
	int CurrentAnimID = ModelInstances[InstanceID].CurAnimID;

	ModelInstances[InstanceID].BeingAnimated = true;

	if (Speed == 0.0f)
		Speed = ModelAnimations[CurrentAnimID].BasicAnimSpeed;	// 속도 지정을 0으로 하면 기본 속도로 재생

	ModelInstances[InstanceID].CurAnimTime += Speed;

	if(ModelInstances[InstanceID].CurAnimTime > ModelAnimations[CurrentAnimID].TotalAnimTime)	// 애니메이션이 끝나면
		ModelInstances[InstanceID].CurAnimTime = 0.0f;											// 다시 처음으로! ★

	float CurrentFrame =
		ModelInstances[InstanceID].CurAnimTime * ModelAnimations[CurrentAnimID].FrameRate;		// 현재 프레임
	int Frame0 = (int)floorf( CurrentFrame );													// 보간을 위한 현재 프레임 값
	int Frame1 = Frame0 + 1;																	// 보간을 위한 다음 프레임 값

	if (Frame0 == ModelAnimations[CurrentAnimID].numFrames-1)	// Frame0이 마지막 프레임이라면 Frame1은 다시 0번으로 가자!
		Frame1 = 0;

	if (HLSLInstancing)
	{
		// HLSL 인스턴싱
		ModelInstances[InstanceID].CurFrameTime = CurrentFrame;
		ModelInstances[InstanceID].Frame0 = Frame0;
		ModelInstances[InstanceID].Frame1 = Frame1;
	}
	else
	{
		// HLSL 인스턴싱 미사용 & HLSL 미사용

		// 보간값(Interpolation) 계산
		float Interpolation = CurrentFrame - Frame0;				// Frame0과 CurrentFrame의 시간차를 얻어와 보간값으로 사용한다.
		Joint_MD5* InterpolatedJoints = new Joint_MD5[MAX_JOINTS_MD5];

		for( int i = 0; i < ModelAnimations[CurrentAnimID].numJoints; i++)
		{
			Joint_MD5 TempJoint;
			Joint_MD5 Joint0 = ModelAnimations[CurrentAnimID].FrameSekelton[Frame0].Skeleton[i];	// Frame0의 i번째 JointData
			Joint_MD5 Joint1 = ModelAnimations[CurrentAnimID].FrameSekelton[Frame1].Skeleton[i];	// Frame1의 i번째 JointData

			TempJoint.Position.x = Joint0.Position.x + (Interpolation * (Joint1.Position.x - Joint0.Position.x));
			TempJoint.Position.y = Joint0.Position.y + (Interpolation * (Joint1.Position.y - Joint0.Position.y));
			TempJoint.Position.z = Joint0.Position.z + (Interpolation * (Joint1.Position.z - Joint0.Position.z));

			XMVECTOR Joint0Orient = XMVectorSet(Joint0.Orientation.x, Joint0.Orientation.y, Joint0.Orientation.z, Joint0.Orientation.w);
			XMVECTOR Joint1Orient = XMVectorSet(Joint1.Orientation.x, Joint1.Orientation.y, Joint1.Orientation.z, Joint1.Orientation.w);

			XMStoreFloat4(&TempJoint.Orientation, XMQuaternionSlerp(Joint0Orient, Joint1Orient, Interpolation));	// 구면 선형 보간

			InterpolatedJoints[i] = TempJoint;		// 결과 값을 저장한다!
		}

		// 정점 정보 갱신
		for (int j = 0; j < numTVertices; j++)
		{
			VERTEX_MD5	TempVert = TVertices[j];
			memset(&TempVert.Position, 0, sizeof(TempVert.Position));	// Position 값을 0으로 초기화!
			memset(&TempVert.Normal, 0, sizeof(TempVert.Normal));		// Normal 값을 0으로 초기화!

			int WeightStart	= (int)TVertices[j].WeightStart;
			int nWeights	= (int)TVertices[j].WeightNum;

			float ResultX = 0.0f;
			float ResultY = 0.0f;
			float ResultZ = 0.0f;

			for (int k = 0; k < nWeights; k++)
			{
				Weight_MD5	TempWeight	= TWeights[WeightStart+k];
				Joint_MD5	TempJoint	= InterpolatedJoints[TempWeight.JointID];	// 보간 적용 ★
				
				XMVECTOR	QUAT, POS;
				XMFLOAT3	Rotated;

				QUAT = XMVectorSet(TempJoint.Orientation.x, TempJoint.Orientation.y, TempJoint.Orientation.z, TempJoint.Orientation.w);
				POS	= XMVectorSet(TempWeight.Position.x, TempWeight.Position.y, TempWeight.Position.z, 0.0f);
				XMStoreFloat3(&Rotated, XMQuaternionMultiply(XMQuaternionMultiply(QUAT, POS), XMQuaternionInverse(QUAT)));

				TempVert.Position.x += ( TempJoint.Position.x + Rotated.x ) * TempWeight.Bias;
				TempVert.Position.y += ( TempJoint.Position.y + Rotated.y ) * TempWeight.Bias;
				TempVert.Position.z += ( TempJoint.Position.z + Rotated.z ) * TempWeight.Bias;

				XMVECTOR TempWeightNormal = XMVectorSet(TempWeight.Normal.x, TempWeight.Normal.y, TempWeight.Normal.z, 0.0f);
				XMStoreFloat3(&Rotated, XMQuaternionMultiply(XMQuaternionMultiply(QUAT, TempWeightNormal), XMQuaternionInverse(QUAT)));

				TempVert.Normal.x -= Rotated.x * TempWeight.Bias;
				TempVert.Normal.y -= Rotated.y * TempWeight.Bias;
				TempVert.Normal.z -= Rotated.z * TempWeight.Bias;
			}

			TVertices[j].Position = TempVert.Position;
			TVertices[j].Normal = TempVert.Normal;		// Store the vertices normal
			XMStoreFloat3(&TVertices[j].Normal, XMVector3Normalize(XMLoadFloat3(&TVertices[j].Normal)));
		}
		SAFE_DELETEARRAY(InterpolatedJoints);


		// 바운딩 박스 갱신
		BoundingBox_MD5	TempBB;
		TempBB.Min = TBB.Max;
		TempBB.Max = TBB.Min;

		for (int j = 0; j < numTVertices; j++)
		{
			if ( TempBB.Min.x > TVertices[j].Position.x)
				TempBB.Min.x = TVertices[j].Position.x;
			if ( TempBB.Min.y > TVertices[j].Position.y)
				TempBB.Min.y = TVertices[j].Position.y;
			if ( TempBB.Min.z > TVertices[j].Position.z)
				TempBB.Min.z = TVertices[j].Position.z;

			if ( TempBB.Max.x < TVertices[j].Position.x)
				TempBB.Max.x = TVertices[j].Position.x;
			if ( TempBB.Max.y < TVertices[j].Position.y)
				TempBB.Max.y = TVertices[j].Position.y;
			if ( TempBB.Max.z < TVertices[j].Position.z)
				TempBB.Max.z = TVertices[j].Position.z;
		}

		TBB_Animed = TempBB;
	}

	return;
}

void ModelMD5::DrawNonHLSL(int InstanceID, D3DXMATRIX matView, D3DXMATRIX matProjection)
{
	// HLSL 미사용
	UpdateModel();

	pDevice->SetTransform(D3DTS_WORLD, &matModelWorld[InstanceID]);

	// 재질을 설정한다.
	D3DMATERIAL9 mtrl;
	ZeroMemory( &mtrl, sizeof( D3DMATERIAL9 ) );
		mtrl.Diffuse.r = mtrl.Ambient.r = 1.0f;
		mtrl.Diffuse.g = mtrl.Ambient.g = 1.0f;
		mtrl.Diffuse.b = mtrl.Ambient.b = 1.0f;
		mtrl.Diffuse.a = mtrl.Ambient.a = 1.0f;
	pDevice->SetMaterial( &mtrl );

	pDevice->SetTexture(0, ModelTextures[0]);	// 일단 0번으로 통일해서 그리자 ★★
	pDevice->SetStreamSource(0, pModelVB, 0, sizeof(VERTEX_MD5));
	pDevice->SetFVF(D3DFVF_VERTEX_MD5);
	pDevice->SetIndices(pModelIB);

	pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numTVertices, 0, numTIndices);

	for (int i = 0; i < numInstances; i++)
	{
		ModelInstances[i].BeingAnimated = false;	// 애니메이션 상태 초기화
	}

	return;
}

void ModelMD5::DrawHLSL(int InstanceID, D3DXMATRIX matView, D3DXMATRIX matProjection)
{
	// HLSL 사용
	UpdateModel();

	// 모델 행렬 지정!★
	D3DXMATRIX matWVP = matModelWorld[InstanceID] * matView * matProjection;
	pHLSL->SetMatrix("matWVP", &matWVP);
	
	pDevice->SetStreamSource( 0, pModelVB, 0, sizeof( VERTEX_MD5 ) );
	pDevice->SetFVF( D3DFVF_VERTEX_MD5 );
	pDevice->SetIndices( pModelIB );

	pHLSL->SetTechnique("HLSLNoInstancing");

	UINT numPasses = 0;
	pHLSL->Begin(&numPasses, NULL);

		for (UINT i = 0; i < numPasses; ++i)
		{
			pHLSL->BeginPass(i);

			pHLSL->SetTexture("DiffuseMap_Tex", ModelTextures[0]);	// 일단 0번으로 통일해서 그리자 ★★
			pHLSL->CommitChanges();

			pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numTVertices, 0, numTIndices);

			pHLSL->EndPass();
		}

	pHLSL->End();

	for (int i = 0; i < numInstances; i++)
	{
		ModelInstances[i].BeingAnimated = false;	// 애니메이션 상태 초기화
	}

	return;
}

void ModelMD5::DrawHLSLInstancing(D3DXMATRIX matView, D3DXMATRIX matProjection)
{
	// HLSL 사용
	UpdateModel();

	// 모델 행렬 지정!★
	D3DXMATRIX matVP = matView * matProjection;
	pHLSL->SetMatrix("matVP", &matVP);

	pDevice->SetVertexDeclaration( g_pVBDeclaration );

	pDevice->SetStreamSource( 0, pModelVB, 0, sizeof( VERTEX_MD5 ) );
	pDevice->SetStreamSourceFreq( 0, D3DSTREAMSOURCE_INDEXEDDATA | numInstances );

	pDevice->SetStreamSource( 1, pModelInstanceVB, 0, sizeof( INSTANCE_DATA_MD5 ) );
	pDevice->SetStreamSourceFreq( 1, D3DSTREAMSOURCE_INSTANCEDATA | 1ul );

	pDevice->SetIndices( pModelIB );

	pHLSL->SetTechnique("HLSLMD5Instancing");

	UINT numPasses = 0;
	pHLSL->Begin(&numPasses, NULL);

		for (UINT i = 0; i < numPasses; ++i)
		{
			pHLSL->BeginPass(i);

			pHLSL->SetTexture("DiffuseMap_Tex", ModelTextures[0]);	// 일단 0번으로 통일해서 그리자 ★★
			pHLSL->CommitChanges();

			pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numTVertices, 0, numTIndices);

			pHLSL->EndPass();
		}

	pHLSL->End();

	pDevice->SetStreamSourceFreq( 0, 1 );
	pDevice->SetStreamSourceFreq( 1, 1 );

	for (int i = 0; i < numInstances; i++)
	{
		ModelInstances[i].BeingAnimated = false;	// 애니메이션 상태 초기화
	}

	return;
}

void ModelMD5::UpdateModel()
{
	int SizeOfVertices = sizeof(VERTEX_MD5)*numTVertices;
	int SizeOfIndices = sizeof(INDEX_MD5)*numTIndices;

	VOID* pVertices;
	if (FAILED(pModelVB->Lock(0, SizeOfVertices, (void**)&pVertices, D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD)))
		return;
	memcpy(pVertices, TVertices, SizeOfVertices);
	pModelVB->Unlock();

	VOID* pIndices;
	if (FAILED(pModelIB->Lock(0, SizeOfIndices, (void **)&pIndices, D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD)))
		return;
	memcpy(pIndices, TIndices, SizeOfIndices);
	pModelIB->Unlock();


	if (HLSLInstancing)
	{
		// HLSL 인스턴싱 사용
		UpdateInstanceBuffer();
	}
	else
	{
		
	}

	return;
}

void ModelMD5::UpdateInstanceBuffer()
{
	INSTANCE_DATA_MD5* pIDATA;
	HRESULT hr = pModelInstanceVB->Lock( 0, NULL, ( void** )&pIDATA, 0 );

	if( SUCCEEDED( hr ) )
	{
		for (int i = 0; i < numInstances; i++)
		{
			INSTANCE_DATA_MD5 InstanceModel;
			memset(&InstanceModel, 0, sizeof(InstanceModel));

			// 인스턴스 정보 대입
			InstanceModel.matModelWorld[0].x = matModelWorld[i]._11;
			InstanceModel.matModelWorld[0].y = matModelWorld[i]._12;
			InstanceModel.matModelWorld[0].z = matModelWorld[i]._13;
			InstanceModel.matModelWorld[0].w = matModelWorld[i]._14;

			InstanceModel.matModelWorld[1].x = matModelWorld[i]._21;
			InstanceModel.matModelWorld[1].y = matModelWorld[i]._22;
			InstanceModel.matModelWorld[1].z = matModelWorld[i]._23;
			InstanceModel.matModelWorld[1].w = matModelWorld[i]._24;

			InstanceModel.matModelWorld[2].x = matModelWorld[i]._31;
			InstanceModel.matModelWorld[2].y = matModelWorld[i]._32;
			InstanceModel.matModelWorld[2].z = matModelWorld[i]._33;
			InstanceModel.matModelWorld[2].w = matModelWorld[i]._34;

			InstanceModel.matModelWorld[3].x = matModelWorld[i]._41;
			InstanceModel.matModelWorld[3].y = matModelWorld[i]._42;
			InstanceModel.matModelWorld[3].z = matModelWorld[i]._43;
			InstanceModel.matModelWorld[3].w = matModelWorld[i]._44;

			switch (ModelInstances[i].BeingAnimated)
			{
			case true:
				InstanceModel.bAnimated = 1; // 0이면 정지, 1이면 애니메이션 ★
				InstanceModel.InstanceAnimtationID = (float)ModelInstances[i].CurAnimID;
				InstanceModel.CurFrameTime = (float)ModelInstances[i].CurFrameTime;
				InstanceModel.Frame0 = (float)ModelInstances[i].Frame0;
				InstanceModel.Frame1 = (float)ModelInstances[i].Frame1;
				break;
			case false:
				InstanceModel.bAnimated = 0; // 0이면 정지, 1이면 애니메이션 ★
				InstanceModel.InstanceAnimtationID = 0;
				InstanceModel.CurFrameTime = 0;
				InstanceModel.Frame0 = 0;
				InstanceModel.Frame1 = 0;
				break;
			}

			*pIDATA = InstanceModel, pIDATA++;
		}

		pModelInstanceVB->Unlock();
	}
}


void ModelMD5::UpdateModelBoundingBox()
{
	XMFLOAT3 Min = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 Max = XMFLOAT3(0.0f, 0.0f, 0.0f);

	Min = TVertices[0].Position;
	Max = TVertices[0].Position;

	for (int j = 0; j < numTVertices; j++)
	{
		Min.x = min(Min.x, TVertices[j].Position.x);
		Min.y = min(Min.y, TVertices[j].Position.y);
		Min.z = min(Min.z, TVertices[j].Position.z);

		Max.x = max(Max.x, TVertices[j].Position.x);
		Max.y = max(Max.y, TVertices[j].Position.y);
		Max.z = max(Max.z, TVertices[j].Position.z);
	}

	TBB.Min = Min;
	TBB.Max = Max;
	return;
}

void ModelMD5::DrawBoundingBoxes()
{
	UpdateModelBoundingBox();

	XMFLOAT3	BaseBoundingBoxMax;
	XMFLOAT3	BaseBoundingBoxMin;

	if (ModelInstances[0].BeingAnimated == false)
	{
		BaseBoundingBoxMax = TBB.Max;
		BaseBoundingBoxMin = TBB.Min;
	}
	else
	{
		BaseBoundingBoxMax = TBB_Animed.Max;
		BaseBoundingBoxMin = TBB_Animed.Min;
	}

	int numVertices = 8;	// 상자니까 점 8개 필요
	VERTEX_MD5_BB *NewVertices = new VERTEX_MD5_BB[numVertices];

		float XLen = BaseBoundingBoxMax.x - BaseBoundingBoxMin.x;
		float YLen = BaseBoundingBoxMax.y - BaseBoundingBoxMin.y;
		float ZLen = BaseBoundingBoxMax.z - BaseBoundingBoxMin.z;

		for (int i = 0; i < 8; i++)
		{
			NewVertices[i].Color = D3DCOLOR_RGBA(255, 0, 255, 255);
		}

		NewVertices[0].Position = BaseBoundingBoxMin;
		NewVertices[1].Position = BaseBoundingBoxMin;
			NewVertices[1].Position.x += XLen;
		NewVertices[2].Position = BaseBoundingBoxMin;
			NewVertices[2].Position.x += XLen;
			NewVertices[2].Position.z += ZLen;
		NewVertices[3].Position = BaseBoundingBoxMin;
			NewVertices[3].Position.z += ZLen;
		NewVertices[4].Position = BaseBoundingBoxMin;
			NewVertices[4].Position.y += YLen;
		NewVertices[5].Position = BaseBoundingBoxMin;
			NewVertices[5].Position.y += YLen;
			NewVertices[5].Position.x += XLen;
		NewVertices[6].Position = BaseBoundingBoxMin;
			NewVertices[6].Position.y += YLen;
			NewVertices[6].Position.x += XLen;
			NewVertices[6].Position.z += ZLen;
		NewVertices[7].Position = BaseBoundingBoxMin;
			NewVertices[7].Position.y += YLen;
			NewVertices[7].Position.z += ZLen;

		int SizeOfVertices = sizeof(VERTEX_MD5_BB)*numVertices;
	
		VOID* pVertices;
		if (FAILED(pBBVB->Lock(0, SizeOfVertices, (void**)&pVertices, D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD)))
			return;
		memcpy(pVertices, NewVertices, SizeOfVertices);
		pBBVB->Unlock();

	SAFE_DELETEARRAY(NewVertices);


	int numIndices = 12;	// 상자니까 모서리 12개 필요
	INDEX_MD5_BB *NewIndices = new INDEX_MD5_BB[numIndices];

		NewIndices[0]._0 = 0, NewIndices[0]._1 = 1;	// 바닥
		NewIndices[1]._0 = 1, NewIndices[1]._1 = 2;
		NewIndices[2]._0 = 2, NewIndices[2]._1 = 3;
		NewIndices[3]._0 = 3, NewIndices[3]._1 = 0;
		NewIndices[4]._0 = 0, NewIndices[4]._1 = 4;	// 기둥
		NewIndices[5]._0 = 1, NewIndices[5]._1 = 5;
		NewIndices[6]._0 = 2, NewIndices[6]._1 = 6;
		NewIndices[7]._0 = 3, NewIndices[7]._1 = 7;
		NewIndices[8]._0 = 4, NewIndices[8]._1 = 5;	// 천장
		NewIndices[9]._0 = 5, NewIndices[9]._1 = 6;
		NewIndices[10]._0 = 6, NewIndices[10]._1 = 7;
		NewIndices[11]._0 = 7, NewIndices[11]._1 = 4;

		int SizeOfIndices = sizeof(INDEX_MD5_BB)*numIndices;

		VOID* pIndices;
		if (FAILED(pBBIB->Lock(0, SizeOfIndices, (void **)&pIndices, D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD)))
			return;
		memcpy(pIndices, NewIndices, SizeOfIndices);
		pBBIB->Unlock();
	
	SAFE_DELETEARRAY(NewIndices);


	if (pHLSL == NULL)
	{
		// HLSL 미사용
		pDevice->SetTexture(0, 0);
		pDevice->SetStreamSource(0, pBBVB, 0, sizeof(VERTEX_MD5_BB));
		pDevice->SetFVF(D3DFVF_VERTEX_MD5_BB);
		pDevice->SetIndices(pBBIB);

		pDevice->DrawIndexedPrimitive(D3DPT_LINELIST, 0, 0, numVertices, 0, numIndices);
	}
	else
	{
		// HLSL 사용
		pDevice->SetVertexDeclaration( g_pVBDeclaration );

		pDevice->SetStreamSource( 0, pBBVB, 0, sizeof( VERTEX_MD5_BB ) );
		pDevice->SetStreamSourceFreq( 0, D3DSTREAMSOURCE_INDEXEDDATA | numInstances );

		pDevice->SetStreamSource( 1, pModelInstanceVB, 0, sizeof( INSTANCE_DATA_MD5 ) );
		pDevice->SetStreamSourceFreq( 1, D3DSTREAMSOURCE_INSTANCEDATA | 1ul );

		pDevice->SetIndices( pBBIB );

		pHLSL->SetTechnique("HLSLNoInstancing");

		UINT numPasses = 0;
		pHLSL->Begin(&numPasses, NULL);

			for (UINT i = 0; i < numPasses; ++i)
			{
				pHLSL->BeginPass(i);

				pHLSL->CommitChanges();

				pDevice->DrawIndexedPrimitive(D3DPT_LINELIST, 0, 0, numVertices, 0, numIndices);

				pHLSL->EndPass();
			}

		pHLSL->End();

		pDevice->SetStreamSourceFreq( 0, 1 );
		pDevice->SetStreamSourceFreq( 1, 1 );
	}

	return;
}

HRESULT ModelMD5::DrawNormalVecters(float LenFactor)
{
	int numVertices = numTVertices * 2;
	int numIndices = numVertices / 2;

	// 정점 버퍼 업데이트!
	VERTEX_MD5_NORMAL *NewVertices = new VERTEX_MD5_NORMAL[numVertices];

		for (int j = 0; j < numVertices; j+=2)
		{
			NewVertices[j].Normal.x = TVertices[j/2].Position.x;
			NewVertices[j].Normal.y = TVertices[j/2].Position.y;
			NewVertices[j].Normal.z = TVertices[j/2].Position.z;
			NewVertices[j+1].Normal.x = NewVertices[j].Normal.x + TVertices[j/2].Normal.x * LenFactor;
			NewVertices[j+1].Normal.y = NewVertices[j].Normal.y + TVertices[j/2].Normal.y * LenFactor;
			NewVertices[j+1].Normal.z = NewVertices[j].Normal.z + TVertices[j/2].Normal.z * LenFactor;

			NewVertices[j].Color = D3DCOLOR_RGBA(0, 255, 255, 255);
			NewVertices[j+1].Color = D3DCOLOR_RGBA(0, 255, 255, 255);
		}

		int SizeOfVertices = sizeof(VERTEX_MD5_NORMAL)*numVertices;

		VOID* pVertices;
		if (FAILED(pNVVB->Lock(0, SizeOfVertices, (void**)&pVertices, D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD)))
			return E_FAIL;
		memcpy(pVertices, NewVertices, SizeOfVertices);
		pNVVB->Unlock();

	SAFE_DELETEARRAY(NewVertices);


	// 색인 버퍼 업데이트!
	INDEX_MD5_NORMAL *NewIndices = new INDEX_MD5_NORMAL[numIndices];
	int k = 0;

		for (int i = 0; i < numIndices; i++)
		{
			NewIndices[i]._0 = k++;
			NewIndices[i]._1 = k++;
		}

		int SizeOfIndices = sizeof(INDEX_MD5_NORMAL)*numIndices;

		VOID* pIndices;
		if (FAILED(pNVIB->Lock(0, SizeOfIndices, (void **)&pIndices, D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD)))
			return E_FAIL;
		memcpy(pIndices, NewIndices, SizeOfIndices);
		pNVIB->Unlock();
	
	SAFE_DELETEARRAY(NewIndices);

	pDevice->SetTexture(0, 0);
	pDevice->SetStreamSource(0, pNVVB, 0, sizeof(VERTEX_MD5_NORMAL));
	pDevice->SetFVF(D3DFVF_VERTEX_MD5_NORMAL);
	pDevice->SetIndices(pNVIB);

	pDevice->DrawIndexedPrimitive(D3DPT_LINELIST, 0, 0, numVertices, 0, numIndices);

	return S_OK;	// 함수 종료!
}

PickingRay ModelMD5::GetPickingRay(int MouseX, int MouseY, int ScreenWidth, int ScreenHeight, D3DXMATRIX matView, D3DXMATRIX matProj)
{
	if (MouseX < 0 || MouseY < 0 || MouseX > ScreenWidth || MouseY > ScreenHeight)
		return PickingRay(D3DXVECTOR3(0,0,0), D3DXVECTOR3(9999.0f,0,0));

	D3DVIEWPORT9 vp;
	D3DXMATRIX InvView;

	D3DXVECTOR3 MouseViewPortXY, PickingRayDir, PickingRayPos;

	pDevice->GetViewport(&vp);
	D3DXMatrixInverse(&InvView, NULL, &matView);

	MouseViewPortXY.x = (( (((MouseX-vp.X)*2.0f/vp.Width ) - 1.0f)) - matProj._31 ) / matProj._11;
	MouseViewPortXY.y = ((- (((MouseY-vp.Y)*2.0f/vp.Height) - 1.0f)) - matProj._32 ) / matProj._22;
	MouseViewPortXY.z = 1.0f;

	PickingRayDir.x = MouseViewPortXY.x*InvView._11 + MouseViewPortXY.y*InvView._21 + MouseViewPortXY.z*InvView._31;
	PickingRayDir.y = MouseViewPortXY.x*InvView._12 + MouseViewPortXY.y*InvView._22 + MouseViewPortXY.z*InvView._32;
	PickingRayDir.z = MouseViewPortXY.x*InvView._13 + MouseViewPortXY.y*InvView._23 + MouseViewPortXY.z*InvView._33;
	D3DXVec3Normalize(&PickingRayDir, &PickingRayDir);

	PickingRayPos.x = InvView._41;
	PickingRayPos.y = InvView._42;
	PickingRayPos.z = InvView._43;

	return PickingRay(PickingRayPos, PickingRayDir);
}

bool ModelMD5::CheckMouseOverPerInstance(int InstanceID, int MouseX, int MouseY, int ScreenWidth, int ScreenHeight,
	D3DXMATRIX matView, D3DXMATRIX matProj)
{
	PickingRay PR = GetPickingRay(MouseX, MouseY, ScreenWidth, ScreenHeight, matView, matProj);

	if (PR.Dir.x == 9999.0f)
		return false;

	DistanceCmp[InstanceID]				= FLT_MAX;
	MouseOverPerInstances[InstanceID]	= false;
	PickedPosition[InstanceID]			= XMFLOAT3(0, 0, 0);

	for (int i = 0; i < numTIndices; i++)
	{
		int ID0 = TIndices[i]._0;
		int ID1 = TIndices[i]._1;
		int ID2 = TIndices[i]._2;

		D3DXVECTOR3 p0, p1, p2;
		p0 = D3DXVECTOR3(TVertices[ID0].Position.x, TVertices[ID0].Position.y, TVertices[ID0].Position.z);
		p1 = D3DXVECTOR3(TVertices[ID1].Position.x, TVertices[ID1].Position.y, TVertices[ID1].Position.z);
		p2 = D3DXVECTOR3(TVertices[ID2].Position.x, TVertices[ID2].Position.y, TVertices[ID2].Position.z);

		// 인스턴스
		D3DXVec3TransformCoord(&p0, &p0, &matModelWorld[InstanceID]);
		D3DXVec3TransformCoord(&p1, &p1, &matModelWorld[InstanceID]);
		D3DXVec3TransformCoord(&p2, &p2, &matModelWorld[InstanceID]);

		float pU, pV, pDist;

		if (D3DXIntersectTri(&p0, &p1, &p2, &PR.Pos, &PR.Dir, &pU, &pV, &pDist))
		{
			if (pDist < DistanceCmp[InstanceID])
			{
				DistanceCmp[InstanceID] = pDist;
				D3DXVECTOR3 TempPosition = p0 + (p1*pU - p0*pU) + (p2*pV - p0*pV);
				D3DXVec3TransformCoord(&TempPosition, &TempPosition, &matModelWorld[InstanceID]);

				PickedPosition[InstanceID].x = TempPosition.x;
				PickedPosition[InstanceID].y = TempPosition.y;
				PickedPosition[InstanceID].z = TempPosition.z;
			}

			MouseOverPerInstances[InstanceID] = true;
		}
	}

	switch (MouseOverPerInstances[InstanceID])
	{
		case true:
			return true;
		case false:
			return false;
	}

	return false;
}

void ModelMD5::CheckMouseOverFinal()
{
	float	ClosestDistance		= FLT_MAX;
	int		ClosestInstanceID	= -1;

	for (int i = 0; i < numInstances; i++)
	{
		if (MouseOverPerInstances[i] == true)
		{
			if ( DistanceCmp[i] < ClosestDistance )
			{
				ClosestDistance = DistanceCmp[i];
				ClosestInstanceID = i;
			}
		}
	}

	for (int i = 0; i < numInstances; i++)
	{
		MouseOverPerInstances[i] = false;
	}

	if (ClosestInstanceID != -1)
		MouseOverPerInstances[ClosestInstanceID] = true;
}