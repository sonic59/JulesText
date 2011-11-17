/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-11 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

END_JUCE_NAMESPACE

@interface JuceGLView   : UIView
{
}
+ (Class) layerClass;
@end

@implementation JuceGLView
+ (Class) layerClass
{
    return [CAEAGLLayer class];
}
@end

BEGIN_JUCE_NAMESPACE

//==============================================================================
class GLESContext   : public OpenGLContext
{
public:
    GLESContext (UIView* parentView,
                 Component* const component_,
                 const OpenGLPixelFormat& pixelFormat,
                 const GLESContext* const sharedContext,
                 NSUInteger apiType)
        : component (component_), glLayer (nil), context (nil),
          useDepthBuffer (pixelFormat.depthBufferBits > 0),
          frameBufferHandle (0), colorBufferHandle (0),
          depthBufferHandle (0), lastWidth (0), lastHeight (0)
    {
        view = [[JuceGLView alloc] initWithFrame: CGRectMake (0, 0, 64, 64)];
        view.opaque = YES;
        view.hidden = NO;
        view.backgroundColor = [UIColor blackColor];
        view.userInteractionEnabled = NO;

        glLayer = (CAEAGLLayer*) [view layer];
        [parentView addSubview: view];

        if (sharedContext != nullptr)
            context = [[EAGLContext alloc] initWithAPI: apiType
                                            sharegroup: [sharedContext->context sharegroup]];
        else
            context = [[EAGLContext alloc] initWithAPI: apiType];

        createGLBuffers();
    }

    ~GLESContext()
    {
        deleteContext();

        [view removeFromSuperview];
        [view release];
        freeGLBuffers();
    }

    void deleteContext()
    {
        makeInactive();
        [context release];
        context = nil;
    }

    bool makeActive() const noexcept
    {
        jassert (context != nil);

        [EAGLContext setCurrentContext: context];
        glBindFramebufferOES (GL_FRAMEBUFFER_OES, frameBufferHandle);
        return true;
    }

    void swapBuffers()
    {
        glBindRenderbufferOES (GL_RENDERBUFFER_OES, colorBufferHandle);
        [context presentRenderbuffer: GL_RENDERBUFFER_OES];
    }

    bool makeInactive() const noexcept
    {
        return [EAGLContext setCurrentContext: nil];
    }

    bool isActive() const noexcept
    {
        return [EAGLContext currentContext] == context;
    }

    void* getRawContext() const noexcept            { return glLayer; }
    unsigned int getFrameBufferID() const           { return (unsigned int) frameBufferHandle; }

    void updateWindowPosition (const Rectangle<int>& bounds)
    {
        // For some strange reason, the view seems to fail unless its width is a multiple of 8...
        view.frame = CGRectMake ((CGFloat) bounds.getX(), (CGFloat) bounds.getY(),
                                 (CGFloat) (bounds.getWidth() & ~7),
                                 (CGFloat) bounds.getHeight());

        if (lastWidth != bounds.getWidth() || lastHeight != bounds.getHeight())
        {
            lastWidth = bounds.getWidth();
            lastHeight = bounds.getHeight();
            freeGLBuffers();
            createGLBuffers();
        }
    }

    bool setSwapInterval (const int numFramesPerSwap)
    {
        numFrames = numFramesPerSwap;
        return true;
    }

    int getSwapInterval() const
    {
        return numFrames;
    }

    //==============================================================================
    void createGLBuffers()
    {
        makeActive();

        glGenFramebuffersOES (1, &frameBufferHandle);
        glGenRenderbuffersOES (1, &colorBufferHandle);
        glGenRenderbuffersOES (1, &depthBufferHandle);

        glBindRenderbufferOES (GL_RENDERBUFFER_OES, colorBufferHandle);
        [context renderbufferStorage: GL_RENDERBUFFER_OES fromDrawable: glLayer];

        GLint width, height;
        glGetRenderbufferParameterivOES (GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &width);
        glGetRenderbufferParameterivOES (GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &height);

        if (useDepthBuffer)
        {
            glBindRenderbufferOES (GL_RENDERBUFFER_OES, depthBufferHandle);
            glRenderbufferStorageOES (GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, width, height);
        }

        glBindRenderbufferOES (GL_RENDERBUFFER_OES, colorBufferHandle);

        glBindFramebufferOES (GL_FRAMEBUFFER_OES, frameBufferHandle);
        glFramebufferRenderbufferOES (GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, colorBufferHandle);

        if (useDepthBuffer)
            glFramebufferRenderbufferOES (GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthBufferHandle);

        jassert (glCheckFramebufferStatusOES (GL_FRAMEBUFFER_OES) == GL_FRAMEBUFFER_COMPLETE_OES);
    }

    void freeGLBuffers()
    {
        if (frameBufferHandle != 0)
        {
            glDeleteFramebuffersOES (1, &frameBufferHandle);
            frameBufferHandle = 0;
        }

        if (colorBufferHandle != 0)
        {
            glDeleteRenderbuffersOES (1, &colorBufferHandle);
            colorBufferHandle = 0;
        }

        if (depthBufferHandle != 0)
        {
            glDeleteRenderbuffersOES (1, &depthBufferHandle);
            depthBufferHandle = 0;
        }
    }

    //==============================================================================
private:
    WeakReference<Component> component;
    JuceGLView* view;
    CAEAGLLayer* glLayer;
    EAGLContext* context;
    bool useDepthBuffer;
    GLuint frameBufferHandle, colorBufferHandle, depthBufferHandle;
    int numFrames;
    int lastWidth, lastHeight;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GLESContext);
};


OpenGLContext* OpenGLComponent::createContext()
{
    JUCE_AUTORELEASEPOOL
    ComponentPeer* peer = getPeer();

    if (peer != nullptr)
        return new GLESContext ((UIView*) peer->getNativeHandle(), this, preferredPixelFormat,
                                dynamic_cast <const GLESContext*> (contextToShareListsWith),
                                type == openGLES2 ? kEAGLRenderingAPIOpenGLES2 : kEAGLRenderingAPIOpenGLES1);

    return nullptr;
}

void OpenGLComponent::internalRepaint (int x, int y, int w, int h)
{
    Component::internalRepaint (x, y, w, h);
}

void OpenGLComponent::updateEmbeddedPosition (const Rectangle<int>& bounds)
{
    if (context != nullptr)
        static_cast <GLESContext*> (context.get())->updateWindowPosition (bounds);
}

//==============================================================================
bool OpenGLHelpers::isContextActive()
{
    return [EAGLContext currentContext] != nil;
}
