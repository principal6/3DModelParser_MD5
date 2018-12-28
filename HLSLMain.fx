// --- 전역 변수 --- //
float4x4	matVP;

texture		DiffuseMap_Tex;
sampler2D	DiffuseSampler =
	sampler_state
	{
	   Texture = (DiffuseMap_Tex);
	};

struct VS_INPUT_MODEL
{
	float4	Position	: POSITION0;
	float3	Normal		: NORMAL0;
	float2	Texture		: TEXCOORD0;
	float4	matWorld0	: POSITION1;
	float4	matWorld1	: POSITION2;
	float4	matWorld2	: POSITION3;
	float4	matWorld3	: POSITION4;
	float	bAnimated	: PSIZE0;
};

struct VS_OUTPUT
{
	float4 Position : POSITION0;
	float2 Texture	: TEXCOORD0;
	float3 Normal	: TEXCOORD1;
};

VS_OUTPUT vs_main( VS_INPUT_MODEL Input )
{
	VS_OUTPUT Output;

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

float4 ps_main(VS_OUTPUT Input) : COLOR
{
	float4 albedo = tex2D(DiffuseSampler, Input.Texture);
	
	return albedo.rgba;
}

technique HLSLMain
{
	pass Pass_0
	{
		VertexShader	= compile vs_2_0 vs_main();
		PixelShader		= compile ps_2_0 ps_main();
	}
}