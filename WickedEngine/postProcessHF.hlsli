#ifndef _POSTPROCESS_HF_
#define _POSTPROCESS_HF_

#include "imageHF.hlsli"
#include "packHF.hlsli"
#include "depthConvertHF.hlsli"


float2 GetVelocity(in int2 pixel)
{
#ifdef DILATE_VELOCITY_BEST_3X3 // search best velocity in 3x3 neighborhood

	float bestDepth = g_xFrame_MainCamera_ZFarP;
	int2 bestPixel = int2(0, 0);

	[unroll]
	for (int i = -1; i <= 1; ++i)
	{
		[unroll]
		for (int j = -1; j <= 1; ++j)
		{
			int2 curPixel = pixel + int2(i, j);
			float depth = texture_lineardepth[curPixel];
			[flatten]
			if (depth < bestDepth)
			{
				bestDepth = depth;
				bestPixel = curPixel;
			}
		}
	}

	return texture_gbuffer1[bestPixel].zw;

#elif defined DILATE_VELOCITY_BEST_FAR // search best velocity in a far reaching 5-tap pattern

	float bestDepth = g_xFrame_MainCamera_ZFarP;
	int2 bestPixel = int2(0, 0);

	// top-left
	int2 curPixel = pixel + int2(-2, -2);
	float depth = texture_lineardepth[curPixel];
	[flatten]
	if (depth < bestDepth)
	{
		bestDepth = depth;
		bestPixel = curPixel;
	}

	// top-right
	curPixel = pixel + int2(2, -2);
	depth = texture_lineardepth[curPixel];
	[flatten]
	if (depth < bestDepth)
	{
		bestDepth = depth;
		bestPixel = curPixel;
	}

	// bottom-right
	curPixel = pixel + int2(2, 2);
	depth = texture_lineardepth[curPixel];
	[flatten]
	if (depth < bestDepth)
	{
		bestDepth = depth;
		bestPixel = curPixel;
	}

	// bottom-left
	curPixel = pixel + int2(-2, 2);
	depth = texture_lineardepth[curPixel];
	[flatten]
	if (depth < bestDepth)
	{
		bestDepth = depth;
		bestPixel = curPixel;
	}

	// center
	curPixel = pixel;
	depth = texture_lineardepth[curPixel];
	[flatten]
	if (depth < bestDepth)
	{
		bestDepth = depth;
		bestPixel = curPixel;
	}

	return texture_gbuffer1[bestPixel].zw;

#elif defined DILATE_VELOCITY_AVG_FAR

	float2 velocity_TL = texture_gbuffer1[pixel + int2(-2, -2)].zw;
	float2 velocity_TR = texture_gbuffer1[pixel + int2(2, -2)].zw;
	float2 velocity_BL = texture_gbuffer1[pixel + int2(-2, 2)].zw;
	float2 velocity_BR = texture_gbuffer1[pixel + int2(2, 2)].zw;
	float2 velocity_CE = texture_gbuffer1[pixel].zw;

	return (velocity_TL + velocity_TR + velocity_BL + velocity_BR + velocity_CE) / 5.0f;

#else

	return texture_gbuffer1[pixel].zw;

#endif // DILATE_VELOCITY
}

float loadDepth(float2 texCoord)
{
	float2 dim;
	texture_lineardepth.GetDimensions(dim.x, dim.y);
	return texture_lineardepth.Load(int3(dim*texCoord, 0)).r;
}
float4 loadMask(float2 texCoord)
{
	float2 dim;
	texture_1.GetDimensions(dim.x, dim.y);
	return texture_1.Load(int3(dim*texCoord, 0));
}
float4 loadScene(float2 texCoord)
{
	float2 dim;
	texture_0.GetDimensions(dim.x, dim.y);
	return texture_0.Load(int3(dim*texCoord, 0));
}

#endif // _POSTPROCESS_HF_
