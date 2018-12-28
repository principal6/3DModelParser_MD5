// --- 전역 변수 --- //
float4x4	matWVP;

texture		DiffuseMap_Tex;
sampler2D	DiffuseSampler =
	sampler_state
	{
	   Texture = (DiffuseMap_Tex);
	};

struct VS_INPUT
{
	float4 Position : POSITION;
	float3 Normal	: NORMAL;
	float2 Texture	: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Position : POSITION;
	float2 Texture	: TEXCOORD0;
	float3 Normal	: TEXCOORD1;
};

VS_OUTPUT vs_main( VS_INPUT Input )
{
	VS_OUTPUT Output;

	Output.Position = mul( Input.Position, matWVP );
	Output.Normal = normalize( Input.Normal );
	Output.Texture = Input.Texture;

	return( Output );
}

float4 ps_main(VS_OUTPUT Input) : COLOR
{
	float4 albedo = tex2D(DiffuseSampler, Input.Texture);
	
	return albedo.rgba;
}

technique HLSLTest
{
	pass Pass_0
	{
		VertexShader	= compile vs_2_0 vs_main();
		PixelShader		= compile ps_2_0 ps_main();
	}
}