#include "ModelMD5.h"
#include "Parser.h"

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

MD5Joint		ModelJoints[MAX_JOINTS];
MD5Object		ModelObjects[MAX_OBJECTS];
MD5Material		ModelMaterials[MAX_MATERIALS];
MD5Mesh			ModelMeshes[MAX_MESHES];


// 기타 변수
char		TabChar[] = {'\t'};


// 함수 원형 선언
char*	StringTrim(char* sBuf);
char*	SplitString(char* String, char* SplitChar, int SplitIndex);
char*	GetNameBetweenBraces(char* val);
char*	GetNameBetweenQuotes(char* val);
bool	FindString_MD5(char* val, char* cmp);
bool	FindChar(char* val, char cmp[]);
void	GetFloatFromLine(char* line, char* split, char* splita);


bool ModelMD5::CreateModel(LPDIRECT3DDEVICE9 D3DDevice, char* BaseDir, char* FileNameWithoutExtension)
{
	// 변수 초기화
	memset(ModelJoints, 0, sizeof(ModelJoints));
	memset(ModelObjects, 0, sizeof(ModelObjects));
	memset(ModelMaterials, 0, sizeof(ModelMaterials));
	memset(ModelMeshes, 0, sizeof(ModelMeshes));
	memset(&ModelAnimation, 0, sizeof(ModelAnimation));
	memset(&ModelInstances, 0, sizeof(ModelInstances));

	memset(numMeshVertices, 0, sizeof(numMeshVertices));
	memset(numMeshIndices, 0, sizeof(numMeshIndices));
	memset(numWeights, 0, sizeof(numWeights));

	MD5Version		= 0;
	numMeshes		= 0;
	numJoints		= 0;
	numObjects		= 0;
	numMaterials	= 0;
	TotalAnimCount	= 0;
	numInstances	= 0;

	// 모델 파일이 있는 폴더 설정
	SetBaseDirection(BaseDir);

	// 모델 메쉬 불러오기
	char	NewFileName[MAX_NAME_LEN] = {0};
		strcpy_s(NewFileName, FileNameWithoutExtension);
		strcat_s(NewFileName, ".MD5MESH");
	OpenMeshFromFile(NewFileName);

	// 모델 텍스처 불러오기
	for (int i = 0; i < numMeshes; i++)
	{
		SetTexture(D3DDevice, i);
	}

	return true;
}

bool ModelMD5::CreateAnimation(int AnimationID, char* AnimationFileName, float AnimationSpeed)
{
	// 모델 애니메이션 불러오기
	char	NewFileName[MAX_NAME_LEN] = {0};
		memset(NewFileName, 0, sizeof(NewFileName));
		strcat_s(NewFileName, AnimationFileName);
		strcat_s(NewFileName, ".MD5ANIM");
	OpenAnimationFromFile(AnimationID, NewFileName);

	if (AnimationSpeed <= 0.0f)
		AnimationSpeed = 0.02f;

	ModelAnimation[AnimationID].BasicAnimSpeed = AnimationSpeed;

	// 애니메이션 개수 1개 증가
	TotalAnimCount++;

	return true;
}

bool ModelMD5::InstanceSetAnimation(int InstanceID, int AnimationID, float StartAnimTime)
{
	if (AnimationID > TotalAnimCount)
		AnimationID = 0;

	ModelInstances[InstanceID].CurAnimID = AnimationID;

	if (StartAnimTime > ModelAnimation[AnimationID].TotalAnimTime)
		StartAnimTime = 0;

	ModelInstances[InstanceID].CurAnimTime = StartAnimTime;

	return true;
}

void ModelMD5::AddInstance(XMFLOAT3 Translation, XMFLOAT3 Rotation, XMFLOAT3 Scaling)
{
	numInstances++;

	memset(&ModelInstances[numInstances-1], 0, sizeof(ModelInstances[numInstances-1]));

	ModelInstances[numInstances-1].Translation = Translation;
	ModelInstances[numInstances-1].Rotation = Rotation;
	ModelInstances[numInstances-1].Scaling = Scaling;

	return;
}

void ModelMD5::SetInstance(int InstanceID, XMFLOAT3 Translation, XMFLOAT3 Rotation, XMFLOAT3 Scaling)
{
	ModelInstances[InstanceID].Translation = Translation;
	ModelInstances[InstanceID].Rotation = Rotation;
	ModelInstances[InstanceID].Scaling = Scaling;

	return;
}

void ModelMD5::SetBaseDirection(char* Dir)
{
	strcpy_s(BaseDir, Dir);
	return;
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
		strcpy_s(sLine, StringTrim(sLine));					// 앞 뒤 공백 제거
		int iLen = strlen(sLine);

		if (iLen <= 1)						// 빈 줄이면 다음 줄로 넘어가자! (개행문자 때문에 최소 길이가 1이다.)★
			continue;


		// 중괄호 열기 전
		if ( FindString_MD5(sLine, MD5KeyWords[0]) )	// MD5Version 4843 & MD5Version 10 지원 ★★
		{
			GetFloatFromLine(sLine, " ", TabChar);
			MD5Version = (int)ParseFloats[1];
			continue;
		}

		if ( FindString_MD5(sLine, MD5KeyWords[1]) )	// commandline "~"
		{
			continue;								// 주석은 건너뛰자
		}

		if ( FindString_MD5(sLine, MD5KeyWords[2]) )	// numJoints ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			numJoints = (int)ParseFloats[1];
			continue;
		}

		if ( FindString_MD5(sLine, MD5KeyWords[3]) )	// numMeshes ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			numMeshes = (int)ParseFloats[1];
			continue;
		}

		if ( FindString_MD5(sLine, MD5KeyWords[4]) )	// numObjects ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			numObjects = (int)ParseFloats[1];
			continue;
		}

		if ( FindString_MD5(sLine, MD5KeyWords[5]) )	// numMaterials ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			numMaterials = (int)ParseFloats[1];
			continue;
		}

		if ( FindString_MD5(sLine, MD5KeyWords[6]) )	// joints {
		{
			strcpy_s(CurKey, MD5KeyWords[6]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}

		if ( FindString_MD5(sLine, MD5KeyWords[7]) )	// objects {
		{
			strcpy_s(CurKey, MD5KeyWords[7]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}

		if ( FindString_MD5(sLine, MD5KeyWords[8]) )	// materials {
		{
			strcpy_s(CurKey, MD5KeyWords[8]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}

		if ( FindString_MD5(sLine, MD5KeyWords[9]) )	// mesh {
		{
			MeshCount++;
			strcpy_s(CurKey, MD5KeyWords[9]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}


		// 중괄호 열린 상태
		if ( FindString_MD5(CurKey, MD5KeyWords[6]) )	// joints {
		{
			CurKeyCount++;

			MD5Joint	TempJoint;

				// joint의 이름에 공백이 들어가 있는 경우에 대비해 이름 읽어온 후 지우고 float값을 얻어온다.
				char tempName[MAX_NAME_LEN];
				memset(tempName, 0, sizeof(tempName));
				strcpy_s( tempName, GetNameBetweenQuotes(sLine) );
				strcpy_s( TempJoint.Name, tempName );
				int	tempLen = strlen(tempName) + 2;	// 이름 & 앞뒤 쌍따옴표 제거하기 위함

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

		if ( FindString_MD5(CurKey, MD5KeyWords[7]) )		// objects {
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

		if ( FindString_MD5(CurKey, MD5KeyWords[8]) )		// materials {
		{
			CurKeyCount++;
			strcpy_s( ModelMaterials[CurKeyCount-1].Name, SplitString(sLine, "\"", 0) );
			strcpy_s( ModelMaterials[CurKeyCount-1].TextureFileName, SplitString(sLine, "\"", 2) );

			if (CurKeyCount >= numMaterials)
				memset(CurKey, 0, sizeof(CurKey));		// 중괄호 닫기

			continue;
		}

		if ( FindString_MD5(CurKey, MD5KeyWords[9]) )		// mesh {
		{
			if ( FindString_MD5(sLine, MD5KeyWords[10]) )	// '//' 주석 줄
				continue;								// 건너뛰자!

			if ( FindString_MD5(sLine, MD5KeyWords[11]) )	// meshindex
				continue;

			if ( FindString_MD5(sLine, MD5KeyWords[12]) )	// shader
			{
				if ( MD5Version == 4843 )	// 신버전이면 material의 이름이 나옴!★
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
			if ( FindString_MD5(sLine, MD5KeyWords[13]) )	// numverts
			{
				GetFloatFromLine(sLine, " ", TabChar);
				numMeshVertices[MeshCount-1] = (int)ParseFloats[1];

				continue;
			}
			if ( FindString_MD5(sLine, MD5KeyWords[14]) )	// vert
			{
				GetFloatFromLine(sLine, " ", TabChar);
				int VIndex = (int)ParseFloats[1];

				ModelMeshes[MeshCount-1].Vertices[VIndex].texCoord.x = ParseFloats[3];
				ModelMeshes[MeshCount-1].Vertices[VIndex].texCoord.y = ParseFloats[4];
				ModelMeshes[MeshCount-1].VertexWeightStart[VIndex] = (int)ParseFloats[6];
				ModelMeshes[MeshCount-1].VertexNumWeights[VIndex] = (int)ParseFloats[7];

				continue;
			}
			if ( FindString_MD5(sLine, MD5KeyWords[15]) )	// numtris
			{
				GetFloatFromLine(sLine, " ", TabChar);
				numMeshIndices[MeshCount-1] = (int)ParseFloats[1];

				continue;
			}
			if ( FindString_MD5(sLine, MD5KeyWords[16]) )	// tri
			{
				GetFloatFromLine(sLine, " ", TabChar);
				int TIndex = (int)ParseFloats[1];

				ModelMeshes[MeshCount-1].Indices[TIndex]._0 = (int)ParseFloats[2];
				ModelMeshes[MeshCount-1].Indices[TIndex]._1 = (int)ParseFloats[3];
				ModelMeshes[MeshCount-1].Indices[TIndex]._2 = (int)ParseFloats[4];

				continue;
			}
			if ( FindString_MD5(sLine, MD5KeyWords[17]) )	// numweights
			{
				GetFloatFromLine(sLine, " ", TabChar);
				numWeights[MeshCount-1] = (int)ParseFloats[1];
				continue;
			}
			if ( FindString_MD5(sLine, MD5KeyWords[18]) )	// weight
			{
				GetFloatFromLine(sLine, " ", TabChar);
				int WIndex = (int)ParseFloats[1];
				ModelMeshes[MeshCount-1].Weights[WIndex].JointID = (int)ParseFloats[2];

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

			XMFLOAT3 Result	= XMFLOAT3(0.0f, 0.0f, 0.0f);

			for (int k = 0; k < nWeights; k++)
			{
				int CurJointID = ModelMeshes[i].Weights[WeightStart+k].JointID;
				MD5Joint	TempJoint	= ModelJoints[CurJointID];
				MD5Weight	TempWeight	= ModelMeshes[i].Weights[WeightStart+k];

				// 사원수(Quaternion)을 사용해 정점을 회전시킨다. (사원수 * 정점 * 사원수의 역)
				XMVECTOR	Q1, POS, Q2;
				XMFLOAT3	Rotated;

				Q1	= XMVectorSet(TempJoint.Orientation.x, TempJoint.Orientation.y, TempJoint.Orientation.z, TempJoint.Orientation.w);
				POS	= XMVectorSet(TempWeight.Position.x, TempWeight.Position.y, TempWeight.Position.z, 0.0f);
				Q2	= XMVectorSet(-TempJoint.Orientation.x, -TempJoint.Orientation.y, -TempJoint.Orientation.z, TempJoint.Orientation.w);

				XMStoreFloat3( &Rotated, XMQuaternionMultiply( XMQuaternionMultiply( Q1, POS ), Q2 ) );

				Result.x += (TempJoint.Position.x + Rotated.x) * TempWeight.Bias;
				Result.y += (TempJoint.Position.y + Rotated.y) * TempWeight.Bias;
				Result.z += (TempJoint.Position.z + Rotated.z) * TempWeight.Bias;
			}
			
			ModelMeshes[i].Vertices[j].pos = Result;
		}
	}


	// 법선 계산 ★★★
	for (int i = 0; i < numMeshes; i++)
	{
		for(int j = 0; j < numMeshVertices[i]; ++j)
		{
			ModelMeshes[i].Vertices[j].normal.x = 0.0f;
			ModelMeshes[i].Vertices[j].normal.y = 0.0f;
			ModelMeshes[i].Vertices[j].normal.z = 0.0f;
		}
	}

	for (int i = 0; i < numMeshes; i++)
	{
		XMFLOAT3 tempNormal[MAX_MESH_VERTICES];
		XMFLOAT3 unnormalized = XMFLOAT3(0.0f, 0.0f, 0.0f);

		float vecX, vecY, vecZ;

		XMVECTOR edge1 = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
		XMVECTOR edge2 = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

		for(int j = 0; j < numMeshIndices[i]; ++j)
		{
			//Get the vector describing one edge of our triangle (edge 0,2)
			vecX = ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._0].pos.x - ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._2].pos.x;
			vecY = ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._0].pos.y - ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._2].pos.y;
			vecZ = ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._0].pos.z - ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._2].pos.z;		
			edge1 = XMVectorSet(vecX, vecY, vecZ, 0.0f);	//Create our first edge

			//Get the vector describing another edge of our triangle (edge 2,1)
			vecX = ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._2].pos.x - ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._1].pos.x;
			vecY = ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._2].pos.y - ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._1].pos.y;
			vecZ = ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._2].pos.z - ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._1].pos.z;
			edge2 = XMVectorSet(vecX, vecY, vecZ, 0.0f);	//Create our second edge

			//Cross multiply the two edge vectors to get the un-normalized face normal
			XMStoreFloat3(&unnormalized, XMVector3Cross(edge1, edge2));

			tempNormal[j] = unnormalized;
		}

		//Compute vertex normals (normal Averaging)
		XMVECTOR normalSum = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
		int facesUsing = 0;
		float tX, tY, tZ;	//temp axis variables

		for(int j = 0; j < numMeshVertices[i]; ++j)
		{
			for(int k = 0; k < numMeshIndices[i]; ++k)
			{
				if( ModelMeshes[i].Indices[k]._0 == j || ModelMeshes[i].Indices[k]._1 == j || ModelMeshes[i].Indices[k]._2 == j )
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

			ModelMeshes[i].Vertices[j].normal.x = -XMVectorGetX(normalSum);
			ModelMeshes[i].Vertices[j].normal.y = -XMVectorGetY(normalSum);
			ModelMeshes[i].Vertices[j].normal.z = -XMVectorGetZ(normalSum);					

			ANYVERTEX tempVert = ModelMeshes[i].Vertices[j];			// Get the current vertex
			XMVECTOR normal = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);		// Clear normal

			int WeightStart = ModelMeshes[i].VertexWeightStart[j];
			int nWeights = ModelMeshes[i].VertexNumWeights[j];

			for ( int k = 0; k < nWeights; k++)				// Loop through each of the vertices weights
			{
				MD5Joint tempJoint = ModelJoints[ModelMeshes[i].Weights[WeightStart + k].JointID];	// Get the joints orientation
				XMVECTOR jointOrientation = XMVectorSet(tempJoint.Orientation.x, tempJoint.Orientation.y, tempJoint.Orientation.z, tempJoint.Orientation.w);

				normal = XMQuaternionMultiply(XMQuaternionMultiply(XMQuaternionInverse(jointOrientation), normalSum), jointOrientation);		

				XMStoreFloat3(&ModelMeshes[i].Weights[WeightStart + k].Normal, XMVector3Normalize(normal));			// Store the normalized quaternion into our weights normal
			}				

			normalSum = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
			facesUsing = 0;
		}

	}


	fclose(fp);

	return true;
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
		if ( FindString_MD5(sLine, MD5KeyWords[0]) )	// MD5Version 4843 & 10
		{
			GetFloatFromLine(sLine, " ", TabChar);
			// if ( MD5Version != (int)ParseFloats[1]);	// 모델과 애니메이션의 버전이 달라도 그냥 불러와 보자..
			continue;
		}

		if ( FindString_MD5(sLine, MD5KeyWords[1]) )	// commandline "~"
		{
			continue;								// 주석은 건너뛰자
		}

		if ( FindString_MD5(sLine, MD5KeyWords[19]) )	// numFrames ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			ModelAnimation[AnimationID].numFrames = (int)ParseFloats[1];
			continue;
		}

		if ( FindString_MD5(sLine, MD5KeyWords[2]) )	// numJoints ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			ModelAnimation[AnimationID].numJoints = (int)ParseFloats[1];
			if (ModelAnimation[AnimationID].numJoints != numJoints)	// 애니메이션의 numJoints가 불러온 모델의 numJoints와 다를 경우
				return false;										// 불러오기를 중단한다. ★★
			continue;
		}
		if ( FindString_MD5(sLine, MD5KeyWords[20]) )	// frameRate ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			ModelAnimation[AnimationID].FrameRate = (int)ParseFloats[1];
			continue;
		}
		if ( FindString_MD5(sLine, MD5KeyWords[21]) )	// numAnimatedComponents ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			ModelAnimation[AnimationID].numAnimatedComponents = (int)ParseFloats[1];
			continue;
		}
		if ( FindString_MD5(sLine, MD5KeyWords[22]) )	// hierarchy {
		{
			strcpy_s(CurKey, MD5KeyWords[22]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}
		if ( FindString_MD5(sLine, MD5KeyWords[23]) )	// bounds {
		{
			strcpy_s(CurKey, MD5KeyWords[23]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}
		if ( FindString_MD5(sLine, MD5KeyWords[24]) )	// baseframe {
		{
			strcpy_s(CurKey, MD5KeyWords[24]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}
		if ( FindString_MD5(sLine, MD5KeyWords[25]) )	// frame ~ {
		{
			FrameCount++;
			GetFloatFromLine(sLine, " ", TabChar);
			TempFrameID = (int)ParseFloats[1];

			strcpy_s(CurKey, MD5KeyWords[25]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}


		// 중괄호 열린 상태
		if ( FindString_MD5(CurKey, MD5KeyWords[22]) )	// hierarchy {
		{
			CurKeyCount++;

			// hierarchy(즉, joint)의 이름에 공백이 들어가 있는 경우에 대비해 이름을 읽어온 후 지워준 후 float값을 얻어온다.
			char tempName[MAX_NAME_LEN];
			memset(tempName, 0, sizeof(tempName));
			strcpy_s( tempName, SplitString(sLine, "\"", 0) );
			strcpy_s( ModelAnimation[AnimationID].JointInfo[CurKeyCount-1].Name, tempName );
			int	tempLen = strlen(tempName) + 2;		// 이름 & 앞뒤 쌍따옴표 제거하기 위함

			char tempLine[MAX_PARSE_LINE];
			memset(tempLine, 0, sizeof(tempLine));
			int	tempLineLen = strlen(sLine);
			for (int i = 0; i < tempLineLen-tempLen; i++)
			{
				tempLine[i] = sLine[i+tempLen];
			}
			
			GetFloatFromLine(tempLine, " ", TabChar);
			ModelAnimation[AnimationID].JointInfo[CurKeyCount-1].ParentID	= (int)ParseFloats[0];
			ModelAnimation[AnimationID].JointInfo[CurKeyCount-1].Flags		= (int)ParseFloats[1];
			ModelAnimation[AnimationID].JointInfo[CurKeyCount-1].StartIndex	= (int)ParseFloats[2];

			if (CurKeyCount >= ModelAnimation[AnimationID].numJoints)
				memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기

			continue;
		}

		if ( FindString_MD5(CurKey, MD5KeyWords[23]) )	// bounds {
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

			ModelAnimation[AnimationID].BoundingBoxes[CurKeyCount-1] = TempBB;

			if (CurKeyCount >= ModelAnimation[AnimationID].numFrames)
				memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기

			continue;
		}

		if ( FindString_MD5(CurKey, MD5KeyWords[24]) )	// baseframe {
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

			ModelAnimation[AnimationID].JointBaseFrame[CurKeyCount-1] = TempBaseFrame;

			if (CurKeyCount >= ModelAnimation[AnimationID].numJoints)
				memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기

			continue;
		}

		if ( FindString_MD5(CurKey, MD5KeyWords[25]) )	// frame ~ {
		{
			// FRAME: 각 Frame이 Baseframe에서 변화한 위치!★
			MD5Frame	TempFrameData;
			memset(&TempFrameData, 0, sizeof(TempFrameData));
			TempFrameData.FrameID = TempFrameID;

			for(int i = 0; i < ModelAnimation[AnimationID].numJoints; i++)
			{
				if (ModelAnimation[AnimationID].JointInfo[i].Flags == 0)	 // 플래그가 0이면 건너뛰자!★
					continue;

				GetFloatFromLine(sLine, " ", TabChar);

				int k = ModelAnimation[AnimationID].JointInfo[i].StartIndex;

				if (ModelAnimation[AnimationID].numAnimatedComponents < 6)
				{
					for (int j = 0; j < ModelAnimation[AnimationID].numAnimatedComponents; j++)
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

			ModelAnimation[AnimationID].FrameData[FrameCount-1] = TempFrameData;


			for (int i = 0; i < ModelAnimation[AnimationID].numJoints; i++)
			{
				MD5Joint TempFrameJoint = ModelAnimation[AnimationID].JointBaseFrame[i];	// 기본적으로 좌표는 BaseFrame 값에서 출발!
				TempFrameJoint.ParentID = ModelAnimation[AnimationID].JointInfo[i].ParentID;
				
				int tempJointIDStart = ModelAnimation[AnimationID].JointInfo[i].StartIndex;

				if(ModelAnimation[AnimationID].JointInfo[i].Flags & 1)	//	Position.x	( 000001 )
					TempFrameJoint.Position.x		= TempFrameData.JointData[tempJointIDStart++];

				if(ModelAnimation[AnimationID].JointInfo[i].Flags & 2)	//	Position.y	( 000010 )
					TempFrameJoint.Position.z		= TempFrameData.JointData[tempJointIDStart++];	// 3DSMAX -> DIRECTX 좌표계!★

				if(ModelAnimation[AnimationID].JointInfo[i].Flags & 4)	//	Position.z	( 000100 )
					TempFrameJoint.Position.y		= TempFrameData.JointData[tempJointIDStart++];

				if(ModelAnimation[AnimationID].JointInfo[i].Flags & 8)	//	Orientation.x	( 001000 )
					TempFrameJoint.Orientation.x	= TempFrameData.JointData[tempJointIDStart++];

				if(ModelAnimation[AnimationID].JointInfo[i].Flags & 16)	//	Orientation.y	( 010000 )
					TempFrameJoint.Orientation.z	= TempFrameData.JointData[tempJointIDStart++];	// 3DSMAX -> DIRECTX 좌표계!★

				if(ModelAnimation[AnimationID].JointInfo[i].Flags & 32)	//	Orientation.z	( 100000 )
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
					MD5Joint	ParentJoint = ModelAnimation[AnimationID].FrameSekelton[FrameCount-1].Skeleton[TempFrameJoint.ParentID];

					XMVECTOR	Q1, POS, Q2;
					XMFLOAT3	RotatedPos;

					Q1	= XMVectorSet(ParentJoint.Orientation.x, ParentJoint.Orientation.y, ParentJoint.Orientation.z, ParentJoint.Orientation.w);
					POS	= XMVectorSet(TempFrameJoint.Position.x, TempFrameJoint.Position.y, TempFrameJoint.Position.z, 0.0f);
					Q2	= XMVectorSet(-ParentJoint.Orientation.x, -ParentJoint.Orientation.y, -ParentJoint.Orientation.z, ParentJoint.Orientation.w);

					XMStoreFloat3( &RotatedPos, XMQuaternionMultiply( XMQuaternionMultiply( Q1, POS ), Q2 ) );

					TempFrameJoint.Position.x = RotatedPos.x + ParentJoint.Position.x;
					TempFrameJoint.Position.y = RotatedPos.y + ParentJoint.Position.y;
					TempFrameJoint.Position.z = RotatedPos.z + ParentJoint.Position.z;

					XMVECTOR	TempJointOrient;
					TempJointOrient = XMVectorSet(TempFrameJoint.Orientation.x, TempFrameJoint.Orientation.y, TempFrameJoint.Orientation.z, TempFrameJoint.Orientation.w);
					TempJointOrient = XMQuaternionMultiply(Q1, TempJointOrient);
					TempJointOrient = XMQuaternionNormalize(TempJointOrient);
					
					XMStoreFloat4(&TempFrameJoint.Orientation, TempJointOrient);
				}

				ModelAnimation[AnimationID].FrameSekelton[FrameCount-1].Skeleton[i] = TempFrameJoint;
			}

			memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기
			continue;
		}
	}

	// 변수 초기화
	ModelAnimation[AnimationID].FrameTime = 1.0f / ModelAnimation[AnimationID].FrameRate;			// 각 프레임(Frame) 당 시간 설정 ★
	ModelAnimation[AnimationID].TotalAnimTime =
		ModelAnimation[AnimationID].numFrames * ModelAnimation[AnimationID].FrameTime;				// 애니메이션 전체 길이

	return true;
}


void ModelMD5::InstanceAnimate(int InstanceID, float Speed)
{
	if (Speed == 0.0f)
		Speed = ModelAnimation[ModelInstances[InstanceID].CurAnimID].BasicAnimSpeed;	// 속도 지정을 0으로 하면 기본 속도로 재생

	ModelInstances[InstanceID].CurAnimTime += Speed;

	if(ModelInstances[InstanceID].CurAnimTime > ModelAnimation[ModelInstances[InstanceID].CurAnimID].TotalAnimTime)	// 애니메이션이 끝나면
		ModelInstances[InstanceID].CurAnimTime = 0.0f;																// 다시 처음으로! ★

	float CurrentFrame =
		ModelInstances[InstanceID].CurAnimTime * ModelAnimation[ModelInstances[InstanceID].CurAnimID].FrameRate;	// 현재 프레임
	int Frame0 = (int)floorf( CurrentFrame );															// 보간을 위한 현재 프레임 값
	int Frame1 = Frame0 + 1;																			// 보간을 위한 다음 프레임 값

	if(Frame0 == ModelAnimation[ModelInstances[InstanceID].CurAnimID].numFrames-1)	// Frame0이 마지막 프레임이라면 Frame1은 다시 0번으로 가자!
		Frame1 = 0;

	float Interpolation = CurrentFrame - Frame0;			// Frame0과 CurrentFrame의 시간차를 얻어와 보간값으로 사용한다.

	MD5Joint InterpolatedJoints[MAX_JOINTS];

	// 보간값(Interpolation) 계산
	for( int i = 0; i < ModelAnimation[ModelInstances[InstanceID].CurAnimID].numJoints; i++)
	{
		MD5Joint TempJoint;
		MD5Joint Joint0 = ModelAnimation[ModelInstances[InstanceID].CurAnimID].FrameSekelton[Frame0].Skeleton[i];	// Frame0의 i번째 JointData
		MD5Joint Joint1 = ModelAnimation[ModelInstances[InstanceID].CurAnimID].FrameSekelton[Frame1].Skeleton[i];	// Frame1의 i번째 JointData

		TempJoint.Position.x = Joint0.Position.x + (Interpolation * (Joint1.Position.x - Joint0.Position.x));
		TempJoint.Position.y = Joint0.Position.y + (Interpolation * (Joint1.Position.y - Joint0.Position.y));
		TempJoint.Position.z = Joint0.Position.z + (Interpolation * (Joint1.Position.z - Joint0.Position.z));

		XMVECTOR Joint0Orient = XMVectorSet(Joint0.Orientation.x, Joint0.Orientation.y, Joint0.Orientation.z, Joint0.Orientation.w);
		XMVECTOR Joint1Orient = XMVectorSet(Joint1.Orientation.x, Joint1.Orientation.y, Joint1.Orientation.z, Joint1.Orientation.w);

		XMStoreFloat4( &TempJoint.Orientation, XMQuaternionSlerp(Joint0Orient, Joint1Orient, Interpolation) );	// 구면 선형 보간

		InterpolatedJoints[i] = TempJoint;		// 결과 값을 저장한다!
	}

	// 정점 정보 업데이트
	for (int i = 0; i < numMeshes; i++)
	{
		for (int j = 0; j < numMeshVertices[i]; j++)
		{
			ANYVERTEX	TempVert = ModelMeshes[i].Vertices[j];
			memset(&TempVert.pos, 0, sizeof(TempVert.pos));			// Position 값을 0으로 초기화!
			memset(&TempVert.normal, 0, sizeof(TempVert.normal));	// Normal 값을 0으로 초기화!

			int WeightStart = ModelMeshes[i].VertexWeightStart[j];
			int nWeights = ModelMeshes[i].VertexNumWeights[j];

			float ResultX = 0.0f;
			float ResultY = 0.0f;
			float ResultZ = 0.0f;

			for (int k = 0; k < nWeights; k++)
			{
				MD5Weight	TempWeight	= ModelMeshes[i].Weights[WeightStart+k];
				MD5Joint	TempJoint	= InterpolatedJoints[TempWeight.JointID];
				
				XMVECTOR	Q1, POS, Q2;
				XMFLOAT4	Rotated;

				Q1	= XMVectorSet(TempJoint.Orientation.x, TempJoint.Orientation.y, TempJoint.Orientation.z, TempJoint.Orientation.w);
				POS	= XMVectorSet(TempWeight.Position.x, TempWeight.Position.y, TempWeight.Position.z, 0.0f);
				Q2	= XMVectorSet(-TempJoint.Orientation.x, -TempJoint.Orientation.y, -TempJoint.Orientation.z, TempJoint.Orientation.w);

				XMStoreFloat4( &Rotated, XMQuaternionMultiply( XMQuaternionMultiply( Q1, POS ), Q2 ) );

				TempVert.pos.x += ( TempJoint.Position.x + Rotated.x ) * TempWeight.Bias;
				TempVert.pos.y += ( TempJoint.Position.y + Rotated.y ) * TempWeight.Bias;
				TempVert.pos.z += ( TempJoint.Position.z + Rotated.z ) * TempWeight.Bias;


				XMVECTOR TempWeightNormal = XMVectorSet(TempWeight.Normal.x, TempWeight.Normal.y, TempWeight.Normal.z, 0.0f);
				XMStoreFloat4( &Rotated, XMQuaternionMultiply(XMQuaternionMultiply( Q1, TempWeightNormal), Q2 ) );

				TempVert.normal.x -= Rotated.x * TempWeight.Bias;
				TempVert.normal.y -= Rotated.y * TempWeight.Bias;
				TempVert.normal.z -= Rotated.z * TempWeight.Bias;
			}
			
			ModelMeshes[i].Vertices[j].pos = TempVert.pos;
			ModelMeshes[i].Vertices[j].normal = TempVert.normal;		// Store the vertices normal
			XMStoreFloat3(&ModelMeshes[i].Vertices[j].normal, XMVector3Normalize(XMLoadFloat3(&ModelMeshes[i].Vertices[j].normal)));
		}
	}
}

HRESULT ModelMD5::UpdateVertices(LPDIRECT3DDEVICE9 D3DDevice, int MeshIndex)
{
	if( g_pModelVB != NULL )
		g_pModelVB->Release();

	if( g_pModelIB != NULL )
		g_pModelIB->Release();

	// 정점 버퍼 업데이트!
	ANYVERTEX *NewVertices = new ANYVERTEX[numMeshVertices[MeshIndex]];

		for (int i = 0; i < numMeshVertices[MeshIndex]; i++)
		{
			NewVertices[i].pos.x = ModelMeshes[MeshIndex].Vertices[i].pos.x;
			NewVertices[i].pos.y = ModelMeshes[MeshIndex].Vertices[i].pos.y;
			NewVertices[i].pos.z = ModelMeshes[MeshIndex].Vertices[i].pos.z;
			NewVertices[i].texCoord.x = ModelMeshes[MeshIndex].Vertices[i].texCoord.x;
			NewVertices[i].texCoord.y = ModelMeshes[MeshIndex].Vertices[i].texCoord.y;
			NewVertices[i].normal.x = ModelMeshes[MeshIndex].Vertices[i].normal.x;
			NewVertices[i].normal.y = ModelMeshes[MeshIndex].Vertices[i].normal.y;
			NewVertices[i].normal.z = ModelMeshes[MeshIndex].Vertices[i].normal.z;
		}

		int SizeOfVertices = sizeof(ANYVERTEX)*numMeshVertices[MeshIndex];
		if (FAILED(D3DDevice->CreateVertexBuffer(SizeOfVertices, 0, D3DFVF_ANYVERTEX, D3DPOOL_DEFAULT, &g_pModelVB, NULL)))
			return E_FAIL;
	
		VOID* pVertices;
		if (FAILED(g_pModelVB->Lock(0, SizeOfVertices, (void**)&pVertices, 0)))
			return E_FAIL;
		memcpy(pVertices, NewVertices, SizeOfVertices);
		g_pModelVB->Unlock();

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
		if (FAILED(D3DDevice->CreateIndexBuffer(SizeOfIndices, 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &g_pModelIB, NULL)))
			return E_FAIL;

		VOID* pIndices;
		if (FAILED(g_pModelIB->Lock(0, SizeOfIndices, (void **)&pIndices, 0)))
			return E_FAIL;
		memcpy(pIndices, NewIndices, SizeOfIndices);
		g_pModelIB->Unlock();
	
	delete[] NewIndices;

	return S_OK;	// 함수 종료!
}

HRESULT ModelMD5::SetTexture(LPDIRECT3DDEVICE9 D3DDevice, int MeshIndex)
{
	if ( ModelTextures[MeshIndex] != NULL)
		ModelTextures[MeshIndex]->Release();

	// 텍스처 불러오기
	char	NewFileName[MAX_NAME_LEN] = {0};
	strcpy_s(NewFileName, BaseDir);
	strcat_s(NewFileName, ModelMaterials[MeshIndex].TextureFileName);

	D3DXCreateTextureFromFile(D3DDevice, NewFileName, &ModelTextures[MeshIndex]);

	return S_OK;
}

void ModelMD5::DrawModel(LPDIRECT3DDEVICE9 D3DDevice)
{
	// 모델의 각 메쉬를 그린다!
	for (int i = 0; i < numMeshes; i++)
	{
		UpdateVertices(D3DDevice, i);

		// 재질을 설정한다.
		D3DMATERIAL9 mtrl;
		ZeroMemory( &mtrl, sizeof( D3DMATERIAL9 ) );
		mtrl.Diffuse.r = mtrl.Ambient.r = 1.0f;
		mtrl.Diffuse.g = mtrl.Ambient.g = 1.0f;
		mtrl.Diffuse.b = mtrl.Ambient.b = 1.0f;
		mtrl.Diffuse.a = mtrl.Ambient.a = 1.0f;
		D3DDevice->SetMaterial( &mtrl );

		D3DDevice->SetTexture(0, ModelTextures[i]);
		D3DDevice->SetStreamSource(0, g_pModelVB, 0, sizeof(ANYVERTEX));
		D3DDevice->SetFVF(D3DFVF_ANYVERTEX);
		D3DDevice->SetIndices(g_pModelIB);

		D3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numMeshVertices[i], 0, numMeshIndices[i]);
	}

	return;
}

void ModelMD5::Destroy()
{	
	for (int i = 0; i < numMeshes; i++)
	{
		if ( ModelTextures[i] != NULL)
			ModelTextures[i]->Release();
	}

	return;
}