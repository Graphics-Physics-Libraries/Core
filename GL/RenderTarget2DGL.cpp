#include "../Engine.h"
#include "RenderTarget2DGL.h"
#include "../common/gl.h"
#include "../base/BitMask.h"
#include "../Graphics.h"
#include "../image/Texture.h"
#include "Texture2DGL.h"
#include "CubeTextureGL.h"

namespace Core {

    RenderTarget2DGL::RenderTarget2DGL(Bool hasColor, Bool hasDepth, Bool enableStencilBuffer,
                                       const TextureAttributes& colorTextureAttributes, 
                                       const TextureAttributes& depthTextureAttributes, Vector2u size,
                                       Int32 initialFBOID) :
        RenderTarget2D(hasColor, hasDepth, enableStencilBuffer, colorTextureAttributes, depthTextureAttributes, size), RenderTargetGL(initialFBOID) {

    }

    RenderTarget2DGL::~RenderTarget2DGL() {
       if(this->colorTexture) Graphics::safeReleaseObject(this->colorTexture);
       if(this->depthTexture) Graphics::safeReleaseObject(this->depthTexture);
    }

    /*
     * Perform all initialization for this render target. This render target will not
     * be valid until this method successfully completes.
     */
    Bool RenderTarget2DGL::init() {

        this->initFramebuffer();

        // generate a color texture attachment
        // TODO: For now we are only supporting a texture type color attachment
        if (this->hasColorBuffer) {
            this->colorTexture = Engine::instance()->createTexture2D(this->colorTextureAttributes);
            this->buildAndVerifyTexture(this->colorTexture);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->colorTexture->getTextureID(), 0);
        }

        // generate a depth texture attachment
        // TODO: For now we are only supporting a texture type depth attachment if a stencil attachment is not included
        if (this->hasDepthBuffer && !this->enableStencilBuffer) {
            this->depthTexture = Engine::instance()->createTexture2D(this->depthTextureAttributes);
            this->buildAndVerifyTexture(this->depthTexture);
            
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->depthTexture->getTextureID(), 0);
        }
        else if (this->hasDepthBuffer && this->enableStencilBuffer) {
            this->initDepthStencilBufferCombo(this->size.x, this->size.y);
        }

        this->completeFramebuffer();
        return true;
    }

    void RenderTarget2DGL::destroyColorBuffer() {
        if (this->hasColorBuffer) {
            if (this->colorTexture) {
                WeakPointer<Texture2D> texture = WeakPointer<Texture>::dynamicPointerCast<Texture2D>(this->colorTexture);
                Graphics::safeReleaseObject(texture);
                this->colorTexture = WeakPointer<Texture>::nullPtr();
            }
        }
    }

    void RenderTarget2DGL::destroyDepthBuffer() {
        if (this->hasDepthBuffer) {
            if (!this->enableStencilBuffer) {
                if (this->depthTexture) {
                    WeakPointer<Texture2D> texture = WeakPointer<Texture>::dynamicPointerCast<Texture2D>(this->depthTexture);
                    Graphics::safeReleaseObject(texture);
                    this->depthTexture = WeakPointer<Texture>::nullPtr();
                }
            }
            else if (this->hasDepthBuffer && this->enableStencilBuffer) {
                this->destroyDepthStencilBufferCombo();
            }
        }
    }

}
