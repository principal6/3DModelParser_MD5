#define AnimationJointTexW	800
#define AnimationWeightTexW	800
#define AnimationWeightTexH	100
#define MAX_ANIMATIONS		20			// 모델 - 애니메이션 최대 개수
#define MAX_FRAMES			100			// 모델 - 애니메이션 최대 프레임

// --- 전역 변수 --- //
float4x4	matVP;

texture		DiffuseMap_Tex;
texture		AnimationJoint_Tex;
texture		AnimationWeight_Tex;

sampler2D	DiffuseSampler =
	sampler_state
	{
		Texture = (DiffuseMap_Tex);
	};

sampler2D	AnimationJointSampler =
	sampler_state
	{
		Texture = (AnimationJoint_Tex);
	};

sampler2D	AnimationWeightSampler =
	sampler_state
	{
		Texture = (AnimationWeight_Tex);
	};

struct VS_INPUT_MODEL
{
	// 정점 정보
	float4	Position	: POSITION0;
	float3	Normal		: NORMAL0;
	float2	Texture		: TEXCOORD0;
	float	WeightStart	: PSIZE0;
	float	WeightNum	: PSIZE1;

	// 인스턴스 정보
	float4	matWorld0		: POSITION1;
	float4	matWorld1		: POSITION2;
	float4	matWorld2		: POSITION3;
	float4	matWorld3		: POSITION4;
	float	bAnimated		: PSIZE2;
	float	AnimationID		: PSIZE3;
	float	CurFrameTime	: PSIZE4;
	float	Frame0			: PSIZE5;
	float	Frame1			: PSIZE6;
};

struct VS_OUTPUT_MODEL
{
	float4 Position : POSITION0;
	float2 Texture	: TEXCOORD0;
	float3 Normal	: TEXCOORD1;
};

float GetMod(float Dividend, float Divisor)	// fmod()가 오차값을 발생시키기 때문에 만든 함수★★
{
	float times = floor(Dividend / Divisor);
	float multiplied = Divisor * times;
	return (Dividend - multiplied);
}

float4 GetJointPosition(in int AnimID, in int FrameID, in int JointID)
{
	float4 Output;
	Output.x = tex2Dlod(AnimationJointSampler, float4(((0.5 + JointID*8)/AnimationJointTexW), ((0.5 + FrameID + AnimID*MAX_FRAMES)/(MAX_FRAMES * MAX_ANIMATIONS)), 0, 0));
	Output.y = tex2Dlod(AnimationJointSampler, float4(((0.5 + JointID*8 + 1)/AnimationJointTexW), ((0.5 + FrameID + AnimID*MAX_FRAMES)/(MAX_FRAMES * MAX_ANIMATIONS)), 0, 0));
	Output.z = tex2Dlod(AnimationJointSampler, float4(((0.5 + JointID*8 + 2)/AnimationJointTexW), ((0.5 + FrameID + AnimID*MAX_FRAMES)/(MAX_FRAMES * MAX_ANIMATIONS)), 0, 0));
	Output.w = tex2Dlod(AnimationJointSampler, float4(((0.5 + JointID*8 + 3)/AnimationJointTexW), ((0.5 + FrameID + AnimID*MAX_FRAMES)/(MAX_FRAMES * MAX_ANIMATIONS)), 0, 0));
	return Output;
}

float4 GetJointOrientation(in int AnimID, in int FrameID, in int JointID)
{
	float4 Output;
	Output.x = tex2Dlod(AnimationJointSampler, float4(((0.5 + JointID*8 + 4)/AnimationJointTexW), ((0.5 + FrameID + AnimID*MAX_FRAMES)/(MAX_FRAMES * MAX_ANIMATIONS)), 0, 0));
	Output.y = tex2Dlod(AnimationJointSampler, float4(((0.5 + JointID*8 + 5)/AnimationJointTexW), ((0.5 + FrameID + AnimID*MAX_FRAMES)/(MAX_FRAMES * MAX_ANIMATIONS)), 0, 0));
	Output.z = tex2Dlod(AnimationJointSampler, float4(((0.5 + JointID*8 + 6)/AnimationJointTexW), ((0.5 + FrameID + AnimID*MAX_FRAMES)/(MAX_FRAMES * MAX_ANIMATIONS)), 0, 0));
	Output.w = tex2Dlod(AnimationJointSampler, float4(((0.5 + JointID*8 + 7)/AnimationJointTexW), ((0.5 + FrameID + AnimID*MAX_FRAMES)/(MAX_FRAMES * MAX_ANIMATIONS)), 0, 0));
	return Output;
}

int GetWeightJointID(in int WeightID)
{
	float Dividend = WeightID * 8;
	int WeightU = GetMod(Dividend, AnimationWeightTexW);
	int WeightV = floor(Dividend / AnimationWeightTexW);
	int Output = tex2Dlod(AnimationWeightSampler, float4(((0.5 + WeightU)/AnimationWeightTexW), ((0.5 + WeightV)/AnimationWeightTexH), 0, 0));
	return Output;
}

float GetWeightBias(in int WeightID)
{
	float Dividend = WeightID * 8;
	int WeightU = GetMod(Dividend, AnimationWeightTexW);
	int WeightV = floor(Dividend / AnimationWeightTexW);
	float Output = tex2Dlod(AnimationWeightSampler, float4(((0.5 + WeightU+1)/AnimationWeightTexW), ((0.5 + WeightV)/AnimationWeightTexH), 0, 0));
	return Output;
}

float4 GetWeightPosition(in int WeightID)
{
	float Dividend = WeightID * 8;
	int WeightU = GetMod(Dividend, AnimationWeightTexW);
	int WeightV = floor(Dividend / AnimationWeightTexW);
	float4 Output;
	Output.x = tex2Dlod(AnimationWeightSampler, float4(((0.5 + WeightU+2)/AnimationWeightTexW), ((0.5 + WeightV)/AnimationWeightTexH), 0, 0));
	Output.y = tex2Dlod(AnimationWeightSampler, float4(((0.5 + WeightU+3)/AnimationWeightTexW), ((0.5 + WeightV)/AnimationWeightTexH), 0, 0));
	Output.z = tex2Dlod(AnimationWeightSampler, float4(((0.5 + WeightU+4)/AnimationWeightTexW), ((0.5 + WeightV)/AnimationWeightTexH), 0, 0));
	Output.w = 0;
	return Output;
}

float4 GetWeightNormal(in int WeightID)
{
	float Dividend = WeightID * 8;
	int WeightU = GetMod(Dividend, AnimationWeightTexW);
	int WeightV = floor(Dividend / AnimationWeightTexW);
	float4 Output;
	Output.x = tex2Dlod(AnimationWeightSampler, float4(((0.5 + WeightU+5)/AnimationWeightTexW), ((0.5 + WeightV)/AnimationWeightTexH), 0, 0));
	Output.y = tex2Dlod(AnimationWeightSampler, float4(((0.5 + WeightU+6)/AnimationWeightTexW), ((0.5 + WeightV)/AnimationWeightTexH), 0, 0));
	Output.z = tex2Dlod(AnimationWeightSampler, float4(((0.5 + WeightU+7)/AnimationWeightTexW), ((0.5 + WeightV)/AnimationWeightTexH), 0, 0));
	Output.w = 0;
	return Output;
}

float4 QuaternionMul( float4 Q1, float4 Q2 )
{
	float4 Output;
	Output.x = (Q2.w * Q1.x) + (Q2.x * Q1.w) + (Q2.y * Q1.z) - (Q2.z * Q1.y);
	Output.y = (Q2.w * Q1.y) - (Q2.x * Q1.z) + (Q2.y * Q1.w) + (Q2.z * Q1.x);
	Output.z = (Q2.w * Q1.z) + (Q2.x * Q1.y) - (Q2.y * Q1.x) + (Q2.z * Q1.w);
	Output.w = (Q2.w * Q1.w) - (Q2.x * Q1.x) - (Q2.y * Q1.y) - (Q2.z * Q1.z);
	return Output;
}

float4 QuaternionRotateF( float4 Q1, float4 Pos )
{
	float4 Output;
	float4 Q2 = float4(-Q1.x, -Q1.y, -Q1.z, Q1.w);
	Output = QuaternionMul( QuaternionMul(Q1, Pos), Q2 );
	return Output;
}

VS_OUTPUT_MODEL vs_main( VS_INPUT_MODEL Input )
{
	VS_OUTPUT_MODEL Output;

	// 인스턴스별 월드 행렬
	float4x4 matWorld = float4x4(Input.matWorld0.x, Input.matWorld0.y, Input.matWorld0.z, Input.matWorld0.w,
		Input.matWorld1.x, Input.matWorld1.y, Input.matWorld1.z, Input.matWorld1.w,
		Input.matWorld2.x, Input.matWorld2.y, Input.matWorld2.z, Input.matWorld2.w,
		Input.matWorld3.x, Input.matWorld3.y, Input.matWorld3.z, Input.matWorld3.w);

	// 애니메이션 적용
	float4 TempPosition	= float4(0, 0, 0, 0);
	float4 TempNormal	= float4(0, 0, 0, 0);

	int WeightStart	= Input.WeightStart;
	int WeightNum	= Input.WeightNum;
	int AnimID		= Input.AnimationID;

	float CurFrameTime	= Input.CurFrameTime;
	int Frame0	= Input.Frame0;
	int Frame1	= Input.Frame1;

	for (int i = 0; i < WeightNum; i++)
	{
		int WeightID = WeightStart + i;
		int JointID = GetWeightJointID(WeightID);
		float Bias = GetWeightBias(WeightID);

		float4 WeightPos = GetWeightPosition(WeightID);
		float4 WeightNorm = GetWeightNormal(WeightID);

		float4 Joint0Pos = GetJointPosition(AnimID, Frame0, JointID);
		float4 Joint0Ori = GetJointOrientation(AnimID, Frame0, JointID);

		float4 RotatedPos = QuaternionRotateF(Joint0Ori, WeightPos);
		TempPosition += ( Joint0Pos + RotatedPos ) * Bias;

		float4 RotatedNorm = QuaternionRotateF(Joint0Ori, WeightNorm);
		TempNormal -= RotatedNorm * Bias;
	}

	Output.Position = mul( TempPosition, matWorld );
	Output.Position = mul( Output.Position, matVP );
	Output.Normal = normalize( TempNormal.xyz );
	Output.Texture = Input.Texture;

	return( Output );
}

VS_OUTPUT_MODEL vs_still( VS_INPUT_MODEL Input )
{
	VS_OUTPUT_MODEL Output;

	float4x4 matWorld = float4x4(Input.matWorld0.x, Input.matWorld0.y, Input.matWorld0.z, Input.matWorld0.w,
		Input.matWorld1.x, Input.matWorld1.y, Input.matWorld1.z, Input.matWorld1.w,
		Input.matWorld2.x, Input.matWorld2.y, Input.matWorld2.z, Input.matWorld2.w,
		Input.matWorld3.x, Input.matWorld3.y, Input.matWorld3.z, Input.matWorld3.w);

	Output.Position = mul( Input.Position, matWorld );
	Output.Position = mul( Output.Position, matVP );
	Output.Normal = normalize( Input.Normal );
	Output.Texture = Input.Texture;

	return( Output );
}

float4 ps_main(VS_OUTPUT_MODEL Input) : COLOR
{
	float4 albedo = tex2D(DiffuseSampler, Input.Texture);
	return albedo.rgba;
}

technique HLSLMain
{
	pass Pass_0
	{
		VertexShader	= compile vs_3_0 vs_main();
		PixelShader		= compile ps_3_0 ps_main();
	}
}

technique HLSLStill
{
	pass Pass_0
	{
		VertexShader	= compile vs_3_0 vs_still();
		PixelShader		= compile ps_3_0 ps_main();
	}
}
