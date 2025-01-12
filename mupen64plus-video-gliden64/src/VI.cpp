#include "GLideN64.h"
#include <math.h>
#include "Types.h"
#include "VI.h"
#include "OpenGL.h"
#include "N64.h"
#include "gSP.h"
#include "gDP.h"
#include "RSP.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "Config.h"
#include "Debug.h"

using namespace std;

VIInfo VI;

void VI_UpdateSize()
{
	const f32 xScale = _FIXED2FLOAT( _SHIFTR( *gfx_info.VI_X_SCALE, 0, 12 ), 10 );
//	f32 xOffset = _FIXED2FLOAT( _SHIFTR( *gfx_info.VI_X_SCALE, 16, 12 ), 10 );

	const u32 vScale = _SHIFTR(*gfx_info.VI_Y_SCALE, 0, 12);
//	f32 yOffset = _FIXED2FLOAT( _SHIFTR( *gfx_info.VI_Y_SCALE, 16, 12 ), 10 );

	const u32 hEnd = _SHIFTR( *gfx_info.VI_H_START, 0, 10 );
	const u32 hStart = _SHIFTR( *gfx_info.VI_H_START, 16, 10 );

	// These are in half-lines, so shift an extra bit
	const u32 vEnd = _SHIFTR( *gfx_info.VI_V_START, 0, 10 );
	const u32 vStart = _SHIFTR( *gfx_info.VI_V_START, 16, 10 );
	const bool interlacedPrev = VI.interlaced;
	if (VI.width > 0)
		VI.widthPrev = VI.width;

	VI.real_height = vEnd > vStart ? (((vEnd - vStart) >> 1) * vScale) >> 10 : 0;
	VI.width = *gfx_info.VI_WIDTH;
	VI.interlaced = (*gfx_info.VI_STATUS & 0x40) != 0;
	if (VI.interlaced) {
		f32 fullWidth = 640.0f*xScale;
		if (*gfx_info.VI_WIDTH > fullWidth) {
			const u32 scale = (u32)floorf(*gfx_info.VI_WIDTH / fullWidth + 0.5f);
			VI.width /= scale;
			VI.real_height *= scale;
		}
		if (VI.real_height % 2 == 1)
			--VI.real_height;
	}

	VI.PAL = (*gfx_info.VI_V_SYNC & 0x3ff) > 550;
	if (VI.PAL && (vEnd - vStart) > 478) {
		VI.height = (u32)(VI.real_height*1.0041841f);
		if (VI.height > 576)
			VI.height = VI.real_height = 576;
	}
	else {
		VI.height = (u32)(VI.real_height*1.0126582f);
		if (VI.height > 480)
			VI.height = VI.real_height = 480;
	}
	if (VI.height % 2 == 1)
		--VI.height;

//	const int fsaa = ((*gfx_info.VI_STATUS) >> 8) & 3;
//	const int divot = ((*gfx_info.VI_STATUS) >> 4) & 1;
	FrameBufferList & fbList = frameBufferList();
	FrameBuffer * pBuffer = fbList.findBuffer(VI.lastOrigin);
	DepthBuffer * pDepthBuffer = pBuffer != NULL ? pBuffer->m_pDepthBuffer : NULL;
	if (config.frameBufferEmulation.enable && ((config.generalEmulation.hacks & hack_skipVIChangeCheck) == 0) &&
		((interlacedPrev != VI.interlaced) ||
		(VI.width > 0 && VI.width != VI.widthPrev) ||
		(!VI.interlaced && pDepthBuffer != NULL && pDepthBuffer->m_width != VI.width) ||
		((config.generalEmulation.hacks & hack_ignoreVIHeightChange) == 0 && pBuffer != NULL && pBuffer->m_height != VI.height))
	) {
		fbList.removeBuffers(VI.widthPrev);
		fbList.removeBuffers(VI.width);
		depthBufferList().destroy();
		depthBufferList().init();
	}

	VI.rwidth = VI.width != 0 ? 1.0f / VI.width : 0.0f;
	VI.rheight = VI.height != 0 ? 1.0f / VI.height : 0.0f;
}

void VI_UpdateScreen()
{
	if (VI.lastOrigin == -1) // Workaround for Mupen64Plus issue with initialization
		isGLError();

	if (ConfigOpen)
		return;

	OGLVideo & ogl = video();
	if (ogl.changeWindow())
		return;
	if (ogl.resizeWindow())
		return;
	ogl.saveScreenshot();

	bool bVIUpdated = false;
	if (*gfx_info.VI_ORIGIN_REG != VI.lastOrigin) {
		VI_UpdateSize();
		bVIUpdated = true;
		ogl.updateScale();
	}

	if (config.frameBufferEmulation.enable) {

		FrameBuffer * pBuffer = frameBufferList().findBuffer(*gfx_info.VI_ORIGIN_REG);
		if (pBuffer == NULL)
			gDP.changed |= CHANGED_CPU_FB_WRITE;
		else if (!pBuffer->isValid()) {
			gDP.changed |= CHANGED_CPU_FB_WRITE;
			if (config.frameBufferEmulation.copyToRDRAM == 0)
				pBuffer->copyRdram();
		}

		const bool bCFB = (gDP.changed&CHANGED_CPU_FB_WRITE) == CHANGED_CPU_FB_WRITE;
		bool bNeedSwap = false;
		switch (config.frameBufferEmulation.bufferSwapMode) {
		case Config::bsOnVerticalInterrupt:
			bNeedSwap = true;
			break;
		case Config::bsOnVIOriginChange:
			bNeedSwap = bCFB ? true : (*gfx_info.VI_ORIGIN_REG != VI.lastOrigin);
			break;
		case Config::bsOnColorImageChange:
			bNeedSwap = bCFB ? true : (gDP.colorImage.changed != 0);
			break;
		}

		if (bNeedSwap) {
			if (bCFB) {
				if (pBuffer == NULL || pBuffer->m_width != VI.width) {
					if (!bVIUpdated) {
						VI_UpdateSize();
						ogl.updateScale();
						bVIUpdated = true;
					}
					const u32 size = *gfx_info.VI_STATUS & 3;
					if (VI.height > 0 && size > G_IM_SIZ_8b  && VI.width > 0)
						frameBufferList().saveBuffer(*gfx_info.VI_ORIGIN_REG, G_IM_FMT_RGBA, size, VI.width, VI.height, true);
				}
			}
			if ((((*gfx_info.VI_STATUS) & 3) > 0) && ((config.frameBufferEmulation.copyFromRDRAM && gDP.colorImage.changed) || bCFB)) {
				if (!bVIUpdated) {
					VI_UpdateSize();
					bVIUpdated = true;
				}
				FrameBuffer_CopyFromRDRAM(*gfx_info.VI_ORIGIN_REG, config.frameBufferEmulation.copyFromRDRAM && !bCFB);
			}
			frameBufferList().renderBuffer(*gfx_info.VI_ORIGIN_REG);
			frameBufferList().clearBuffersChanged();
			VI.lastOrigin = *gfx_info.VI_ORIGIN_REG;
#ifdef DEBUG
			while (Debug.paused && !Debug.step);
			Debug.step = FALSE;
#endif
		} 
	}
	else {
		if (gDP.changed & CHANGED_COLORBUFFER) {
			ogl.swapBuffers();
			gDP.changed &= ~CHANGED_COLORBUFFER;
#ifdef DEBUG
			while (Debug.paused && !Debug.step);
			Debug.step = FALSE;
#endif
			VI.lastOrigin = *gfx_info.VI_ORIGIN_REG;
		}
	}

	if (VI.lastOrigin == -1) { // Workaround for Mupen64Plus issue with initialization
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}
}
