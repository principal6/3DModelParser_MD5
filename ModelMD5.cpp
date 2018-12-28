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

Joint_MD5		ModelJoints[MAX_JOINTS];
Object_MD5		ModelObjects[MAX_OBJECTS];
Material_MD5	ModelMaterials[MAX_MATERIALS];
Mesh_MD5		ModelMeshes[MAX_MESHES];


// 기타 변수
char		TabChar[] = {'\t'};


ModelMD5::ModelMD5()
{
	// 변수 초기화
	memset(ModelJoints, 0, sizeof(ModelJoints));
	memset(ModelObjects, 0, sizeof(ModelObjects));
	memset(ModelMaterials, 0, sizeof(ModelMaterials));
	memset(ModelMeshes, 0, sizeof(ModelMeshes));
	memset(&ModelAnimation, 0, sizeof(ModelAnimation));

	memset(&ModelInstances, 0, sizeof(ModelInstances));
	memset(MouseOverPerInstances, 0, sizeof(MouseOverPerInstances));
	memset(DistanceCmp, 0, sizeof(DistanceCmp));
	memset(PickedPosition, 0, sizeof(PickedPosition));

	memset(numMeshVertices, 0, sizeof(numMeshVertices));
	memset(numMeshIndices, 0, sizeof(numMeshIndices));
	memset(numWeights, 0, sizeof(numWeights));

	Version_MD5		= 0;
	numMeshes		= 0;
	numJoints		= 0;
	numObjects		= 0;
	numMaterials	= 0;
	TotalAnimCount	= 0;
	numInstances	= 0;
}

ModelMD5::~ModelMD5()
{
	for (int i = 0; i < numMeshes; i++)
	{
		SAFE_RELEASE(ModelTextures[i]);
	}
}

bool ModelMD5::CreateModel(LPDIRECT3DDEVICE9 D3DDevice, char* BaseDir, char* FileNameWithoutExtension)
{
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

	// 바운딩 박스 생성하기
	CreateMeshBoundingBoxes();

	return true;
}

void ModelMD5::CreateMeshBoundingBoxes()
{
	XMFLOAT3 Min = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 Max = XMFLOAT3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i < numMeshes; i++)
	{
		Min = ModelMeshes[i].Vertices[0].Position;
		Max = ModelMeshes[i].Vertices[0].Position;

		for (int j = 0; j < numMeshVertices[i]; j++)
		{
			if ( Min.x > ModelMeshes[i].Vertices[j].Position.x)
				Min.x = ModelMeshes[i].Vertices[j].Position.x;
			if ( Min.y > ModelMeshes[i].Vertices[j].Position.y)
				Min.y = ModelMeshes[i].Vertices[j].Position.y;
			if ( Min.z > ModelMeshes[i].Vertices[j].Position.z)
				Min.z = ModelMeshes[i].Vertices[j].Position.z;

			if ( Max.x < ModelMeshes[i].Vertices[j].Position.x)
				Max.x = ModelMeshes[i].Vertices[j].Position.x;
			if ( Max.y < ModelMeshes[i].Vertices[j].Position.y)
				Max.y = ModelMeshes[i].Vertices[j].Position.y;
			if ( Max.z < ModelMeshes[i].Vertices[j].Position.z)
				Max.z = ModelMeshes[i].Vertices[j].Position.z;
		}

		ModelMeshes[i].BB.Min = Min;
		ModelMeshes[i].BB.Max = Max;
	}

	return;
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

	if (StartAnimTime < 0)	// -값이 들어오면 시작 시간은 초기화하지 않는다.
		return true;

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
			continue;								// 주석은 건너뛰자
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
			strcpy_s(CurKey, KeyWords_MD5[6]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[7]) )	// objects {
		{
			strcpy_s(CurKey, KeyWords_MD5[7]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[8]) )	// materials {
		{
			strcpy_s(CurKey, KeyWords_MD5[8]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[9]) )	// mesh {
		{
			MeshCount++;
			strcpy_s(CurKey, KeyWords_MD5[9]);		// 중괄호 열림!
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

		if ( FindString_MD5(CurKey, KeyWords_MD5[7]) )		// objects {
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

		if ( FindString_MD5(CurKey, KeyWords_MD5[8]) )		// materials {
		{
			CurKeyCount++;
			strcpy_s( ModelMaterials[CurKeyCount-1].Name, SplitString(sLine, "\"", 0) );
			strcpy_s( ModelMaterials[CurKeyCount-1].TextureFileName, SplitString(sLine, "\"", 2) );

			if (CurKeyCount >= numMaterials)
				memset(CurKey, 0, sizeof(CurKey));		// 중괄호 닫기

			continue;
		}

		if ( FindString_MD5(CurKey, KeyWords_MD5[9]) )		// mesh {
		{
			if ( FindString_MD5(sLine, KeyWords_MD5[10]) )	// '//' 주석 줄
				continue;								// 건너뛰자!

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
				numMeshVertices[MeshCount-1] = (int)ParseFloats[1];

				continue;
			}
			if ( FindString_MD5(sLine, KeyWords_MD5[14]) )	// vert
			{
				GetFloatFromLine(sLine, " ", TabChar);
				int VIndex = (int)ParseFloats[1];

				ModelMeshes[MeshCount-1].Vertices[VIndex].Texture.x = ParseFloats[3];
				ModelMeshes[MeshCount-1].Vertices[VIndex].Texture.y = ParseFloats[4];
				ModelMeshes[MeshCount-1].VertexWeightStart[VIndex] = (int)ParseFloats[6];
				ModelMeshes[MeshCount-1].VertexNumWeights[VIndex] = (int)ParseFloats[7];

				continue;
			}
			if ( FindString_MD5(sLine, KeyWords_MD5[15]) )	// numtris
			{
				GetFloatFromLine(sLine, " ", TabChar);
				numMeshIndices[MeshCount-1] = (int)ParseFloats[1];

				continue;
			}
			if ( FindString_MD5(sLine, KeyWords_MD5[16]) )	// tri
			{
				GetFloatFromLine(sLine, " ", TabChar);
				int TIndex = (int)ParseFloats[1];

				ModelMeshes[MeshCount-1].Indices[TIndex]._0 = (int)ParseFloats[2];
				ModelMeshes[MeshCount-1].Indices[TIndex]._1 = (int)ParseFloats[3];
				ModelMeshes[MeshCount-1].Indices[TIndex]._2 = (int)ParseFloats[4];

				continue;
			}
			if ( FindString_MD5(sLine, KeyWords_MD5[17]) )	// numweights
			{
				GetFloatFromLine(sLine, " ", TabChar);
				numWeights[MeshCount-1] = (int)ParseFloats[1];
				continue;
			}
			if ( FindString_MD5(sLine, KeyWords_MD5[18]) )	// weight
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
				Joint_MD5	TempJoint	= ModelJoints[CurJointID];
				Weight_MD5	TempWeight	= ModelMeshes[i].Weights[WeightStart+k];

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

			ModelMeshes[i].Vertices[j].Position = Result;
		}
	}


	// 법선 계산 ★★★
	for (int i = 0; i < numMeshes; i++)
	{
		for(int j = 0; j < numMeshVertices[i]; ++j)
		{
			ModelMeshes[i].Vertices[j].Normal.x = 0.0f;
			ModelMeshes[i].Vertices[j].Normal.y = 0.0f;
			ModelMeshes[i].Vertices[j].Normal.z = 0.0f;
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
			vecX = ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._0].Position.x - ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._2].Position.x;
			vecY = ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._0].Position.y - ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._2].Position.y;
			vecZ = ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._0].Position.z - ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._2].Position.z;		
			edge1 = XMVectorSet(vecX, vecY, vecZ, 0.0f);	//Create our first edge

			//Get the vector describing another edge of our triangle (edge 2,1)
			vecX = ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._2].Position.x - ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._1].Position.x;
			vecY = ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._2].Position.y - ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._1].Position.y;
			vecZ = ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._2].Position.z - ModelMeshes[i].Vertices[ModelMeshes[i].Indices[j]._1].Position.z;
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

			ModelMeshes[i].Vertices[j].Normal.x = -XMVectorGetX(normalSum);
			ModelMeshes[i].Vertices[j].Normal.y = -XMVectorGetY(normalSum);
			ModelMeshes[i].Vertices[j].Normal.z = -XMVectorGetZ(normalSum);					

			VERTEX_MD5 tempVert = ModelMeshes[i].Vertices[j];			// Get the current vertex
			XMVECTOR normal = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);		// Clear normal

			int WeightStart = ModelMeshes[i].VertexWeightStart[j];
			int nWeights = ModelMeshes[i].VertexNumWeights[j];

			for ( int k = 0; k < nWeights; k++)				// Loop through each of the vertices weights
			{
				Joint_MD5 tempJoint = ModelJoints[ModelMeshes[i].Weights[WeightStart + k].JointID];	// Get the joints orientation
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
		if ( FindString_MD5(sLine, KeyWords_MD5[0]) )	// Version_MD5 4843 & 10
		{
			GetFloatFromLine(sLine, " ", TabChar);
			// if ( Version_MD5 != (int)ParseFloats[1]);	// 모델과 애니메이션의 버전이 달라도 그냥 불러와 보자..
			continue;
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[1]) )	// commandline "~"
		{
			continue;								// 주석은 건너뛰자
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[19]) )	// numFrames ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			ModelAnimation[AnimationID].numFrames = (int)ParseFloats[1];
			continue;
		}

		if ( FindString_MD5(sLine, KeyWords_MD5[2]) )	// numJoints ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			ModelAnimation[AnimationID].numJoints = (int)ParseFloats[1];
			if (ModelAnimation[AnimationID].numJoints != numJoints)	// 애니메이션의 numJoints가 불러온 모델의 numJoints와 다를 경우
				return false;										// 불러오기를 중단한다. ★★
			continue;
		}
		if ( FindString_MD5(sLine, KeyWords_MD5[20]) )	// frameRate ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			ModelAnimation[AnimationID].FrameRate = (int)ParseFloats[1];
			continue;
		}
		if ( FindString_MD5(sLine, KeyWords_MD5[21]) )	// numAnimatedComponents ~
		{
			GetFloatFromLine(sLine, " ", TabChar);
			ModelAnimation[AnimationID].numAnimatedComponents = (int)ParseFloats[1];
			continue;
		}
		if ( FindString_MD5(sLine, KeyWords_MD5[22]) )	// hierarchy {
		{
			strcpy_s(CurKey, KeyWords_MD5[22]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}
		if ( FindString_MD5(sLine, KeyWords_MD5[23]) )	// bounds {
		{
			strcpy_s(CurKey, KeyWords_MD5[23]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}
		if ( FindString_MD5(sLine, KeyWords_MD5[24]) )	// baseframe {
		{
			strcpy_s(CurKey, KeyWords_MD5[24]);		// 중괄호 열림!
			CurKeyCount = 0;
			continue;
		}
		if ( FindString_MD5(sLine, KeyWords_MD5[25]) )	// frame ~ {
		{
			FrameCount++;
			GetFloatFromLine(sLine, " ", TabChar);
			TempFrameID = (int)ParseFloats[1];

			strcpy_s(CurKey, KeyWords_MD5[25]);		// 중괄호 열림!
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

			ModelAnimation[AnimationID].BoundingBoxes[CurKeyCount-1] = TempBB;

			if (CurKeyCount >= ModelAnimation[AnimationID].numFrames)
				memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기

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

			ModelAnimation[AnimationID].JointBaseFrame[CurKeyCount-1] = TempBaseFrame;

			if (CurKeyCount >= ModelAnimation[AnimationID].numJoints)
				memset(CurKey, 0, sizeof(CurKey));	// 중괄호 닫기

			continue;
		}

		if ( FindString_MD5(CurKey, KeyWords_MD5[25]) )	// frame ~ {
		{
			// FRAME: 각 Frame이 Baseframe에서 변화한 위치!★
			Frame_MD5	TempFrameData;
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
				Joint_MD5 TempFrameJoint = ModelAnimation[AnimationID].JointBaseFrame[i];	// 기본적으로 좌표는 BaseFrame 값에서 출발!
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
					Joint_MD5	ParentJoint = ModelAnimation[AnimationID].FrameSekelton[FrameCount-1].Skeleton[TempFrameJoint.ParentID];

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
	ModelInstances[InstanceID].BeingAnimated = true;

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

	Joint_MD5 InterpolatedJoints[MAX_JOINTS];

	// 보간값(Interpolation) 계산
	for( int i = 0; i < ModelAnimation[ModelInstances[InstanceID].CurAnimID].numJoints; i++)
	{
		Joint_MD5 TempJoint;
		Joint_MD5 Joint0 = ModelAnimation[ModelInstances[InstanceID].CurAnimID].FrameSekelton[Frame0].Skeleton[i];	// Frame0의 i번째 JointData
		Joint_MD5 Joint1 = ModelAnimation[ModelInstances[InstanceID].CurAnimID].FrameSekelton[Frame1].Skeleton[i];	// Frame1의 i번째 JointData

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
		BoundingBox_MD5	TempBB;
		TempBB.Min = ModelMeshes[i].BB.Max;
		TempBB.Max = ModelMeshes[i].BB.Min;

		for (int j = 0; j < numMeshVertices[i]; j++)
		{
			VERTEX_MD5	TempVert = ModelMeshes[i].Vertices[j];
			memset(&TempVert.Position, 0, sizeof(TempVert.Position));	// Position 값을 0으로 초기화!
			memset(&TempVert.Normal, 0, sizeof(TempVert.Normal));		// Normal 값을 0으로 초기화!

			int WeightStart = ModelMeshes[i].VertexWeightStart[j];
			int nWeights = ModelMeshes[i].VertexNumWeights[j];

			float ResultX = 0.0f;
			float ResultY = 0.0f;
			float ResultZ = 0.0f;

			for (int k = 0; k < nWeights; k++)
			{
				Weight_MD5	TempWeight	= ModelMeshes[i].Weights[WeightStart+k];
				Joint_MD5	TempJoint	= InterpolatedJoints[TempWeight.JointID];

				XMVECTOR	Q1, POS, Q2;
				XMFLOAT4	Rotated;

				Q1	= XMVectorSet(TempJoint.Orientation.x, TempJoint.Orientation.y, TempJoint.Orientation.z, TempJoint.Orientation.w);
				POS	= XMVectorSet(TempWeight.Position.x, TempWeight.Position.y, TempWeight.Position.z, 0.0f);
				Q2	= XMVectorSet(-TempJoint.Orientation.x, -TempJoint.Orientation.y, -TempJoint.Orientation.z, TempJoint.Orientation.w);
				XMStoreFloat4( &Rotated, XMQuaternionMultiply( XMQuaternionMultiply( Q1, POS ), Q2 ) );

				TempVert.Position.x += ( TempJoint.Position.x + Rotated.x ) * TempWeight.Bias;
				TempVert.Position.y += ( TempJoint.Position.y + Rotated.y ) * TempWeight.Bias;
				TempVert.Position.z += ( TempJoint.Position.z + Rotated.z ) * TempWeight.Bias;

				XMVECTOR TempWeightNormal = XMVectorSet(TempWeight.Normal.x, TempWeight.Normal.y, TempWeight.Normal.z, 0.0f);
				XMStoreFloat4( &Rotated, XMQuaternionMultiply(XMQuaternionMultiply( Q1, TempWeightNormal), Q2 ) );

				TempVert.Normal.x -= Rotated.x * TempWeight.Bias;
				TempVert.Normal.y -= Rotated.y * TempWeight.Bias;
				TempVert.Normal.z -= Rotated.z * TempWeight.Bias;
			}

			ModelMeshes[i].Vertices[j].Position = TempVert.Position;
			ModelMeshes[i].Vertices[j].Normal = TempVert.Normal;		// Store the vertices normal
			XMStoreFloat3(&ModelMeshes[i].Vertices[j].Normal, XMVector3Normalize(XMLoadFloat3(&ModelMeshes[i].Vertices[j].Normal)));


			if ( TempBB.Min.x > ModelMeshes[i].Vertices[j].Position.x)
				TempBB.Min.x = ModelMeshes[i].Vertices[j].Position.x;
			if ( TempBB.Min.y > ModelMeshes[i].Vertices[j].Position.y)
				TempBB.Min.y = ModelMeshes[i].Vertices[j].Position.y;
			if ( TempBB.Min.z > ModelMeshes[i].Vertices[j].Position.z)
				TempBB.Min.z = ModelMeshes[i].Vertices[j].Position.z;

			if ( TempBB.Max.x < ModelMeshes[i].Vertices[j].Position.x)
				TempBB.Max.x = ModelMeshes[i].Vertices[j].Position.x;
			if ( TempBB.Max.y < ModelMeshes[i].Vertices[j].Position.y)
				TempBB.Max.y = ModelMeshes[i].Vertices[j].Position.y;
			if ( TempBB.Max.z < ModelMeshes[i].Vertices[j].Position.z)
				TempBB.Max.z = ModelMeshes[i].Vertices[j].Position.z;

		}

		ModelMeshes[i].BB_Animed = TempBB;
	}

	return;
}

HRESULT ModelMD5::UpdateVertices(LPDIRECT3DDEVICE9 D3DDevice, int MeshIndex)
{
	if( g_pModelVB != NULL )
		g_pModelVB->Release();

	if( g_pModelIB != NULL )
		g_pModelIB->Release();

	// 정점 버퍼 업데이트!
	VERTEX_MD5 *NewVertices = new VERTEX_MD5[numMeshVertices[MeshIndex]];

	for (int i = 0; i < numMeshVertices[MeshIndex]; i++)
	{
		NewVertices[i].Position.x = ModelMeshes[MeshIndex].Vertices[i].Position.x;
		NewVertices[i].Position.y = ModelMeshes[MeshIndex].Vertices[i].Position.y;
		NewVertices[i].Position.z = ModelMeshes[MeshIndex].Vertices[i].Position.z;
		NewVertices[i].Texture.x = ModelMeshes[MeshIndex].Vertices[i].Texture.x;
		NewVertices[i].Texture.y = ModelMeshes[MeshIndex].Vertices[i].Texture.y;
		NewVertices[i].Normal.x = ModelMeshes[MeshIndex].Vertices[i].Normal.x;
		NewVertices[i].Normal.y = ModelMeshes[MeshIndex].Vertices[i].Normal.y;
		NewVertices[i].Normal.z = ModelMeshes[MeshIndex].Vertices[i].Normal.z;
	}

	int SizeOfVertices = sizeof(VERTEX_MD5)*numMeshVertices[MeshIndex];
	if (FAILED(D3DDevice->CreateVertexBuffer(SizeOfVertices, 0, D3DFVF_VERTEX_MD5, D3DPOOL_DEFAULT, &g_pModelVB, NULL)))
		return E_FAIL;

	VOID* pVertices;
	if (FAILED(g_pModelVB->Lock(0, SizeOfVertices, (void**)&pVertices, 0)))
		return E_FAIL;
	memcpy(pVertices, NewVertices, SizeOfVertices);
	g_pModelVB->Unlock();

	delete[] NewVertices;
	NewVertices = NULL;

	// 색인 버퍼 업데이트!
	INDEX_MD5 *NewIndices = new INDEX_MD5[numMeshIndices[MeshIndex]];

	for (int i = 0; i < numMeshIndices[MeshIndex]; i++)
	{
		NewIndices[i]._0 = ModelMeshes[MeshIndex].Indices[i]._0;
		NewIndices[i]._1 = ModelMeshes[MeshIndex].Indices[i]._1;
		NewIndices[i]._2 = ModelMeshes[MeshIndex].Indices[i]._2;
	}

	int SizeOfIndices = sizeof(INDEX_MD5)*numMeshIndices[MeshIndex];
	if (FAILED(D3DDevice->CreateIndexBuffer(SizeOfIndices, 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &g_pModelIB, NULL)))
		return E_FAIL;

	VOID* pIndices;
	if (FAILED(g_pModelIB->Lock(0, SizeOfIndices, (void **)&pIndices, 0)))
		return E_FAIL;
	memcpy(pIndices, NewIndices, SizeOfIndices);
	g_pModelIB->Unlock();

	delete[] NewIndices;
	NewIndices = NULL;

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
		D3DDevice->SetStreamSource(0, g_pModelVB, 0, sizeof(VERTEX_MD5));
		D3DDevice->SetFVF(D3DFVF_VERTEX_MD5);
		D3DDevice->SetIndices(g_pModelIB);

		D3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numMeshVertices[i], 0, numMeshIndices[i]);
	}

	for (int i = 0; i < numInstances; i++)
	{
		ModelInstances[i].BeingAnimated = false;	// 애니메이션 상태 초기화
	}

	return;
}

void ModelMD5::DrawBoundingBoxes(LPDIRECT3DDEVICE9 D3DDevice, int InstanceID)
{
	for (int i = 0; i < numMeshes; i++)
	{
		XMFLOAT3	BaseBoundingBoxMax;
		XMFLOAT3	BaseBoundingBoxMin;

		if (ModelInstances[InstanceID].BeingAnimated == false)
		{
			BaseBoundingBoxMax = ModelMeshes[i].BB.Max;
			BaseBoundingBoxMin = ModelMeshes[i].BB.Min;
		}
		else
		{
			BaseBoundingBoxMax = ModelMeshes[i].BB_Animed.Max;
			BaseBoundingBoxMin = ModelMeshes[i].BB_Animed.Min;
		}

		if( g_pModelVB != NULL )
			g_pModelVB->Release();

		if( g_pModelIB != NULL )
			g_pModelIB->Release();

		int numVertices = 8;	// 상자니까 점 8개 필요
		VERTEX_MD5_BB *NewVertices = new VERTEX_MD5_BB[numVertices];

			float XLen = BaseBoundingBoxMax.x - BaseBoundingBoxMin.x;
			float YLen = BaseBoundingBoxMax.y - BaseBoundingBoxMin.y;
			float ZLen = BaseBoundingBoxMax.z - BaseBoundingBoxMin.z;

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
			if (FAILED(D3DDevice->CreateVertexBuffer(SizeOfVertices, 0, D3DFVF_VERTEX_MD5_BB, D3DPOOL_DEFAULT, &g_pModelVB, NULL)))
				return;
	
			VOID* pVertices;
			if (FAILED(g_pModelVB->Lock(0, SizeOfVertices, (void**)&pVertices, 0)))
				return;
			memcpy(pVertices, NewVertices, SizeOfVertices);
			g_pModelVB->Unlock();

		delete[] NewVertices;
		NewVertices = NULL;
		

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
			if (FAILED(D3DDevice->CreateIndexBuffer(SizeOfIndices, 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &g_pModelIB, NULL)))
				return;

			VOID* pIndices;
			if (FAILED(g_pModelIB->Lock(0, SizeOfIndices, (void **)&pIndices, 0)))
				return;
			memcpy(pIndices, NewIndices, SizeOfIndices);
			g_pModelIB->Unlock();
	
		delete[] NewIndices;
		NewIndices = NULL;


		D3DDevice->SetStreamSource(0, g_pModelVB, 0, sizeof(VERTEX_MD5_BB));
		D3DDevice->SetFVF(D3DFVF_VERTEX_MD5_BB);
		D3DDevice->SetIndices(g_pModelIB);

		D3DDevice->DrawIndexedPrimitive(D3DPT_LINELIST, 0, 0, numVertices, 0, numIndices);
	}

	return;
}

HRESULT ModelMD5::DrawNormalVecters(LPDIRECT3DDEVICE9 D3DDevice, float LenFactor)
{
	for (int i = 0; i < numMeshes; i++)
	{

		if( g_pModelVB != NULL )
			g_pModelVB->Release();

		if( g_pModelIB != NULL )
			g_pModelIB->Release();

		int numVertices = numMeshVertices[i] * 2;
		int numIndices = numVertices / 2;

		// 정점 버퍼 업데이트!
		VERTEX_MD5_NORMAL *NewVertices = new VERTEX_MD5_NORMAL[numVertices];

			for (int j = 0; j < numVertices; j+=2)
			{
				NewVertices[j].Normal.x = ModelMeshes[i].Vertices[j/2].Position.x;
				NewVertices[j].Normal.y = ModelMeshes[i].Vertices[j/2].Position.y;
				NewVertices[j].Normal.z = ModelMeshes[i].Vertices[j/2].Position.z;
				NewVertices[j+1].Normal.x = NewVertices[j].Normal.x + ModelMeshes[i].Vertices[j/2].Normal.x * LenFactor;
				NewVertices[j+1].Normal.y = NewVertices[j].Normal.y + ModelMeshes[i].Vertices[j/2].Normal.y * LenFactor;
				NewVertices[j+1].Normal.z = NewVertices[j].Normal.z + ModelMeshes[i].Vertices[j/2].Normal.z * LenFactor;
			}

			int SizeOfVertices = sizeof(VERTEX_MD5_NORMAL)*numVertices;
			if (FAILED(D3DDevice->CreateVertexBuffer(SizeOfVertices, 0, D3DFVF_VERTEX_MD5_NORMAL, D3DPOOL_DEFAULT, &g_pModelVB, NULL)))
				return E_FAIL;
	
			VOID* pVertices;
			if (FAILED(g_pModelVB->Lock(0, SizeOfVertices, (void**)&pVertices, 0)))
				return E_FAIL;
			memcpy(pVertices, NewVertices, SizeOfVertices);
			g_pModelVB->Unlock();

		delete[] NewVertices;


		// 색인 버퍼 업데이트!
		INDEX_MD5_NORMAL *NewIndices = new INDEX_MD5_NORMAL[numIndices];
		int k = 0;

			for (int i = 0; i < numIndices; i++)
			{
				NewIndices[i]._0 = k++;
				NewIndices[i]._1 = k++;
			}

			int SizeOfIndices = sizeof(INDEX_MD5_NORMAL)*numIndices;
			if (FAILED(D3DDevice->CreateIndexBuffer(SizeOfIndices, 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &g_pModelIB, NULL)))
				return E_FAIL;

			VOID* pIndices;
			if (FAILED(g_pModelIB->Lock(0, SizeOfIndices, (void **)&pIndices, 0)))
				return E_FAIL;
			memcpy(pIndices, NewIndices, SizeOfIndices);
			g_pModelIB->Unlock();
	
		delete[] NewIndices;

		D3DDevice->SetStreamSource(0, g_pModelVB, 0, sizeof(VERTEX_MD5_NORMAL));
		D3DDevice->SetFVF(D3DFVF_VERTEX_MD5_NORMAL);
		D3DDevice->SetIndices(g_pModelIB);

		D3DDevice->DrawIndexedPrimitive(D3DPT_LINELIST, 0, 0, numVertices, 0, numIndices);

	}

	return S_OK;	// 함수 종료!
}

PickingRay ModelMD5::GetPickingRay(LPDIRECT3DDEVICE9 D3DDevice, int MouseX, int MouseY,
	int ScreenWidth, int ScreenHeight, D3DXMATRIX matView, D3DXMATRIX matProj)
{
	if (MouseX < 0 || MouseY < 0 || MouseX > ScreenWidth || MouseY > ScreenHeight)
		return PickingRay(D3DXVECTOR3(0,0,0), D3DXVECTOR3(9999.0f,0,0));

	D3DVIEWPORT9 vp;
	D3DXMATRIX InvView;

	D3DXVECTOR3 MouseViewPortXY, PickingRayDir, PickingRayPos;

	D3DDevice->GetViewport(&vp);
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

bool ModelMD5::CheckMouseOverPerInstance(LPDIRECT3DDEVICE9 D3DDevice, int InstanceID, int MouseX, int MouseY,
	int ScreenWidth, int ScreenHeight, D3DXMATRIX matView, D3DXMATRIX matProj)
{
	PickingRay PR = GetPickingRay(D3DDevice, MouseX, MouseY, ScreenWidth, ScreenHeight, matView, matProj);

	if (PR.Dir.x == 9999.0f)
		return false;

	DistanceCmp[InstanceID]				= FLT_MAX;
	MouseOverPerInstances[InstanceID]	= false;
	PickedPosition[InstanceID]			= XMFLOAT3(0, 0, 0);

		for (int j = 0; j < numMeshes; j++)
		{
			int numIndices = numMeshIndices[j];

			for (int i = 0; i < numIndices; i++)
			{
				int ID0 = ModelMeshes[j].Indices[i]._0;
				int ID1 = ModelMeshes[j].Indices[i]._1;
				int ID2 = ModelMeshes[j].Indices[i]._2;

				D3DXVECTOR3 p0, p1, p2;
				p0 = D3DXVECTOR3(ModelMeshes[j].Vertices[ID0].Position.x, ModelMeshes[j].Vertices[ID0].Position.y, ModelMeshes[j].Vertices[ID0].Position.z);
				p1 = D3DXVECTOR3(ModelMeshes[j].Vertices[ID1].Position.x, ModelMeshes[j].Vertices[ID1].Position.y, ModelMeshes[j].Vertices[ID1].Position.z);
				p2 = D3DXVECTOR3(ModelMeshes[j].Vertices[ID2].Position.x, ModelMeshes[j].Vertices[ID2].Position.y, ModelMeshes[j].Vertices[ID2].Position.z);

				// 인스턴스
				D3DXMATRIX matInstTrans;
				D3DXMATRIX matInstRotX;
				D3DXMATRIX matInstRotY;
				D3DXMATRIX matInstRotZ;
				D3DXMATRIX matInstScal;
				D3DXMATRIX matInstWorld;

				D3DXMatrixTranslation(&matInstTrans,
					ModelInstances[InstanceID].Translation.x, ModelInstances[InstanceID].Translation.y, ModelInstances[InstanceID].Translation.z);

				D3DXMatrixRotationX(&matInstRotX, ModelInstances[InstanceID].Rotation.x);
				D3DXMatrixRotationY(&matInstRotY, ModelInstances[InstanceID].Rotation.y);
				D3DXMatrixRotationZ(&matInstRotZ, ModelInstances[InstanceID].Rotation.z);

				D3DXMatrixScaling(&matInstScal,
					ModelInstances[InstanceID].Scaling.x, ModelInstances[InstanceID].Scaling.y, ModelInstances[InstanceID].Scaling.z);
				
				matInstWorld = matInstRotX * matInstRotY * matInstRotZ * matInstScal * matInstTrans;

				D3DXVec3TransformCoord(&p0, &p0, &matInstWorld);
				D3DXVec3TransformCoord(&p1, &p1, &matInstWorld);
				D3DXVec3TransformCoord(&p2, &p2, &matInstWorld);

				float pU, pV, pDist;

				if (D3DXIntersectTri(&p0, &p1, &p2, &PR.Pos, &PR.Dir, &pU, &pV, &pDist))
				{
					if (pDist < DistanceCmp[InstanceID])
					{
						DistanceCmp[InstanceID] = pDist;
						D3DXVECTOR3 TempPosition = p0 + (p1*pU - p0*pU) + (p2*pV - p0*pV);
						D3DXVec3TransformCoord(&TempPosition, &TempPosition, &matInstWorld);

						PickedPosition[InstanceID].x = TempPosition.x;
						PickedPosition[InstanceID].y = TempPosition.y;
						PickedPosition[InstanceID].z = TempPosition.z;
					}

					MouseOverPerInstances[InstanceID] = true;
				}
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