#include "globals.hlsli"
#include "cullingShaderHF.hlsli"

CBUFFER(EmittedParticleCB, CBSLOT_OTHER_EMITTEDPARTICLE)
{
	float2		xAdd;
	float		xMotionBlurAmount;
	float		padding;
};

struct GS_INPUT{
	float4	pos			: SV_POSITION;
	float4  inSizOpMir	: TEXCOORD0;
	//float3	color		: COLOR;
	float	rot			: ROTATION;
	float3 vel			: VELOCITY;
};

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
	nointerpolation float4 opaAddDarkSiz	: TEXCOORD1;
	//float3 col				: TEXCOORD2;
	float4 pp				: TEXCOORD2;
};

[maxvertexcount(4)]
void main(point GS_INPUT p[1], inout TriangleStream<VertextoPixel> triStream, uint vid : SV_PrimitiveID)
{
	float quadLength = p[0].inSizOpMir.x*0.5f;

	//const Plane planes[6] = {
	//	{ g_xFrame_FrustumPlanesWS[0].xyz, -g_xFrame_FrustumPlanesWS[0].w }, // left plane
	//	{ g_xFrame_FrustumPlanesWS[1].xyz, -g_xFrame_FrustumPlanesWS[1].w }, // right plane
	//	{ g_xFrame_FrustumPlanesWS[2].xyz, -g_xFrame_FrustumPlanesWS[2].w }, // top plane
	//	{ g_xFrame_FrustumPlanesWS[3].xyz, -g_xFrame_FrustumPlanesWS[3].w }, // bottom plane
	//	{ g_xFrame_FrustumPlanesWS[4].xyz, -g_xFrame_FrustumPlanesWS[4].w }, // near plane
	//	{ g_xFrame_FrustumPlanesWS[5].xyz, -g_xFrame_FrustumPlanesWS[5].w }, // far plane
	//};
	//Sphere sphere = { p[0].pos.xyz, quadLength };
	//if (!SphereInsideFrustum(sphere, planes))
	//{
	//	return;
	//}

	VertextoPixel p1 = (VertextoPixel)0;
	float2x2 rot = float2x2(
		cos(p[0].rot),-sin(p[0].rot),
		sin(p[0].rot),cos(p[0].rot)
		);

	p[0].vel = normalize(p[0].vel);
    float3 normal			= p[0].pos.xyz - g_xCamera_CamPos.xyz;
	normal					= mul(normal, (float3x3)g_xCamera_View);

	float3 r = float3( mul(float2(0,1),rot),0 );
    float3 rightAxis		= cross(r, normal);
    float3 upAxis			= cross(normal, rightAxis);
    rightAxis				= normalize(rightAxis);
    upAxis					= normalize(upAxis);

    float4 rightVector		= float4(rightAxis.xyz, 1.0f);
    float4 upVector         = float4(upAxis.xyz, 1.0f);
	p[0].pos				= mul(p[0].pos, g_xCamera_View);
	p[0].vel				= mul(p[0].vel, (float3x3)g_xCamera_View);



	//p1.col = p[0].color;
	p1.opaAddDarkSiz = float4(saturate(p[0].inSizOpMir.y),xAdd.x,xAdd.y,p[0].inSizOpMir.x);

		//rightVector.xy=mul(rightVector.xy,rot);
		//upVector.xy=mul(upVector.xy,rot);

	p1.pos.w=1;
    p1.pos.xyz = p[0].pos.xyz+rightVector.xyz*(-quadLength)+upVector.xyz*(quadLength);
	// extrude along velocity (blur)
	p1.pos.xyz += p[0].vel * dot(normalize(p1.pos.xyz - p[0].pos.xyz), p[0].vel) * xMotionBlurAmount;
    p1.tex = float2(0.0f, 0.0f);
    p1.pos = mul(p1.pos, g_xCamera_Proj);
	if(p[0].inSizOpMir.z==1) p1.tex.x=1-p1.tex.x;
	if(p[0].inSizOpMir.w==1) p1.tex.y=1-p1.tex.y;
	p1.pp = p1.pos;
    triStream.Append(p1);
	
	p1.pos.w=1;
    p1.pos.xyz = p[0].pos.xyz+rightVector.xyz*(-quadLength)+upVector.xyz*(-quadLength);
	// extrude along velocity (blur)
	p1.pos.xyz += p[0].vel * dot(normalize(p1.pos.xyz - p[0].pos.xyz), p[0].vel) * xMotionBlurAmount;
    p1.tex = float2(0.0f, 1.0f);
    p1.pos = mul(p1.pos, g_xCamera_Proj);
	if(p[0].inSizOpMir.z==1) p1.tex.x=1-p1.tex.x;
	if(p[0].inSizOpMir.w==1) p1.tex.y=1-p1.tex.y;
	p1.pp = p1.pos;
    triStream.Append(p1);
	
	p1.pos.w=1;
    p1.pos.xyz = p[0].pos.xyz +rightVector.xyz*(quadLength)+upVector.xyz*(quadLength);
	// extrude along velocity (blur)
	p1.pos.xyz += p[0].vel * dot(normalize(p1.pos.xyz - p[0].pos.xyz), p[0].vel) * xMotionBlurAmount;
    p1.tex = float2(1.0f, 0.0f);
    p1.pos = mul(p1.pos, g_xCamera_Proj);
	if(p[0].inSizOpMir.z==1) p1.tex.x=1-p1.tex.x;
	if(p[0].inSizOpMir.w==1) p1.tex.y=1-p1.tex.y;
	p1.pp = p1.pos;
    triStream.Append(p1);
	
	p1.pos.w=1;
    p1.pos.xyz = p[0].pos.xyz +rightVector.xyz*(quadLength)+upVector.xyz*(-quadLength);
	// extrude along velocity (blur)
	p1.pos.xyz += p[0].vel * dot(normalize(p1.pos.xyz - p[0].pos.xyz), p[0].vel) * xMotionBlurAmount;
    p1.tex = float2(1.0f, 1.0f);
    p1.pos = mul(p1.pos, g_xCamera_Proj);
	if(p[0].inSizOpMir.z==1) p1.tex.x=1-p1.tex.x;
	if(p[0].inSizOpMir.w==1) p1.tex.y=1-p1.tex.y;
	p1.pp = p1.pos;
    triStream.Append(p1);
}
