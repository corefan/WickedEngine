#include "objectHF.hlsli"



PixelInputType_Simple main(Input_Object_POS_TEX input)
{
	PixelInputType_Simple Out = (PixelInputType_Simple)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);

	Out.instanceColor = input.instance.color_dither.rgb;
	Out.dither = input.instance.color_dither.a;

	float4 pos = float4(input.pos.xyz, 1);

	pos = mul(pos, WORLD);

	Out.clip = dot(pos, g_xClipPlane);

	affectWind(pos.xyz, input.pos.w, g_xFrame_Time);


	Out.pos = mul(pos, g_xCamera_VP);
	Out.tex = input.tex.xy;


	return Out;
}