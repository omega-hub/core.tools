/******************************************************************************
 * THE OMEGA LIB PROJECT
 *-----------------------------------------------------------------------------
 * Copyright 2010-2015		Electronic Visualization Laboratory, 
 *							University of Illinois at Chicago
 * Authors:										
 *  Alessandro Febretti		febret@gmail.com
 *-----------------------------------------------------------------------------
 * Copyright (c) 2010-2015, Electronic Visualization Laboratory,  
 * University of Illinois at Chicago
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * Redistributions of source code must retain the above copyright notice, this 
 * list of conditions and the following disclaimer. Redistributions in binary 
 * form must reproduce the above copyright notice, this list of conditions and 
 * the following disclaimer in the documentation and/or other materials provided 
 * with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *-----------------------------------------------------------------------------
 * oimg
 *	A basic image viewer / stereo pattern generator tool
 ******************************************************************************/
#include <omega.h>
#include <omegaGl.h>

using namespace omega;

bool sDrawStereoPattern = false;
Color sLeftColor("red");
Color sRightColor("blue");

enum ImageMode { ImageModeCenter, ImageModeStretch, ImageModeTile };
ImageMode sImageMode;
String sImageModeString = "";
String sImageName = "";
double sScale = 1.0;
bool sRepeat = false;

////////////////////////////////////////////////////////////////////////////////
class OImgApplication : public EngineModule
{
public:
    OImgApplication() : EngineModule("OImgApplication") { enableSharedData(); }
    void commitSharedData(SharedOStream& out);
    void updateSharedData(SharedIStream& in);
    virtual void initialize();
    virtual void initializeRenderer(Renderer* r);

public:
    Ref<PixelData> image;
};

////////////////////////////////////////////////////////////////////////////////
class OImgRenderPass: public RenderPass
{
public:
    OImgRenderPass(Renderer* client, OImgApplication* app) : 
        RenderPass(client, "OImgRenderPass"),
        myApp(app)
    { }

    virtual void render(Renderer* client, const DrawContext& context);

private:
    OImgApplication* myApp;
};

////////////////////////////////////////////////////////////////////////////////
void OImgApplication::commitSharedData(SharedOStream& out)
{
    if(sImageModeString == "center") sImageMode = ImageModeCenter;
    else if(sImageModeString == "tile") sImageMode = ImageModeTile;
    else if(sImageModeString == "stretch") sImageMode = ImageModeStretch;

    // Sent input args to slave nodes, then disable data sharing
    out << sImageName << sDrawStereoPattern << sImageMode << sScale << sRepeat;
    disableSharedData();
}

////////////////////////////////////////////////////////////////////////////////
void OImgApplication::updateSharedData(SharedIStream& in)
{
    in >> sImageName >> sDrawStereoPattern >> sImageMode >> sScale >> sRepeat;
    // On slave nodes, sImageName will not be set at initialization time, 
    // so if needed load image here.
    if(sImageName != "")
    {
        image = ImageUtils::loadImage(sImageName);
    }
}

////////////////////////////////////////////////////////////////////////////////
void OImgApplication::initialize()
{
    if(sImageName != "")
    {
        image = ImageUtils::loadImage(sImageName);
    }
}

////////////////////////////////////////////////////////////////////////////////
void OImgApplication::initializeRenderer(Renderer* r)
{
    r->addRenderPass(new OImgRenderPass(r, this)); 
}

////////////////////////////////////////////////////////////////////////////////
void OImgRenderPass::render(Renderer* client, const DrawContext& context)
{
    if(context.task == DrawContext::OverlayDrawTask)
    {
        DrawInterface* di = client->getRenderer();
        di->beginDraw2D(context);

        Vector2f pos = context.tile->activeCanvasRect.min.cast<omicron::real>();
        Vector2f size = context.tile->activeCanvasRect.size().cast<omicron::real>();

        // Draw stereo pattern when enabled
        if(sDrawStereoPattern)
        {
            if(context.eye == DrawContext::EyeLeft)
            {
                di->drawRect(pos, size, sLeftColor);
            }
            else if(context.eye == DrawContext::EyeRight)
            {
                di->drawRect(pos, size, sRightColor);
            }
        }

        if(context.eye == DrawContext::EyeCyclop)
        {
            // Initialize texture
            if(myApp->image != NULL)
            {
                Texture* t = myApp->image->getTexture(context);
                glColor3f(1, 1, 1);
                if(sImageMode == ImageModeStretch)
                {
                    if(sRepeat)
                    {
                        //size = context.tile->displayConfig.getCanvasRect().size().cast<omicron::real>();
                        di->drawRectTexture(t, pos, size);
                    }
                    else
                    {
                        size = context.tile->displayConfig.getCanvasRect().size().cast<omicron::real>();
                        di->drawRectTexture(t, Vector2f::Zero(), size);
                    }
                }
                else if(sImageMode == ImageModeCenter)
                {
                    if(sRepeat)
                    {
                        Vector2f canvasSize = size;
                        size = Vector2f(t->getWidth(), t->getHeight()) * sScale;
                        pos += (canvasSize - size) / 2;
                        di->drawRectTexture(t, pos, size);
                    }
                    else
                    {
                        Vector2f canvasSize = context.tile->displayConfig.getCanvasRect().size().cast<omicron::real>();
                        size = Vector2f(t->getWidth(), t->getHeight()) * sScale;
                        pos = (canvasSize - size) / 2;
                        di->drawRectTexture(t, pos, size);
                    }
                }
                else if(sImageMode == ImageModeTile)
                {
                    // TODO: This is not completely correct and misses repeat
                    // mode. Finish implementation.
                    Vector2f canvasSize = context.tile->displayConfig.getCanvasRect().size().cast<omicron::real>();
                    Vector2f imgsize = Vector2f(t->getWidth(), t->getHeight()) * sScale;
                    Vector2f uvmin = Vector2f::Zero();
                    Vector2f uvmax = canvasSize.cwiseQuotient(imgsize.cast<omicron::real>());
                    di->drawRectTexture(t, pos, size, 0, uvmin, uvmax);
                }
            }
        }

        di->endDraw();
    }
}

////////////////////////////////////////////////////////////////////////////////
// ApplicationBase entry point
int main(int argc, char** argv)
{
    Application<OImgApplication> app("oimg");

    oargs().newNamedString('>', "img", "image-path", "image file path", sImageName);
    oargs().newNamedString('m', "mode", "Image mode", "image mode: one of stretch,tile,center. Default mode: center", sImageModeString);
    oargs().newNamedDouble('s', "scale", "image scale", "sets the image scale", sScale);
    oargs().newFlag('R', "repeat", "repeats the image on each node instead of stretching/tiling a single copy", sRepeat);
    oargs().newFlag('p', "pattern", "draw a stereo pattern", sDrawStereoPattern);

    return omain(app, argc, argv);
}
