#include "ModelMD5.h"

KeyWords	MD5KeyWords[] =
{
	"MD5Version"	,	// 0
	"commandline"	,	// 1
	"numJoints"		,	// 2
	"numMeshes"		,	// 3
	"numObjects"	,	// 4
	"numMaterials"	,	// 5
	"joints"		,	// 6
	"objects"		,	// 7
	"materials"		,	// 8
	"mesh"			,	// 9
	"//"			,	// 10
	"meshindex"		,	// 11
	"shader"		,	// 12
	"numverts"		,	// 13
	"vert"			,	// 14
	"numtris"		,	// 15
	"tri"			,	// 16
	"numweights"	,	// 17
	"weight"		,	// 18
};

struct MD5Joint
{
	AnyName		Name;
	int			ParentIndex;
	D3DVECTOR	Position;
	D3DVECTOR4	Orientation;
};

struct MD5Object
{
	AnyName		Name;
	int			Index;				// 아마??★
	int			numMeshes;			// 아마??★
};

struct MD5Weight
{
	int			JointIndex;
	float		Bias;
	D3DVECTOR	Position;
};

struct MD5Material
{
	AnyName		Name;
	AnyName		TextureFileName;
};

struct MD5Mesh
{
	AnyName			MaterialName;

	CUSTOMVERTEX	Vertices[MAX_VERTICES];
	int				VertexWeightStart[MAX_VERTICES];
	int				numVertexWeights[MAX_VERTICES];

	CUSTOMINDEX		Indices[MAX_INDICES];

	MD5Weight		Weights[MAX_WEIGHTS];
};

MD5Joint		ModelJoints[MAX_JOINTS];
MD5Object		ModelObjects[MAX_OBJECTS];
MD5Material		ModelMaterials[MAX_MATERIALS];
MD5Mesh			ModelMeshes[MAX_MESHES];


// 함수 원형 선언!
void	StringTrim(char* sBuf);
char*	GetName(char* val);
char*	GetNameBetweenBraces(char* val);
char*	GetNameBetweenQuotes(char* val);
bool	FindString(char* val, char* cmp);
bool	FindChar(char* val, char cmp[]);
//void	GetFloatFromLine(char* line, char split, char splita);
void	GetFloatFromLine(char* line, char* split, char* splita);
	float	ParseFloats[MAX_PARSE_LINE];

void StringTrim(char* sBuf)
{
	int iLen = 0;
	int	i=0;
	int iCnt=0;

	iLen = strlen(sBuf);

	if(iLen < 1)
		return;

	// 우측의 공백 제거
	iCnt = 0;
	iLen = strlen(sBuf);

	for(i=iLen-1; i>=0; --i)
	{
		char* p = sBuf + i;

		if( ' ' == *p || '\t' == *p)
			continue;

		*(sBuf + i+1) = '\0';
		break;
	}
	iLen = i+1 +1;

	// 좌측의 공백제거
	char sT[MAX_PARSE_LINE] = {0};
	strncpy(sT, sBuf, iLen);

	for(i=0; i < iLen; ++i)
	{
		char* p = sT + i;

		if( ' ' == *p || '\t' == *p)
			continue;

		break;
	}

	strcpy(sBuf, sT+i);	// 결과 반영
}

char* SplitString(char* String, char* SplitChar, int SplitIndex)
{
	int iLen = strlen(String);

	char tempdst[MAX_PARSE_LINE];
	memset(tempdst, 0, sizeof(tempdst));

	int previ = 0;
	int splitcount = 0;
	
	for (int i = 1; i < iLen; i++)
	{
		if (String[i] == SplitChar[0])
		{
			if (splitcount == SplitIndex)
			{
				for (int j = previ+1; j < i; j++)
				{
					tempdst[j-previ-1] = String[j];
				}
				break;
			}

			splitcount++;
			previ = i;
		}
	}

	return tempdst;
}

char* GetName(char* val)
{
	int iLen = strlen(val);
	char temp[MAX_PARSE_LINE] = {0};
	int firstspace = 0;
	int secondspace = 0;

	for (int i = 0; i < iLen; i++)
	{
		if (val[i] == ' ')
		{
			if (firstspace == 0)
			{
				firstspace = i+1;
			}
			else
			{
				secondspace = i-1;
				break;
			}
		}
	}

	for (int i = firstspace; i <= secondspace; i++)
	{
		temp[i-firstspace] = val[i];
	}

	return temp;
}

char* GetNameBetweenBraces(char* val)
{
	int iLen = strlen(val);
	char temp[MAX_PARSE_LINE] = {0};
	int firstspace = 0;
	int secondspace = 0;

	for (int i = 0; i < iLen; i++)
	{
		if (val[i] == '{')
		{
			firstspace = i+1;
		}
		if (val[i] == '}')
		{
			secondspace = i-1;
			break;
		}
	}

	for (int i = firstspace; i <= secondspace; i++)
	{
		temp[i-firstspace] = val[i];
	}

	StringTrim(temp);

	return temp;
}

char* GetNameBetweenQuotes(char* val)
{
	int iLen = strlen(val);
	char temp[MAX_PARSE_LINE] = {0};
	int firstspace = 0;
	int secondspace = 0;

	for (int i = 0; i < iLen; i++)
	{
		if (val[i] == '\"')
		{
			if (firstspace == 0)
			{
				firstspace = i+1;
			}
			else
			{
				secondspace = i-1;
				break;
			}
		}
	}

	for (int i = firstspace; i <= secondspace; i++)
	{
		temp[i-firstspace] = val[i];
	}

	StringTrim(temp);

	return temp;
}

bool FindString(char* val, char* cmp)
{
	return (0 == _strnicmp(val, cmp, strlen(cmp)) ) ? 1: 0;
}

bool FindChar(char* val, char cmp[])
{
	char temp[MAX_PARSE_LINE];
	char temp2;
	strcpy(temp, val);
	temp2 = cmp[0];

	for (int i = 0; i <= MAX_PARSE_LINE; i++)
	{
		if (temp[i] == temp2)
			return true;
	}

	return false;
}

void GetFloatFromLine(char* line, char* split, char* splita)
{
	char temp[MAX_PARSE_LINE] = {0};
	strcpy(temp, line);
	strcat(temp, split);	// 마지막에 나오는 값도 얻어올 수 있도록 split문자 마지막에 하나 추가
	int iLen = strlen(temp);

	memset(ParseFloats, 0, sizeof(ParseFloats));

	int previ = 0;
	int iNum = 0;

	for (int i = 0; i < iLen; i++)
	{
		char parsetemp[MAX_PARSE_LINE] = {0};
		if ((temp[i] == split[0]) || (temp[i] == splita[0]))
		{
			if (previ >= i)	// split 문자가 연속으로 나옴. 즉, 값이 없음!
				continue;

			for (int j = previ; j < i; j++)
			{
				parsetemp[j-previ] = temp[j];
			}
			iNum++;
			ParseFloats[iNum-1] = atof(parsetemp);
			previ = i+1;
		}
	}
}

D3DVECTOR4 QuaternionMultiply(D3DVECTOR4 Q1, D3DVECTOR4 Q2)
{
	D3DVECTOR4 Result;
	
	Result.x = (Q2.w * Q1.x) + (Q2.x * Q1.w) + (Q2.y * Q1.z) - (Q2.z * Q1.y);
	Result.y = (Q2.w * Q1.y) - (Q2.x * Q1.z) + (Q2.y * Q1.w) + (Q2.z * Q1.x);
	Result.z = (Q2.w * Q1.z) + (Q2.x * Q1.y) - (Q2.y * Q1.x) + (Q2.z * Q1.w);
	Result.w = (Q2.w * Q1.w) - (Q2.x * Q1.x) - (Q2.y * Q1.y) - (Q2.z * Q1.z);

	return Result;
}

bool ModelMD5::OpenModelFromFile(char* FileName)
{
	FILE*	fp;						// 불러올 X 파일
	char	sLine[MAX_PARSE_LINE];		// 파일에서 읽어올 한 줄

	if(!(fp = fopen(FileName, "rt")))
		return false;

	memset(ModelJoints, 0, sizeof(MD5Joint)*MAX_JOINTS);

	MD5Version = 0;
	numJoints = 0;
	numMeshes = 0;
	numObjects = 0;
	numMaterials = 0;

	char	CurKey[MAX_KEYWORD_LEN] = {0};
	int		CurKeyCount = 0;
	int		MeshCount = 0;

	while(!feof(fp))
	{
		fgets(sLine, MAX_PARSE_LINE, fp);
		StringTrim(sLine);
		int iLen = strlen(sLine);

		if (iLen <= 1)	// 지금 빈 줄이면 다음 줄 읽자! (개행문자 때문에 최소 길이가 1임)★★
			continue;

		// 중괄호 열린 상태
		if ( FindString(CurKey, MD5KeyWords[6]) )	// joints {
		{
			CurKeyCount++;
			strcpy( ModelJoints[CurKeyCount-1].Name, GetNameBetweenQuotes(sLine) );
			GetFloatFromLine(sLine, " ", " ");

			ModelJoints[CurKeyCount-1].ParentIndex = (int)ParseFloats[1];
			ModelJoints[CurKeyCount-1].Position.x = ParseFloats[3];
			ModelJoints[CurKeyCount-1].Position.y = ParseFloats[5];		// DirectX니까 Y축과 Z축 교환!★★
			ModelJoints[CurKeyCount-1].Position.z = ParseFloats[4];
			ModelJoints[CurKeyCount-1].Orientation.x = ParseFloats[8];
			ModelJoints[CurKeyCount-1].Orientation.y = ParseFloats[10];	// DirectX니까 Y축과 Z축 교환!★★
			ModelJoints[CurKeyCount-1].Orientation.z = ParseFloats[9];

			// 사원수(Quaternion) w값 생성
			float t = 1.0f - ( ModelJoints[CurKeyCount-1].Orientation.x * ModelJoints[CurKeyCount-1].Orientation.x )
				- ( ModelJoints[CurKeyCount-1].Orientation.y * ModelJoints[CurKeyCount-1].Orientation.y )
				- ( ModelJoints[CurKeyCount-1].Orientation.z * ModelJoints[CurKeyCount-1].Orientation.z );
			if ( t < 0.0f )
			{
				ModelJoints[CurKeyCount-1].Orientation.w = 0.0f;
			}
			else
			{
				ModelJoints[CurKeyCount-1].Orientation.w = -sqrtf(t);
			}

			if (CurKeyCount >= numJoints)
				memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기

			continue;
		}
		if ( FindString(CurKey, MD5KeyWords[7]) )	// objects {
		{
			CurKeyCount++;
			strcpy( ModelObjects[CurKeyCount-1].Name, GetNameBetweenQuotes(sLine) );
			GetFloatFromLine(sLine, " ", " ");

			ModelObjects[CurKeyCount-1].Index = (int)ParseFloats[1];
			ModelObjects[CurKeyCount-1].numMeshes = (int)ParseFloats[2];

			if (CurKeyCount >= numObjects)
				memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기

			continue;
		}
		if ( FindString(CurKey, MD5KeyWords[8]) )	// materials {
		{
			CurKeyCount++;
			strcpy( ModelMaterials[CurKeyCount-1].Name, SplitString(sLine, "\"", 0) );
			strcpy( ModelMaterials[CurKeyCount-1].TextureFileName, SplitString(sLine, "\"", 2) );

			if (CurKeyCount >= numMaterials)
				memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기

			continue;
		}

		if ( FindString(CurKey, MD5KeyWords[9]) )	// mesh {
		{
			if ( FindString(sLine, MD5KeyWords[10]) )	// 주석 줄
				continue;								// 건너뛰자

			if ( FindString(sLine, MD5KeyWords[11]) )	// meshindex
			{
				GetFloatFromLine(sLine, " ", " ");
				//ModelMeshes[MeshCount-1].MeshIndex = (int)ParseFloats[1];
				continue;
			}
			if ( FindString(sLine, MD5KeyWords[12]) )	// shader
			{
				strcpy( ModelMeshes[MeshCount-1].MaterialName, SplitString(sLine, "\"", 1) );
				continue;
			}
			if ( FindString(sLine, MD5KeyWords[13]) )	// numverts
			{
				GetFloatFromLine(sLine, " ", " ");
				numMeshVertices[MeshCount-1] = (int)ParseFloats[1];
				continue;
			}
			if ( FindString(sLine, MD5KeyWords[14]) )	// vert
			{
				GetFloatFromLine(sLine, " ", " ");
				int VIndex = (int)ParseFloats[1];
				ModelMeshes[MeshCount-1].Vertices[VIndex].texCoord.x = ParseFloats[3];
				ModelMeshes[MeshCount-1].Vertices[VIndex].texCoord.y = ParseFloats[4];
				ModelMeshes[MeshCount-1].VertexWeightStart[VIndex] = (int)ParseFloats[6];
				ModelMeshes[MeshCount-1].numVertexWeights[VIndex] = (int)ParseFloats[7];
				continue;
			}
			if ( FindString(sLine, MD5KeyWords[15]) )	// numtris
			{
				GetFloatFromLine(sLine, " ", " ");
				numMeshIndices[MeshCount-1] = (int)ParseFloats[1];
				continue;
			}
			if ( FindString(sLine, MD5KeyWords[16]) )	// tri
			{
				GetFloatFromLine(sLine, " ", " ");
				int TIndex = (int)ParseFloats[1];
				ModelMeshes[MeshCount-1].Indices[TIndex]._0 = (int)ParseFloats[2];
				ModelMeshes[MeshCount-1].Indices[TIndex]._1 = (int)ParseFloats[3];
				ModelMeshes[MeshCount-1].Indices[TIndex]._2 = (int)ParseFloats[4];
				continue;
			}
			if ( FindString(sLine, MD5KeyWords[17]) )	// numweights
			{
				GetFloatFromLine(sLine, " ", " ");
				numWeights[MeshCount-1] = (int)ParseFloats[1];
				continue;
			}
			if ( FindString(sLine, MD5KeyWords[18]) )	// weight
			{
				GetFloatFromLine(sLine, " ", " ");
				int WIndex = (int)ParseFloats[1];
				ModelMeshes[MeshCount-1].Weights[WIndex].JointIndex = (int)ParseFloats[2];
				ModelMeshes[MeshCount-1].Weights[WIndex].Bias = ParseFloats[3];
				ModelMeshes[MeshCount-1].Weights[WIndex].Position.x = ParseFloats[5];
				ModelMeshes[MeshCount-1].Weights[WIndex].Position.y = ParseFloats[7];	// DirectX니까 Y축과 Z축을 교환!★
				ModelMeshes[MeshCount-1].Weights[WIndex].Position.z = ParseFloats[6];
				continue;
			}

			if (FindChar(sLine, "}"))
				memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기
			continue;
		}


		// 중괄호 열기 전
		if ( FindString(sLine, MD5KeyWords[0]) )	// MD5Version 4843
		{
			GetFloatFromLine(sLine, " ", " ");
			MD5Version = (int)ParseFloats[1];
			continue;
		}

		if ( FindString(sLine, MD5KeyWords[1]) )	// commandline "~"
		{
			continue;							// 주석은 건너뛰자
		}

		if ( FindString(sLine, MD5KeyWords[2]) )	// numJoints ~
		{
			GetFloatFromLine(sLine, " ", " ");
			numJoints = (int)ParseFloats[1];
			continue;
		}
		if ( FindString(sLine, MD5KeyWords[3]) )	// numMeshes ~
		{
			GetFloatFromLine(sLine, " ", " ");
			numMeshes = (int)ParseFloats[1];
			continue;
		}
		if ( FindString(sLine, MD5KeyWords[4]) )	// numObjects ~
		{
			GetFloatFromLine(sLine, " ", " ");
			numObjects = (int)ParseFloats[1];
			continue;
		}
		if ( FindString(sLine, MD5KeyWords[5]) )	// numMaterials ~
		{
			GetFloatFromLine(sLine, " ", " ");
			numMaterials = (int)ParseFloats[1];
			continue;
		}
		if ( FindString(sLine, MD5KeyWords[6]) )	// joints {
		{
			strcpy(CurKey, MD5KeyWords[6]);				// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}
		if ( FindString(sLine, MD5KeyWords[7]) )	// objects {
		{
			strcpy(CurKey, MD5KeyWords[7]);				// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}
		if ( FindString(sLine, MD5KeyWords[8]) )	// materials {
		{
			strcpy(CurKey, MD5KeyWords[8]);				// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}
		if ( FindString(sLine, MD5KeyWords[9]) )	// mesh {
		{
			MeshCount++;
			strcpy(CurKey, MD5KeyWords[9]);				// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}

	} // WHILE문 종료!

	// 정점 정보 업데이트
	for (int i = 0; i < numMeshes; i++)
	{
		for (int j = 0; j < numMeshVertices[i]; j++)
		{
			int WeightStart = ModelMeshes[i].VertexWeightStart[j];
			int nWeights = ModelMeshes[i].numVertexWeights[j];

			float ResultX = 0.0f;
			float ResultY = 0.0f;
			float ResultZ = 0.0f;

			for (int k = 0; k < nWeights; k++)
			{
				int CurJointID = ModelMeshes[i].Weights[WeightStart+k].JointIndex;
				D3DVECTOR4 Q1, POS, Q2, Rotated;

				Q1.x = ModelJoints[CurJointID].Orientation.x;
				Q1.y = ModelJoints[CurJointID].Orientation.y;
				Q1.z = ModelJoints[CurJointID].Orientation.z;
				Q1.w = ModelJoints[CurJointID].Orientation.w;

				POS.x = ModelMeshes[i].Weights[WeightStart+k].Position.x;
				POS.y = ModelMeshes[i].Weights[WeightStart+k].Position.y;
				POS.z = ModelMeshes[i].Weights[WeightStart+k].Position.z;
				POS.w = 0.0f;

				Q2.x = -Q1.x;
				Q2.y = -Q1.y;
				Q2.z = -Q1.z;
				Q2.w = Q1.w;

				Rotated = QuaternionMultiply( QuaternionMultiply( Q1, POS ), Q2 );

				ResultX += (ModelJoints[CurJointID].Position.x + Rotated.x) * ModelMeshes[i].Weights[WeightStart+k].Bias;
				ResultY += (ModelJoints[CurJointID].Position.y + Rotated.y) * ModelMeshes[i].Weights[WeightStart+k].Bias;
				ResultZ += (ModelJoints[CurJointID].Position.z + Rotated.z) * ModelMeshes[i].Weights[WeightStart+k].Bias;
			}
			
			ModelMeshes[i].Vertices[j].pos.x = ResultX;
			ModelMeshes[i].Vertices[j].pos.y = ResultY;
			ModelMeshes[i].Vertices[j].pos.z = ResultZ;
		}
	}

	fclose(fp);

	return true;
}

bool ModelMD5::SaveParsedFile(char* FileName)
{
	FILE*	fpParsed;				// 파싱된 파일 (.txt로 저장)
	char NewFileName[260] = {0};
	strcpy(NewFileName, FileName);
	char * p = strchr(NewFileName, '.');
	*p = '\0';
	strcat(p, ".txt");

	fpParsed= fopen(NewFileName, "wt");

	//fprintf(fpParsed, "# 재질[%d]: %s ", i, MatNames[i]);

	fclose(fpParsed);

	return true;
}

HRESULT ModelMD5::LoadMeshToDraw(int MeshIndex)
{
	CUSTOMVERTEX *NewVertices = new CUSTOMVERTEX[numMeshVertices[MeshIndex]];

	for (int i = 0; i < numMeshVertices[MeshIndex]; i++)
	{
		NewVertices[i].pos.x = ModelMeshes[MeshIndex].Vertices[i].pos.x;
		NewVertices[i].pos.y = ModelMeshes[MeshIndex].Vertices[i].pos.y;
		NewVertices[i].pos.z = ModelMeshes[MeshIndex].Vertices[i].pos.z;
		NewVertices[i].texCoord.x = ModelMeshes[MeshIndex].Vertices[i].texCoord.x;
		NewVertices[i].texCoord.y = ModelMeshes[MeshIndex].Vertices[i].texCoord.y;
	}

	if (FAILED(g_pd3dDevice->CreateVertexBuffer(numMeshVertices[MeshIndex] * sizeof(CUSTOMVERTEX),
		0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pModelVB, NULL)))
		return E_FAIL;

	int SizeOfVertices = sizeof(CUSTOMVERTEX)*numMeshVertices[MeshIndex];

	VOID* pVertices;
	if (FAILED(g_pModelVB->Lock(0, SizeOfVertices, (void**)&pVertices, 0)))
		return E_FAIL;
	memcpy(pVertices, NewVertices, SizeOfVertices);
	g_pModelVB->Unlock();

	delete[] NewVertices;

	
	CUSTOMINDEX *NewIndices = new CUSTOMINDEX[numMeshIndices[MeshIndex]];

	for (int i = 0; i < numMeshIndices[MeshIndex]; i++)
	{
		NewIndices[i]._0 = ModelMeshes[MeshIndex].Indices[i]._0;
		NewIndices[i]._1 = ModelMeshes[MeshIndex].Indices[i]._1;
		NewIndices[i]._2 = ModelMeshes[MeshIndex].Indices[i]._2;
	}

	int SizeOfIndices = sizeof(CUSTOMINDEX)*numMeshIndices[MeshIndex];

	if (FAILED(g_pd3dDevice->CreateIndexBuffer(SizeOfIndices, 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &g_pModelIB, NULL)))
		return E_FAIL;

	VOID* pIndices;
	if (FAILED(g_pModelIB->Lock(0, SizeOfIndices, (void **)&pIndices, 0)))
		return E_FAIL;
	memcpy(pIndices, NewIndices, SizeOfIndices);
	g_pModelIB->Unlock();
	
	delete[] NewIndices;

	D3DXCreateTextureFromFile(g_pd3dDevice, ModelMaterials[MeshIndex].TextureFileName, &g_pModelTexture);

	return S_OK;
}

D3DVECTOR4 MatrixMultiplyVector(D3DXMATRIX *MatResult, D3DVECTOR4 Vec)
{
	D3DXMATRIX tempmat = *MatResult;
	D3DVECTOR4 VecResult = {0.0f, 0.0f, 0.0f, 0.0f};

	VecResult.x = tempmat._11 * Vec.x + tempmat._21 * Vec.y + tempmat._31 * Vec.z + tempmat._41 * Vec.w;
	VecResult.y = tempmat._12 * Vec.x + tempmat._22 * Vec.y + tempmat._32 * Vec.z + tempmat._42 * Vec.w;
	VecResult.z = tempmat._13 * Vec.x + tempmat._23 * Vec.y + tempmat._33 * Vec.z + tempmat._43 * Vec.w;
	VecResult.w = tempmat._14 * Vec.x + tempmat._24 * Vec.y + tempmat._34 * Vec.z + tempmat._44 * Vec.w;

	return VecResult;
}

D3DXMATRIX RoationMatrixFromQuaternion(D3DXQUATERNION Quat)
{
	D3DXMATRIX tempmat;
	float x = Quat.x;
	float y = Quat.y;
	float z = Quat.z;
	float w = Quat.w;

	tempmat._11 = 1 - 2*(y*y+z*z);	tempmat._12 = 2*(x*y+w*z);		tempmat._13 = 2*(w*y-x*z);		tempmat._14 = 0;
	tempmat._21 = 2*(x*y-w*z);		tempmat._22 = 1 - 2*(x*x+z*z);	tempmat._23 = 2*(y*z+w*x);		tempmat._24 = 0;
	tempmat._31 = 2*(x*z+w*y);		tempmat._32 = 2*(w*x-y*z);		tempmat._33 = 1 - 2*(x*x+y*y);	tempmat._34 = 0;
	tempmat._41 = 0;				tempmat._42 = 0;				tempmat._43 = 0;				tempmat._44 = 1;

	return tempmat;
}

HRESULT ModelMD5::Animate(char* AnimationName)
{
	/*
	if ( nAnimSets == 0 )	// 애니메이션이 없는 파일이면 종료!
		return E_FAIL;

	CUSTOMVERTEX *NewVertices = new CUSTOMVERTEX[nMeshVertices];

	for (int i = 0; i < nMeshVertices; i++)
	{
		NewVertices[i].pos.x = MeshVertices[i].pos.x;
		NewVertices[i].pos.y = MeshVertices[i].pos.y;
		NewVertices[i].pos.z = MeshVertices[i].pos.z;
		NewVertices[i].texCoord.x = MeshVertices[i].texCoord.x;
		NewVertices[i].texCoord.y = MeshVertices[i].texCoord.y;
	}

	int AnimSetIndex = 0;
	int AnimLength = 0;
	int AnimInterval = (AnimTicks / 30);	// 애니메이션 키 간격!★ (보통 160)

	for (int i = 0; i < nAnimSets; i++)
	{
		if ( FindString(AnimSets[i], AnimationName) )
		{
			AnimSetIndex = i;
			break;
		}
	}

	AnimLength = AnimSetsLength[AnimSetIndex];
	CurrentAnimTime++;	// 애니메이션 진행!★

	if ((CurrentAnimTime-1)*AnimInterval > AnimLength)	// 애니메이션이 다 끝나면 다시 처음으로!! (애니메이션 반복★)
	{
		CurrentAnimTime = 0;
		AnimRCount = 0;
		AnimSCount = 0;
		AnimTCount = 0;
	}

	for (int i = 0; i < nAnimedFrames[AnimSetIndex]; i++)	// 애니메이션의 대상이 되는 모든 뼈대(Frame)
	{
		// Animation R - 회전 애니메이션
		if ( AnimKeysR[AnimSetIndex][i][AnimRCount].time == (CurrentAnimTime-1)*AnimInterval)
		{
			D3DXQUATERNION RotQ;
			//RotQ.x = 1.0f; RotQ.y = 0.0f; RotQ.z = 0.0f; RotQ.w = 0.0f;
			
			RotQ.x = AnimKeysR[AnimSetIndex][i][AnimRCount].x;
			RotQ.y = AnimKeysR[AnimSetIndex][i][AnimRCount].y;
			RotQ.z = AnimKeysR[AnimSetIndex][i][AnimRCount].z;
			RotQ.w = -AnimKeysR[AnimSetIndex][i][AnimRCount].w;

			D3DXMatrixRotationQuaternion(&MatAnimRot, &RotQ);
			D3DXMatrixInverse(&MatAnimRot, NULL, &MatAnimRot);

			//D3DXMATRIX temp;
			//D3DXMatrixRotationY(&temp, -D3DX_PI/2);
			//MatAnimRot = MatAnimRot * temp;

			//MatAnimRot._22 = -MatAnimRot._22;
			//MatAnimRot._33 = -MatAnimRot._33;

			AnimRCount++;
		}

		// Animation S - 크기 변화 애니메이션
		if ( AnimKeysS[AnimSetIndex][i][AnimSCount].time == (CurrentAnimTime-1)*AnimInterval)
		{
			D3DXMatrixScaling(&MatAnimScale, AnimKeysS[AnimSetIndex][i][AnimSCount].x,
				AnimKeysS[AnimSetIndex][i][AnimSCount].y, AnimKeysS[AnimSetIndex][i][AnimSCount].z);

			AnimSCount++;
		}

		// Animation T - 이동 애니메이션
		if ( AnimKeysT[AnimSetIndex][i][AnimTCount].time == (CurrentAnimTime-1)*AnimInterval)
		{
			D3DXMatrixTranslation(&MatAnimTrans, AnimKeysT[AnimSetIndex][i][AnimTCount].x,
				AnimKeysT[AnimSetIndex][i][AnimTCount].y, AnimKeysT[AnimSetIndex][i][AnimTCount].z);

			AnimTCount++;
		}

		MatAnimResult = MatAnimRot * MatAnimScale * MatAnimTrans;	// 애니메이션을 적용하기 위한 행렬을 구한다! ★★
		//MatAnimResult = MatAnimScale * MatAnimTrans;
		//MatAnimResult = MatAnimScale;

		int FrameIndexOfAll = 0;
		for (int j = 0; j < nFrames; j++)
		{
			if ( FindString (FrameNames[j], AnimedFrameNames[AnimSetIndex][i]) )
			{
				FrameIndexOfAll = j;	// 현재 뼈대가 전체 뼈대(Frame) 중 몇 번째 뼈대인지인지 구한다. ★★★
				break;
			}
		}

		AnimedFrameTM[i] = FrameTransformMatrices[FrameIndexOfAll] * MatAnimResult; // 애니메이션 적용된 로컬 월드 행렬!★★
		int a=0;
	}

	
	if (FAILED(g_pd3dDevice->CreateVertexBuffer(nMeshVertices * sizeof(CUSTOMVERTEX),
		0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pModelVB, NULL)))
		return E_FAIL;

	int SizeOfVertices = sizeof(CUSTOMVERTEX)*nMeshVertices;

	VOID* pVertices;
	if (FAILED(g_pModelVB->Lock(0, SizeOfVertices, (void**)&pVertices, 0)))
		return E_FAIL;
	memcpy(pVertices, NewVertices, SizeOfVertices);
	g_pModelVB->Unlock();

	delete[] NewVertices;
	*/

	return S_OK;
}