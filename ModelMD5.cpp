#include "ModelMD5.h"

KEYWORD	MD5KeyWords[] =
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

	// MD5ANIM
	"numFrames"				,	// 19
	"frameRate"				,	// 20
	"numAnimatedComponents"	,	// 21
	"hierarchy"				,	// 22
	"bounds"				,	// 23
	"baseframe"				,	// 24
	"frame "				,	// 25 ★
};

struct MD5Joint
{
	ANYNAME			Name;
	int				ParentID;
	ANY3DVECTOR3	Position;
	ANY3DVECTOR4	Orientation;
};

struct MD5Weight
{
	int				JointIndex;
	float			Bias;
	ANY3DVECTOR3	Position;
};

struct MD5Object	// 최신 버전
{
	ANYNAME			Name;
	int				ObjectID;
	int				numMeshes;
};

struct MD5Material	// 최신 버전
{
	ANYNAME			Name;
	ANYNAME			TextureFileName;
};

struct MD5Mesh
{
	ANYNAME			MaterialName;	// 최신 버전이면 Material의 이름이, 구버전이면 Texture 파일명이 들어 있다.

	ANYVERTEX		Vertices[MAX_MESH_VERTICES];
	int				VertexWeightStart[MAX_MESH_VERTICES];
	int				VertexNumWeights[MAX_MESH_VERTICES];

	ANYINDEX		Indices[MAX_MESH_INDICES];

	MD5Weight		Weights[MAX_MESH_WEIGHTS];
};

struct MD5BoundingBox
{
	ANY3DVECTOR3 Min;
	ANY3DVECTOR3 Max;
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
	int numFrames;
	int numJoints;
	int FrameRate;
	int numAnimatedComponents;

	float FrameTime;
	float TotalAnimTime;
	float CurAnimTime;

	MD5AnimationJointInfo	JointInfo[MAX_JOINTS];
	MD5BoundingBox			BoundingBoxes[MAX_FRAMES];
	MD5Joint				JointBaseFrame[MAX_JOINTS];
	MD5Frame				FrameData[MAX_FRAMES];
	MD5FrameSkeleton		FrameSekelton[MAX_FRAMES];
};

MD5Joint		ModelJoints[MAX_JOINTS];
MD5Object		ModelObjects[MAX_OBJECTS];
MD5Material		ModelMaterials[MAX_MATERIALS];
MD5Mesh			ModelMeshes[MAX_MESHES];
MD5Animation	ModelAnimation;


// 기타 변수
char		TempString[MAX_PARSE_LINE];
float		ParseFloats[MAX_PARSE_LINE];
char		TabChar[] = {'\t'};

// 함수 원형 선언!
void	StringTrim(char* sBuf);
char*	GetNameBetweenBraces(char* val);
char*	GetNameBetweenQuotes(char* val);
bool	FindString(char* val, char* cmp);
bool	FindChar(char* val, char cmp[]);
void	GetFloatFromLine(char* line, char* split, char* splita);

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
	strncpy_s(sT, sBuf, iLen);

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
	memset(TempString, 0, sizeof(TempString));

	int iLen = strlen(String);
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
					TempString[j-previ-1] = String[j];
				}
				break;
			}

			splitcount++;
			previ = i;
		}
	}

	return TempString;
}

char* GetNameBetweenBraces(char* val)
{
	memset(TempString, 0, sizeof(TempString));

	int iLen = strlen(val);
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
		TempString[i-firstspace] = val[i];
	}

	StringTrim(TempString);

	return TempString;
}

char* GetNameBetweenQuotes(char* val)
{
	memset(TempString, 0, sizeof(TempString));

	int iLen = strlen(val);
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
		TempString[i-firstspace] = val[i];
	}

	StringTrim(TempString);

	return TempString;
}

bool FindString(char* val, char* cmp)
{
	return (0 == _strnicmp(val, cmp, strlen(cmp)) ) ? 1: 0;
}

bool FindChar(char* val, char* cmp)
{
	for (int i = 0; i <= MAX_PARSE_LINE; i++)
	{
		if (val[i] == cmp[0])
			return true;
	}

	return false;
}

void GetFloatFromLine(char* line, char* split, char* splita)
{
	memset(ParseFloats, 0, sizeof(ParseFloats));	// float 값을 저장할 변수 초기화

	char temp[MAX_PARSE_LINE] = {0};
	strcpy_s(temp, line);
	strcat_s(temp, split);							// 마지막에 나오는 값도 얻어올 수 있도록 split문자 마지막에 하나 추가
	int iLen = strlen(temp);

	int previ = 0;
	int iNum = 0;

	for (int i = 0; i < iLen; i++)
	{
		char parsetemp[MAX_PARSE_LINE] = {0};
		if ((temp[i] == split[0]) || (temp[i] == splita[0]))
		{
			if (previ >= i)		// split 문자가 연속으로 나옴. 즉, 값이 없음!
				continue;

			for (int j = previ; j < i; j++)
			{
				parsetemp[j-previ] = temp[j];
			}
			iNum++;
			ParseFloats[iNum-1] = (float)atof(parsetemp);
			previ = i+1;
		}
	}
}

ANY3DVECTOR4 QuaternionMultiplyFloat(ANY3DVECTOR4 Q1, float Val)	// 사원수와 숫자의 곱
{
	ANY3DVECTOR4	Result;

	Result.x = Q1.x * Val;
	Result.y = Q1.y * Val;
	Result.z = Q1.z * Val;
	Result.w = Q1.w * Val;

	return		Result;
}

ANY3DVECTOR4 QuaternionMultiply(ANY3DVECTOR4 Q1, ANY3DVECTOR4 Q2)	// 사원수(Quaternion) 값과 위치(Position) 값을 곱해서 회전시킨다! ★
{
	ANY3DVECTOR4	Result;

	// 사원수(x, y, z, w) * 위치(x, y, z, 0.0f) * 역사원수(-x, -y, -z, w) = 회전된 위치(x, y, z, 0.0f)★

	Result.x = (Q2.w * Q1.x) + (Q2.x * Q1.w) + (Q2.y * Q1.z) - (Q2.z * Q1.y);
	Result.y = (Q2.w * Q1.y) - (Q2.x * Q1.z) + (Q2.y * Q1.w) + (Q2.z * Q1.x);
	Result.z = (Q2.w * Q1.z) + (Q2.x * Q1.y) - (Q2.y * Q1.x) + (Q2.z * Q1.w);
	Result.w = (Q2.w * Q1.w) - (Q2.x * Q1.x) - (Q2.y * Q1.y) - (Q2.z * Q1.z);

	return		Result;
}

ANY3DVECTOR4 QuaternionSum(ANY3DVECTOR4 Q1, ANY3DVECTOR4 Q2)	// 사원수의 덧셈
{
	ANY3DVECTOR4	Result;

	Result.x = Q1.x + Q2.x;
	Result.y = Q1.y + Q2.y;
	Result.z = Q1.z + Q2.z;
	Result.w = Q1.w + Q2.w;

	return		Result;
}

ANY3DVECTOR4 QuaternionNormalize(ANY3DVECTOR4 Q)	// 사원수(Quaternion) 값을 정규화(Normalize)한다.
{
	ANY3DVECTOR4	Result;
	float		DevideBy = sqrt(Q.x*Q.x + Q.y*Q.y + Q.z*Q.z + Q.w*Q.w);

	Result.x = Q.x / DevideBy;
	Result.y = Q.y / DevideBy;
	Result.z = Q.z / DevideBy;
	Result.w = Q.w / DevideBy;

	return		Result;
}

ANY3DVECTOR4 QuaternionInverse(ANY3DVECTOR4 Q)
{
	ANY3DVECTOR4	Result;
	float			DevideBy = Q.x*Q.x + Q.y*Q.y + Q.z*Q.z + Q.w*Q.w;	// 이걸로 나눠줘야 정규화된다!

	Result.x = -Q.x	/ DevideBy;
	Result.y = -Q.y	/ DevideBy;
	Result.z = -Q.z	/ DevideBy;
	Result.w =  Q.w	/ DevideBy;

	return			Result;
}

float QuaternionDot(ANY3DVECTOR4 Q1, ANY3DVECTOR4 Q2)		// 사원수의 내적
{
	float	Result;

	Result = Q1.x * Q2.x + Q1.y * Q2.y + Q1.z * Q2.z + Q1.w * Q2.w;

	return Result;
}

ANY3DVECTOR4 QuaternionLerp(ANY3DVECTOR4 Q1, ANY3DVECTOR4 Q2, float Time)	// 사원수 두 개를 선형 보간(Linear intERPolation)한다.
{
	ANY3DVECTOR4 Result;

	Result = QuaternionNormalize( QuaternionSum( QuaternionMultiplyFloat(Q1, 1-Time), QuaternionMultiplyFloat(Q2, Time) ) );

	return Result;
}

ANY3DVECTOR4 QuaternionSlerp(ANY3DVECTOR4 Q1, ANY3DVECTOR4 Q2, float Time)	// 사원수 두 개를 구면 선형 보간(Spherical Linear intERPolation)한다.
{
	ANY3DVECTOR4	Result;
	float Dot = QuaternionDot(Q1, Q2);

	if (Dot < 0)
	{
		Dot = -Dot;
		Result = QuaternionInverse(Q2);
	} else Result = Q2;
		
	if (Dot < 0.95f)
	{
		float Angle = acosf(Dot);
		Result = QuaternionSum( QuaternionMultiplyFloat( Q1, sinf(Angle*(1-Time))/sinf(Angle) ),
			QuaternionMultiplyFloat( Result, sinf(Angle*Time)/sinf(Angle) ) );
		return Result;
	} else	// 각도가 작을 경우, 선형 보간을 한다.
		Result = QuaternionLerp(Q1, Q2, Time);
		return Result;
}

ANY3DVECTOR4 SetANY3DVECTOR4(float x, float y, float z, float w)
{
	ANY3DVECTOR4 Result;
	Result.x = x;
	Result.y = y;
	Result.z = z;
	Result.w = w;

	return Result;
}

bool ModelMD5::OpenModelFromFile(char* FileName)
{
	FILE*	fp;							// 불러올 파일
	char	sLine[MAX_PARSE_LINE];		// 파일에서 읽어올 한 줄

	if( fopen_s(&fp, FileName, "rt") )
		return false;

	memset(ModelJoints, 0, sizeof(ModelJoints));
	memset(ModelObjects, 0, sizeof(ModelObjects));
	memset(ModelMaterials, 0, sizeof(ModelMaterials));
	memset(ModelMeshes, 0, sizeof(ModelMeshes));

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
		fgets(sLine, MAX_PARSE_LINE, fp);	// 파일에서 한 줄을 읽어온다.
		StringTrim(sLine);					// 앞 뒤 공백 제거
		int iLen = strlen(sLine);

		if (iLen <= 1)						// 빈 줄이면 다음 줄로 넘어가자! (개행문자 때문에 최소 길이가 1임)★
			continue;


		// 중괄호 열기 전
		if ( FindString(sLine, MD5KeyWords[0]) )	// MD5Version 4843 & MD5Version 10 지원 ★★
		{
			GetFloatFromLine(sLine, " ", TabChar);
			MD5Version = (int)ParseFloats[1];
			continue;
		}

		if ( FindString(sLine, MD5KeyWords[1]) )	// commandline "~"
		{
			continue;								// 주석은 건너뛰자
		}

		if ( FindString(sLine, MD5KeyWords[2]) )	// numJoints ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			numJoints = (int)ParseFloats[1];
			continue;
		}

		if ( FindString(sLine, MD5KeyWords[3]) )	// numMeshes ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			numMeshes = (int)ParseFloats[1];
			continue;
		}

		if ( FindString(sLine, MD5KeyWords[4]) )	// numObjects ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			numObjects = (int)ParseFloats[1];
			continue;
		}

		if ( FindString(sLine, MD5KeyWords[5]) )	// numMaterials ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			numMaterials = (int)ParseFloats[1];
			continue;
		}

		if ( FindString(sLine, MD5KeyWords[6]) )	// joints {
		{
			strcpy_s(CurKey, MD5KeyWords[6]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}

		if ( FindString(sLine, MD5KeyWords[7]) )	// objects {
		{
			strcpy_s(CurKey, MD5KeyWords[7]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}

		if ( FindString(sLine, MD5KeyWords[8]) )	// materials {
		{
			strcpy_s(CurKey, MD5KeyWords[8]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}

		if ( FindString(sLine, MD5KeyWords[9]) )	// mesh {
		{
			MeshCount++;
			strcpy_s(CurKey, MD5KeyWords[9]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}


		// 중괄호 열린 상태
		if ( FindString(CurKey, MD5KeyWords[6]) )	// joints {
		{
			CurKeyCount++;

			// joint의 이름에 공백이 들어가 있는 경우에 대비해 이름 읽어온 후 지우고 float값을 얻어온다.
			char tempName[MAX_NAME_LEN];
			memset(tempName, 0, sizeof(tempName));
			strcpy_s( tempName, GetNameBetweenQuotes(sLine) );
			strcpy_s( ModelJoints[CurKeyCount-1].Name, tempName );
			int	tempLen = strlen(tempName) + 2;	// 이름 & 앞뒤 쌍따옴표 제거하기 위함

			char tempLine[MAX_PARSE_LINE];
			memset(tempLine, 0, sizeof(tempLine));
			int	tempLineLen = strlen(sLine);

			for (int i = 0; i < tempLineLen-tempLen; i++)
			{
				tempLine[i] = sLine[i+tempLen];
			}

			GetFloatFromLine(tempLine, " ", TabChar);
			
			MD5Joint	TempJoint;

				TempJoint.ParentID = (int)ParseFloats[0];
				TempJoint.Position.x = ParseFloats[2];
				TempJoint.Position.z = ParseFloats[3];		// 3DSMAX -> DIRECTX 좌표계!★
				TempJoint.Position.y = ParseFloats[4];
				TempJoint.Orientation.x = ParseFloats[7];
				TempJoint.Orientation.z = ParseFloats[8];	// 3DSMAX -> DIRECTX 좌표계!★
				TempJoint.Orientation.y = ParseFloats[9];

				// 사원수(Quaternion) w값 생성
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

		if ( FindString(CurKey, MD5KeyWords[7]) )		// objects {
		{
			CurKeyCount++;
			strcpy_s( ModelObjects[CurKeyCount-1].Name, GetNameBetweenQuotes(sLine) );
			GetFloatFromLine(sLine, " ", TabChar);

			ModelObjects[CurKeyCount-1].ObjectID = (int)ParseFloats[1];
			ModelObjects[CurKeyCount-1].numMeshes = (int)ParseFloats[2];

			if (CurKeyCount >= numObjects)
				memset(CurKey, 0, sizeof(CurKey));		// 중괄호 닫기

			continue;
		}

		if ( FindString(CurKey, MD5KeyWords[8]) )		// materials {
		{
			CurKeyCount++;
			strcpy_s( ModelMaterials[CurKeyCount-1].Name, SplitString(sLine, "\"", 0) );
			strcpy_s( ModelMaterials[CurKeyCount-1].TextureFileName, SplitString(sLine, "\"", 2) );

			if (CurKeyCount >= numMaterials)
				memset(CurKey, 0, sizeof(CurKey));		// 중괄호 닫기

			continue;
		}

		if ( FindString(CurKey, MD5KeyWords[9]) )		// mesh {
		{
			if ( FindString(sLine, MD5KeyWords[10]) )	// 주석 줄
				continue;								// 건너뛰자!

			if ( FindString(sLine, MD5KeyWords[11]) )	// meshindex
				continue;

			if ( FindString(sLine, MD5KeyWords[12]) )	// shader
			{
				if ( MD5Version == 10 ) // 옛날 버전이면 바로 텍스처 파일 이름이 나옴!!★
				{
					strcpy_s( ModelMaterials[MeshCount-1].TextureFileName, SplitString(sLine, "\"", 1) );
				}
				else					// 최신 버전이면 material의 이름이 나옴!
				{
					strcpy_s( ModelMeshes[MeshCount-1].MaterialName, SplitString(sLine, "\"", 1) );
				}

				continue;
			}
			if ( FindString(sLine, MD5KeyWords[13]) )	// numverts
			{
				GetFloatFromLine(sLine, " ", TabChar);
				numMeshVertices[MeshCount-1] = (int)ParseFloats[1];

				continue;
			}
			if ( FindString(sLine, MD5KeyWords[14]) )	// vert
			{
				GetFloatFromLine(sLine, " ", TabChar);
				int VIndex = (int)ParseFloats[1];

				ModelMeshes[MeshCount-1].Vertices[VIndex].texCoord.x = ParseFloats[3];
				ModelMeshes[MeshCount-1].Vertices[VIndex].texCoord.y = ParseFloats[4];
				ModelMeshes[MeshCount-1].VertexWeightStart[VIndex] = (int)ParseFloats[6];
				ModelMeshes[MeshCount-1].VertexNumWeights[VIndex] = (int)ParseFloats[7];

				continue;
			}
			if ( FindString(sLine, MD5KeyWords[15]) )	// numtris
			{
				GetFloatFromLine(sLine, " ", TabChar);
				numMeshIndices[MeshCount-1] = (int)ParseFloats[1];

				continue;
			}
			if ( FindString(sLine, MD5KeyWords[16]) )	// tri
			{
				GetFloatFromLine(sLine, " ", TabChar);
				int TIndex = (int)ParseFloats[1];

				ModelMeshes[MeshCount-1].Indices[TIndex]._0 = (int)ParseFloats[2];
				ModelMeshes[MeshCount-1].Indices[TIndex]._1 = (int)ParseFloats[3];
				ModelMeshes[MeshCount-1].Indices[TIndex]._2 = (int)ParseFloats[4];

				continue;
			}
			if ( FindString(sLine, MD5KeyWords[17]) )	// numweights
			{
				GetFloatFromLine(sLine, " ", TabChar);
				numWeights[MeshCount-1] = (int)ParseFloats[1];
				continue;
			}
			if ( FindString(sLine, MD5KeyWords[18]) )	// weight
			{
				GetFloatFromLine(sLine, " ", TabChar);
				int WIndex = (int)ParseFloats[1];
				ModelMeshes[MeshCount-1].Weights[WIndex].JointIndex = (int)ParseFloats[2];
				ModelMeshes[MeshCount-1].Weights[WIndex].Bias = ParseFloats[3];
				ModelMeshes[MeshCount-1].Weights[WIndex].Position.x = ParseFloats[5];
				ModelMeshes[MeshCount-1].Weights[WIndex].Position.z = ParseFloats[6];		// 3DSMAX -> DIRECTX 좌표계!★
				ModelMeshes[MeshCount-1].Weights[WIndex].Position.y = ParseFloats[7];

				continue;
			}

			if (FindChar(sLine, "}"))
				memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기
			continue;
		}


	} // WHILE문 종료!

	// 정점 정보 업데이트
	for (int i = 0; i < numMeshes; i++)
	{
		for (int j = 0; j < numMeshVertices[i]; j++)
		{
			int WeightStart	= ModelMeshes[i].VertexWeightStart[j];
			int nWeights	= ModelMeshes[i].VertexNumWeights[j];

			ANY3DVECTOR3	Result = {0.0f, 0.0f, 0.0f};

			for (int k = 0; k < nWeights; k++)
			{
				int CurJointID = ModelMeshes[i].Weights[WeightStart+k].JointIndex;
				ANY3DVECTOR4 Q1, POS, Q2, Rotated;
				MD5Joint	TempJoint	= ModelJoints[CurJointID];
				MD5Weight	TempWeight	= ModelMeshes[i].Weights[WeightStart+k];

				Q1	= SetANY3DVECTOR4(TempJoint.Orientation.x, TempJoint.Orientation.y, TempJoint.Orientation.z, TempJoint.Orientation.w);

				POS	= SetANY3DVECTOR4(TempWeight.Position.x, TempWeight.Position.y, TempWeight.Position.z, 0.0f);

				Q2	= QuaternionInverse(Q1);

				Rotated = QuaternionMultiply( QuaternionMultiply( Q1, POS ), Q2 );

				Result.x += (TempJoint.Position.x + Rotated.x) * TempWeight.Bias;
				Result.y += (TempJoint.Position.y + Rotated.y) * TempWeight.Bias;
				Result.z += (TempJoint.Position.z + Rotated.z) * TempWeight.Bias;
			}
			
			ModelMeshes[i].Vertices[j].pos = Result;
		}
	}

	fclose(fp);

	return true;
}

bool ModelMD5::OpenAnimationFromFile(char* FileName)
{
	FILE*	fp;								// 불러올 파일
	char	sLine[MAX_PARSE_LINE];			// 파일에서 읽어올 한 줄

	if( fopen_s(&fp, FileName, "rt") )
		return false;

	char	CurKey[MAX_KEYWORD_LEN] = {0};
	int		CurKeyCount = 0;
	int		FrameCount = 0;
	int		TempFrameID = 0;

	while(!feof(fp))
	{
		fgets(sLine, MAX_PARSE_LINE, fp);	// 파일에서 한 줄을 읽어온다.
		StringTrim(sLine);					// 앞 뒤 공백 제거
		int iLen = strlen(sLine);

		if (iLen <= 1)						// 빈 줄이면 다음 줄로 넘어가자! (개행문자 때문에 최소 길이가 1이다.)★
			continue;


		// 중괄호 열기 전
		if ( FindString(sLine, MD5KeyWords[0]) )	// MD5Version 4843
		{
			GetFloatFromLine(sLine, " ", TabChar);
			// if ( MD5Version != (int)ParseFloats[1]);	// 버전이 달라도 그냥 불러오자
			continue;
		}

		if ( FindString(sLine, MD5KeyWords[1]) )	// commandline "~"
		{
			continue;								// 주석은 건너뛰자
		}

		if ( FindString(sLine, MD5KeyWords[19]) )	// numFrames ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			ModelAnimation.numFrames = (int)ParseFloats[1];
			continue;
		}

		if ( FindString(sLine, MD5KeyWords[2]) )	// numJoints ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			ModelAnimation.numJoints = (int)ParseFloats[1];
			if (ModelAnimation.numJoints != numJoints)	// 애니메이션의 Joint 수가 기존 모델의 numJoints와 다를 경우
				return false;							// 불러오기를 중단한다. ★★
			continue;
		}
		if ( FindString(sLine, MD5KeyWords[20]) )	// frameRate ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			ModelAnimation.FrameRate = (int)ParseFloats[1];
			continue;
		}
		if ( FindString(sLine, MD5KeyWords[21]) )	// numAnimatedComponents ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			ModelAnimation.numAnimatedComponents = (int)ParseFloats[1];
			continue;
		}
		if ( FindString(sLine, MD5KeyWords[22]) )	// hierarchy {
		{
			strcpy_s(CurKey, MD5KeyWords[22]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}
		if ( FindString(sLine, MD5KeyWords[23]) )	// bounds {
		{
			strcpy_s(CurKey, MD5KeyWords[23]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}
		if ( FindString(sLine, MD5KeyWords[24]) )	// baseframe {
		{
			strcpy_s(CurKey, MD5KeyWords[24]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}
		if ( FindString(sLine, MD5KeyWords[25]) )	// frame ~ {
		{
			FrameCount++;
			GetFloatFromLine(sLine, " ", TabChar);
			TempFrameID = (int)ParseFloats[1];

			strcpy_s(CurKey, MD5KeyWords[25]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}


		// 중괄호 열린 상태
		if ( FindString(CurKey, MD5KeyWords[22]) )	// hierarchy {
		{
			CurKeyCount++;

			// hierarchy(즉, joint)의 이름에 공백이 들어가 있는 경우에 대비해 이름 읽어온 후 지우고 float값을 얻어온다.
			char tempName[MAX_NAME_LEN];
			memset(tempName, 0, sizeof(tempName));
			strcpy_s( tempName, SplitString(sLine, "\"", 0) );
			strcpy_s( ModelAnimation.JointInfo[CurKeyCount-1].Name, tempName );
			int	tempLen = strlen(tempName) + 2;		// 이름 & 앞뒤 쌍따옴표 제거하기 위함

			char tempLine[MAX_PARSE_LINE];
			memset(tempLine, 0, sizeof(tempLine));
			int	tempLineLen = strlen(sLine);
			for (int i = 0; i < tempLineLen-tempLen; i++)
			{
				tempLine[i] = sLine[i+tempLen];
			}
			
			GetFloatFromLine(tempLine, " ", TabChar);
			ModelAnimation.JointInfo[CurKeyCount-1].ParentID	= (int)ParseFloats[0];
			ModelAnimation.JointInfo[CurKeyCount-1].Flags		= (int)ParseFloats[1];
			ModelAnimation.JointInfo[CurKeyCount-1].StartIndex	= (int)ParseFloats[2];

			if (CurKeyCount >= ModelAnimation.numJoints)
				memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기

			continue;
		}

		if ( FindString(CurKey, MD5KeyWords[23]) )	// bounds {
		{
			CurKeyCount++;
			GetFloatFromLine(sLine, " ", TabChar);

			MD5BoundingBox	TempBB;

				TempBB.Min.x	= ParseFloats[1];
				TempBB.Min.z	= ParseFloats[2];	// 3DSMAX -> DIRECTX 좌표계!★
				TempBB.Min.y	= ParseFloats[3];
				TempBB.Max.x	= ParseFloats[6];
				TempBB.Max.z	= ParseFloats[7];	// 3DSMAX -> DIRECTX 좌표계!★
				TempBB.Max.y	= ParseFloats[8];

			ModelAnimation.BoundingBoxes[CurKeyCount-1] = TempBB;

			if (CurKeyCount >= ModelAnimation.numFrames)
				memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기

			continue;
		}

		if ( FindString(CurKey, MD5KeyWords[24]) )	// baseframe {
		{
			// BASEFRAME: Joint 개수만큼 존재, 각 Frame의 기반이 되는 위치!★
			CurKeyCount++;
			GetFloatFromLine(sLine, " ", TabChar);

			MD5Joint	TempBaseFrame;

				TempBaseFrame.Position.x		= ParseFloats[1];
				TempBaseFrame.Position.z		= ParseFloats[2];	// 3DSMAX -> DIRECTX 좌표계!★
				TempBaseFrame.Position.y		= ParseFloats[3];
				TempBaseFrame.Orientation.x		= ParseFloats[6];
				TempBaseFrame.Orientation.z		= ParseFloats[7];	// 3DSMAX -> DIRECTX 좌표계!★
				TempBaseFrame.Orientation.y		= ParseFloats[8];

			ModelAnimation.JointBaseFrame[CurKeyCount-1] = TempBaseFrame;

			if (CurKeyCount >= ModelAnimation.numJoints)
				memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기

			continue;
		}

		if ( FindString(CurKey, MD5KeyWords[25]) )	// frame ~ {
		{
			// FRAME: Joint 개수만큼 존재, 각 Frame이 Baseframe에서 변화한 위치!★
			MD5Frame	TempFrameData;
			TempFrameData.FrameID = TempFrameID;

			for(int i = 0; i < ModelAnimation.numAnimatedComponents; i++)
			{
				GetFloatFromLine(sLine, " ", TabChar);

				TempFrameData.JointData[i++] = ParseFloats[0];
				TempFrameData.JointData[i++] = ParseFloats[1];	// 3DSMAX -> DIRECTX 좌표계!★
				TempFrameData.JointData[i++] = ParseFloats[2];
				TempFrameData.JointData[i++] = ParseFloats[3];
				TempFrameData.JointData[i++] = ParseFloats[4];	// 3DSMAX -> DIRECTX 좌표계!★
				TempFrameData.JointData[i] = ParseFloats[5];

				fgets(sLine, MAX_PARSE_LINE, fp);	// 파일에서 한 줄을 읽어온다.
				StringTrim(sLine);					// 앞 뒤 공백 제거
			}

			ModelAnimation.FrameData[FrameCount-1] = TempFrameData;


			for (int i = 0; i < ModelAnimation.numJoints; i++)
			{
				MD5Joint TempFrameJoint = ModelAnimation.JointBaseFrame[i];	// 기본적으로 BaseFrame 값에서 출발!
				TempFrameJoint.ParentID = ModelAnimation.JointInfo[i].ParentID;
				
				int tempJointIndexStart = ModelAnimation.JointInfo[i].StartIndex;

				if(ModelAnimation.JointInfo[i].Flags & 1)	//	Position.x	( 000001 )
					TempFrameJoint.Position.x = TempFrameData.JointData[tempJointIndexStart++];

				if(ModelAnimation.JointInfo[i].Flags & 2)	//	Position.y	( 000010 )
					TempFrameJoint.Position.z = TempFrameData.JointData[tempJointIndexStart++];	// 3DSMAX -> DIRECTX 좌표계!★

				if(ModelAnimation.JointInfo[i].Flags & 4)	//	Position.z	( 000100 )
					TempFrameJoint.Position.y = TempFrameData.JointData[tempJointIndexStart++];

				if(ModelAnimation.JointInfo[i].Flags & 8)	//	Orientation.x	( 001000 )
					TempFrameJoint.Orientation.x = TempFrameData.JointData[tempJointIndexStart++];

				if(ModelAnimation.JointInfo[i].Flags & 16)	//	Orientation.y	( 010000 )
					TempFrameJoint.Orientation.z = TempFrameData.JointData[tempJointIndexStart++];	// 3DSMAX -> DIRECTX 좌표계!★

				if(ModelAnimation.JointInfo[i].Flags & 32)	//	Orientation.z	( 100000 )
					TempFrameJoint.Orientation.y = TempFrameData.JointData[tempJointIndexStart];

				// 사원수(Quaternion) w값 생성
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
					MD5Joint ParentJoint = ModelAnimation.FrameSekelton[FrameCount-1].Skeleton[TempFrameJoint.ParentID];

					ANY3DVECTOR4 Q1, POS, Q2, RotatedPos;

					Q1	= SetANY3DVECTOR4(ParentJoint.Orientation.x, ParentJoint.Orientation.y, ParentJoint.Orientation.z, ParentJoint.Orientation.w);

					POS	= SetANY3DVECTOR4(TempFrameJoint.Position.x, TempFrameJoint.Position.y, TempFrameJoint.Position.z, 0.0f);

					Q2	= QuaternionInverse(Q1);

					RotatedPos = QuaternionMultiply( QuaternionMultiply( Q1, POS ), Q2 );

					TempFrameJoint.Position.x = RotatedPos.x + ParentJoint.Position.x;
					TempFrameJoint.Position.y = RotatedPos.y + ParentJoint.Position.y;
					TempFrameJoint.Position.z = RotatedPos.z + ParentJoint.Position.z;

					ANY3DVECTOR4	TempJointOrient;
					TempJointOrient = TempFrameJoint.Orientation;
					TempJointOrient = QuaternionMultiply(ParentJoint.Orientation, TempJointOrient);
					TempJointOrient = QuaternionNormalize(TempJointOrient);
					
					TempFrameJoint.Orientation = TempJointOrient;
				}
				ModelAnimation.FrameSekelton[FrameCount-1].Skeleton[i] = TempFrameJoint;
			}

			memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기
			continue;
		}
	}

	// 변수 초기화
	ModelAnimation.FrameTime = 1.0f / ModelAnimation.FrameRate;							// 각 프레임(Frame) 당 시간 설정 ★
	ModelAnimation.TotalAnimTime = ModelAnimation.numFrames * ModelAnimation.FrameTime;	// 애니메이션 전체 길이
	ModelAnimation.CurAnimTime = 0.0f;													// 현재 애니메이션 시간을 0으로 초기화

	return true;
}

void ModelMD5::Animate(float Speed)
{
	ModelAnimation.CurAnimTime += Speed;							// 애니메이션 진행! (숫자로 애니메이션 속도 조절 가능)

	if(ModelAnimation.CurAnimTime > ModelAnimation.TotalAnimTime)	// 애니메이션이 끝나면
		ModelAnimation.CurAnimTime = 0.0f;							// 다시 처음으로! ★

	float CurrentFrame = ModelAnimation.CurAnimTime * ModelAnimation.FrameRate; // 현재 프레임
	int Frame0 = (int)floorf( CurrentFrame );
	int Frame1 = Frame0 + 1;

	if(Frame0 == ModelAnimation.numFrames-1)		// Frame0이 마지막 프레임이라면 Frame1은 다시 0번으로 가자!
		Frame1 = 0;

	float Interpolation = CurrentFrame - Frame0;	// Frame1과 Frame0의 시간차를 얻어와 보간값으로 사용한다.

	MD5Joint InterpolatedJoints[MAX_JOINTS];

	// 보간값(Interpolation) 계산
	for( int i = 0; i < ModelAnimation.numJoints; i++)
	{
		MD5Joint TempJoint;
		MD5Joint Joint0 = ModelAnimation.FrameSekelton[Frame0].Skeleton[i];	// Frame0의 i번째 JointData
		MD5Joint Joint1 = ModelAnimation.FrameSekelton[Frame1].Skeleton[i];	// Frame1의 i번째 JointData

		//TempJoint.ParentID = Joint0.ParentID;								// 부모 ID 지정

		TempJoint.Position.x = Joint0.Position.x + (Interpolation * (Joint1.Position.x - Joint0.Position.x));
		TempJoint.Position.y = Joint0.Position.y + (Interpolation * (Joint1.Position.y - Joint0.Position.y));
		TempJoint.Position.z = Joint0.Position.z + (Interpolation * (Joint1.Position.z - Joint0.Position.z));

		ANY3DVECTOR4 Joint0Orient = Joint0.Orientation;
		ANY3DVECTOR4 Joint1Orient = Joint1.Orientation;

		TempJoint.Orientation = QuaternionSlerp(Joint0Orient, Joint1Orient, Interpolation);	// 구면 선형 보간

		InterpolatedJoints[i] = TempJoint;		// 결과 값을 저장한다!
	}

	// 정점 정보 업데이트
	for (int i = 0; i < numMeshes; i++)
	{
		for (int j = 0; j < numMeshVertices[i]; j++)
		{
			ANYVERTEX	TempVert = ModelMeshes[i].Vertices[j];
			memset(&TempVert.pos, 0, sizeof(TempVert.pos));			// 먼저 값을 0으로 초기화!

			int WeightStart = ModelMeshes[i].VertexWeightStart[j];
			int nWeights = ModelMeshes[i].VertexNumWeights[j];

			float ResultX = 0.0f;
			float ResultY = 0.0f;
			float ResultZ = 0.0f;

			for (int k = 0; k < nWeights; k++)
			{
				MD5Weight	TempWeight	= ModelMeshes[i].Weights[WeightStart+k];
				MD5Joint	TempJoint	= InterpolatedJoints[TempWeight.JointIndex];
				
				ANY3DVECTOR4 Q1, POS, Q2, Rotated;

				Q1	= SetANY3DVECTOR4(TempJoint.Orientation.x, TempJoint.Orientation.y, TempJoint.Orientation.z, TempJoint.Orientation.w);

				POS	= SetANY3DVECTOR4(TempWeight.Position.x, TempWeight.Position.y, TempWeight.Position.z, 0.0f);

				Q2	= QuaternionInverse(Q1);

				Rotated = QuaternionMultiply( QuaternionMultiply( Q1, POS ), Q2 );

				TempVert.pos.x += ( TempJoint.Position.x + Rotated.x ) * TempWeight.Bias;
				TempVert.pos.y += ( TempJoint.Position.y + Rotated.y ) * TempWeight.Bias;
				TempVert.pos.z += ( TempJoint.Position.z + Rotated.z ) * TempWeight.Bias;
			}
			
			ModelMeshes[i].Vertices[j].pos = TempVert.pos;
		}
	}
}

HRESULT ModelMD5::UpdateVertices(LPDIRECT3DDEVICE9 D3DDevice, int MeshIndex)
{
	if ( ModelVB != NULL)
		ModelVB->Release();
	if ( ModelIB != NULL)
		ModelIB->Release();

	// 정점 버퍼 업데이트!
	ANYVERTEX *NewVertices = new ANYVERTEX[numMeshVertices[MeshIndex]];

		for (int i = 0; i < numMeshVertices[MeshIndex]; i++)
		{
			NewVertices[i].pos.x = ModelMeshes[MeshIndex].Vertices[i].pos.x;
			NewVertices[i].pos.y = ModelMeshes[MeshIndex].Vertices[i].pos.y;
			NewVertices[i].pos.z = ModelMeshes[MeshIndex].Vertices[i].pos.z;
			NewVertices[i].texCoord.x = ModelMeshes[MeshIndex].Vertices[i].texCoord.x;
			NewVertices[i].texCoord.y = ModelMeshes[MeshIndex].Vertices[i].texCoord.y;
		}

		if (FAILED(D3DDevice->CreateVertexBuffer(numMeshVertices[MeshIndex] * sizeof(ANYVERTEX),
			0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &ModelVB, NULL)))
			return E_FAIL;

		int SizeOfVertices = sizeof(ANYVERTEX)*numMeshVertices[MeshIndex];

		VOID* pVertices;
		if (FAILED(ModelVB->Lock(0, SizeOfVertices, (void**)&pVertices, 0)))
			return E_FAIL;
		memcpy(pVertices, NewVertices, SizeOfVertices);
		ModelVB->Unlock();

	delete[] NewVertices;

	// 색인 버퍼 업데이트!
	ANYINDEX *NewIndices = new ANYINDEX[numMeshIndices[MeshIndex]];

		for (int i = 0; i < numMeshIndices[MeshIndex]; i++)
		{
			NewIndices[i]._0 = ModelMeshes[MeshIndex].Indices[i]._0;
			NewIndices[i]._1 = ModelMeshes[MeshIndex].Indices[i]._1;
			NewIndices[i]._2 = ModelMeshes[MeshIndex].Indices[i]._2;
		}

		int SizeOfIndices = sizeof(ANYINDEX)*numMeshIndices[MeshIndex];

		if (FAILED(D3DDevice->CreateIndexBuffer(SizeOfIndices, 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ModelIB, NULL)))
			return E_FAIL;

		VOID* pIndices;
		if (FAILED(ModelIB->Lock(0, SizeOfIndices, (void **)&pIndices, 0)))
			return E_FAIL;
		memcpy(pIndices, NewIndices, SizeOfIndices);
		ModelIB->Unlock();
	
	delete[] NewIndices;

	return S_OK;	// 함수 종료!
}

HRESULT ModelMD5::LoadMeshToDraw(LPDIRECT3DDEVICE9 D3DDevice, int MeshIndex)
{
	if ( ModelVB != NULL)
		ModelVB->Release();
	if ( ModelIB != NULL)
		ModelIB->Release();
	if ( ModelTextures[MeshIndex] != NULL)
		ModelTextures[MeshIndex]->Release();

	ANYVERTEX *NewVertices = new ANYVERTEX[numMeshVertices[MeshIndex]];

	for (int i = 0; i < numMeshVertices[MeshIndex]; i++)
	{
		NewVertices[i].pos.x = ModelMeshes[MeshIndex].Vertices[i].pos.x;
		NewVertices[i].pos.y = ModelMeshes[MeshIndex].Vertices[i].pos.y;
		NewVertices[i].pos.z = ModelMeshes[MeshIndex].Vertices[i].pos.z;
		NewVertices[i].texCoord.x = ModelMeshes[MeshIndex].Vertices[i].texCoord.x;
		NewVertices[i].texCoord.y = ModelMeshes[MeshIndex].Vertices[i].texCoord.y;
	}

	if (FAILED(D3DDevice->CreateVertexBuffer(numMeshVertices[MeshIndex] * sizeof(ANYVERTEX),
		0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &ModelVB, NULL)))
		return E_FAIL;

	int SizeOfVertices = sizeof(ANYVERTEX)*numMeshVertices[MeshIndex];

	VOID* pVertices;
	if (FAILED(ModelVB->Lock(0, SizeOfVertices, (void**)&pVertices, 0)))
		return E_FAIL;
	memcpy(pVertices, NewVertices, SizeOfVertices);
	ModelVB->Unlock();

	delete[] NewVertices;

	
	ANYINDEX *NewIndices = new ANYINDEX[numMeshIndices[MeshIndex]];

	for (int i = 0; i < numMeshIndices[MeshIndex]; i++)
	{
		NewIndices[i]._0 = ModelMeshes[MeshIndex].Indices[i]._0;
		NewIndices[i]._1 = ModelMeshes[MeshIndex].Indices[i]._1;
		NewIndices[i]._2 = ModelMeshes[MeshIndex].Indices[i]._2;
	}

	int SizeOfIndices = sizeof(ANYINDEX)*numMeshIndices[MeshIndex];

	if (FAILED(D3DDevice->CreateIndexBuffer(SizeOfIndices, 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ModelIB, NULL)))
		return E_FAIL;

	VOID* pIndices;
	if (FAILED(ModelIB->Lock(0, SizeOfIndices, (void **)&pIndices, 0)))
		return E_FAIL;
	memcpy(pIndices, NewIndices, SizeOfIndices);
	ModelIB->Unlock();
	
	delete[] NewIndices;

	D3DXCreateTextureFromFile(D3DDevice, ModelMaterials[MeshIndex].TextureFileName, &ModelTextures[MeshIndex]);

	return S_OK;
}

void ModelMD5::CreateModel(LPDIRECT3DDEVICE9 D3DDevice)
{
	for (int i = 0; i < numMeshes; i++)
	{
		LoadMeshToDraw(D3DDevice, i);
	}
}

void ModelMD5::DrawModel(LPDIRECT3DDEVICE9 D3DDevice)
{
	// 모델의 각 메쉬를 그린다!
	for (int i = 0; i < numMeshes; i++)
	{
		UpdateVertices(D3DDevice, i);

		D3DDevice->SetTexture(0, ModelTextures[i]);
		D3DDevice->SetStreamSource(0, ModelVB, 0, sizeof(ANYVERTEX));
		D3DDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
		D3DDevice->SetIndices(ModelIB);

		D3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numMeshVertices[i], 0, numMeshIndices[i]);
	}

	return;
}

void ModelMD5::Destroy()
{
	if ( ModelVB != NULL)
		ModelVB->Release();
	if ( ModelIB != NULL)
		ModelIB->Release();
	
	for (int i = 0; i < numMeshes; i++)
	{
		if ( ModelTextures[i] != NULL)
			ModelTextures[i]->Release();
	}

	return;
}