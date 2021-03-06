#include "ForwardRenderableComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiHelper.h"
#include "wiProfiler.h"

using namespace wiGraphicsTypes;

ForwardRenderableComponent::ForwardRenderableComponent()
{
	Renderable3DComponent::setProperties();

	setSSREnabled(false);
	setSSAOEnabled(false);
	setShadowsEnabled(false);

	setPreferredThreadingCount(0);
}
ForwardRenderableComponent::~ForwardRenderableComponent()
{
}

wiRenderTarget ForwardRenderableComponent::rtMain;
void ForwardRenderableComponent::ResizeBuffers()
{
	Renderable3DComponent::ResizeBuffers();

	FORMAT defaultTextureFormat = GraphicsDevice::GetBackBufferFormat();

	// Protect against multiple buffer resizes when there is no change!
	static UINT lastBufferResWidth = 0, lastBufferResHeight = 0, lastBufferMSAA = 0;
	static FORMAT lastBufferFormat = FORMAT_UNKNOWN;
	if (lastBufferResWidth == wiRenderer::GetInternalResolution().x &&
		lastBufferResHeight == wiRenderer::GetInternalResolution().y &&
		lastBufferMSAA == getMSAASampleCount() &&
		lastBufferFormat == defaultTextureFormat)
	{
		return;
	}
	else
	{
		lastBufferResWidth = wiRenderer::GetInternalResolution().x;
		lastBufferResHeight = wiRenderer::GetInternalResolution().y;
		lastBufferMSAA = getMSAASampleCount();
		lastBufferFormat = defaultTextureFormat;
	}

	rtMain.Initialize(wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y, true, FORMAT_R16G16B16A16_FLOAT, 1, getMSAASampleCount(), false);
	rtMain.Add(FORMAT_R16G16B16A16_FLOAT); // thin gbuffer
}

void ForwardRenderableComponent::Initialize()
{
	ResizeBuffers();

	Renderable3DComponent::Initialize();
}
void ForwardRenderableComponent::Load()
{
	Renderable3DComponent::Load();

}
void ForwardRenderableComponent::Start()
{
	Renderable3DComponent::Start();
}
void ForwardRenderableComponent::Render()
{
	if (getThreadingCount() > 1)
	{

		for (auto workerThread : workerThreads)
		{
			workerThread->wakeup();
		}

		for (auto workerThread : workerThreads)
		{
			workerThread->wait();
		}

		wiRenderer::GetDevice()->ExecuteDeferredContexts();
	}
	else
	{
		RenderFrameSetUp(GRAPHICSTHREAD_IMMEDIATE);
		RenderShadows(GRAPHICSTHREAD_IMMEDIATE);
		RenderReflections(GRAPHICSTHREAD_IMMEDIATE);
		RenderScene(GRAPHICSTHREAD_IMMEDIATE);
		RenderSecondaryScene(rtMain, rtMain, GRAPHICSTHREAD_IMMEDIATE);
		RenderComposition(rtMain, rtMain, GRAPHICSTHREAD_IMMEDIATE);
	}

	Renderable2DComponent::Render();
}


void ForwardRenderableComponent::RenderScene(GRAPHICSTHREAD threadID)
{
	wiProfiler::GetInstance().BeginRange("Opaque Scene", wiProfiler::DOMAIN_GPU, threadID);

	wiRenderer::UpdateCameraCB(wiRenderer::getCamera(), threadID);

	rtMain.Activate(threadID, 0, 0, 0, 0);
	{
		wiRenderer::DrawWorld(wiRenderer::getCamera(), getTessellationEnabled(), threadID, SHADERTYPE_FORWARD, rtReflection.GetTexture(), getHairParticlesEnabled(), true);
		wiRenderer::DrawSky(threadID);
	}
	rtMain.Deactivate(threadID);
	wiRenderer::UpdateGBuffer(rtMain.GetTextureResolvedMSAA(threadID, 0), rtMain.GetTextureResolvedMSAA(threadID, 1), nullptr, nullptr, nullptr, threadID);

	dtDepthCopy.CopyFrom(*rtMain.depth, threadID);

	rtLinearDepth.Activate(threadID); {
		wiImageEffects fx;
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		fx.quality = QUALITY_NEAREST;
		fx.process.setLinDepth(true);
		wiImage::Draw(dtDepthCopy.GetTextureResolvedMSAA(threadID), fx, threadID);
	}
	rtLinearDepth.Deactivate(threadID);

	wiRenderer::UpdateDepthBuffer(dtDepthCopy.GetTextureResolvedMSAA(threadID), rtLinearDepth.GetTexture(), threadID);

	wiProfiler::GetInstance().EndRange(threadID); // Opaque Scene
}

wiDepthTarget* ForwardRenderableComponent::GetDepthBuffer()
{
	return rtMain.depth;
}

void ForwardRenderableComponent::setPreferredThreadingCount(unsigned short value)
{
	Renderable3DComponent::setPreferredThreadingCount(value);

	if (!wiRenderer::GetDevice()->CheckCapability(GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_MULTITHREADED_RENDERING))
	{
		if (value > 1)
			wiHelper::messageBox("Multithreaded rendering not supported by your hardware! Falling back to single threading!", "Caution");
		return;
	}


	switch (value) {
	case 2:
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderFrameSetUp(GRAPHICSTHREAD_REFLECTIONS);
			RenderReflections(GRAPHICSTHREAD_REFLECTIONS);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			wiRenderer::BindPersistentState(GRAPHICSTHREAD_SCENE);
			wiImage::BindPersistentState(GRAPHICSTHREAD_SCENE);
			RenderShadows(GRAPHICSTHREAD_SCENE);
			RenderScene(GRAPHICSTHREAD_SCENE);
			RenderSecondaryScene(rtMain, rtMain, GRAPHICSTHREAD_SCENE);
			RenderComposition(rtMain, rtMain, GRAPHICSTHREAD_SCENE);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_SCENE);
		}));
		break;
	case 3:
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderFrameSetUp(GRAPHICSTHREAD_REFLECTIONS);
			RenderReflections(GRAPHICSTHREAD_REFLECTIONS);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			wiRenderer::BindPersistentState(GRAPHICSTHREAD_SCENE);
			wiImage::BindPersistentState(GRAPHICSTHREAD_SCENE);
			RenderShadows(GRAPHICSTHREAD_SCENE);
			RenderScene(GRAPHICSTHREAD_SCENE);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_SCENE);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			wiRenderer::BindPersistentState(GRAPHICSTHREAD_MISC1);
			wiImage::BindPersistentState(GRAPHICSTHREAD_MISC1);
			wiRenderer::UpdateDepthBuffer(dtDepthCopy.GetTexture(), rtLinearDepth.GetTexture(), GRAPHICSTHREAD_MISC1);
			wiRenderer::UpdateGBuffer(rtMain.GetTexture(0), rtMain.GetTexture(1), rtMain.GetTexture(2), nullptr, nullptr, GRAPHICSTHREAD_MISC1);
			RenderSecondaryScene(rtMain, rtMain, GRAPHICSTHREAD_MISC1);
			RenderComposition(rtMain, rtMain, GRAPHICSTHREAD_MISC1);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_MISC1);
		}));
		break;
	case 4:
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderFrameSetUp(GRAPHICSTHREAD_REFLECTIONS);
			RenderReflections(GRAPHICSTHREAD_REFLECTIONS);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			wiRenderer::BindPersistentState(GRAPHICSTHREAD_SCENE);
			wiImage::BindPersistentState(GRAPHICSTHREAD_SCENE);
			RenderShadows(GRAPHICSTHREAD_SCENE);
			RenderScene(GRAPHICSTHREAD_SCENE);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_SCENE);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			wiRenderer::BindPersistentState(GRAPHICSTHREAD_MISC1);
			wiImage::BindPersistentState(GRAPHICSTHREAD_MISC1);
			wiRenderer::UpdateDepthBuffer(dtDepthCopy.GetTexture(), rtLinearDepth.GetTexture(), GRAPHICSTHREAD_MISC1);
			wiRenderer::UpdateGBuffer(rtMain.GetTexture(0), rtMain.GetTexture(1), rtMain.GetTexture(2), nullptr, nullptr, GRAPHICSTHREAD_MISC1);
			RenderSecondaryScene(rtMain, rtMain, GRAPHICSTHREAD_MISC1);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_MISC1);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			wiRenderer::BindPersistentState(GRAPHICSTHREAD_MISC2);
			wiImage::BindPersistentState(GRAPHICSTHREAD_MISC2);
			wiRenderer::UpdateDepthBuffer(dtDepthCopy.GetTexture(), rtLinearDepth.GetTexture(), GRAPHICSTHREAD_MISC2);
			wiRenderer::UpdateGBuffer(rtMain.GetTexture(0), rtMain.GetTexture(1), rtMain.GetTexture(2), nullptr, nullptr, GRAPHICSTHREAD_MISC2);
			RenderComposition(rtMain, rtMain, GRAPHICSTHREAD_MISC2);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_MISC2);
		}));
		break;
	};

}
