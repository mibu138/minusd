#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/imaging/hd/engine.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <iostream>

using namespace pxr;

typedef GLXContext (*glXCreateContextAttribsARBProc)
    (Display*, GLXFBConfig, GLXContext, Bool, const int*);

int main(int argc, char *argv[])
{
    auto stage = UsdStage::Open("../stage.usda");
    for (const auto& prim : stage->Traverse()) 
    {
        std::cout << prim.GetPath() << '\n';
    }

    // init gl context

    Display* disp = 0;
    Window win = 0;

    disp = XOpenDisplay(0);
    win  = XCreateSimpleWindow(disp, XDefaultRootWindow(disp), 600, 100, 800, 800, 0, 0, 0);

    assert(disp);
    assert(win);

    const char* winname = "floating";

     XChangeProperty( disp, win,
        XInternAtom(disp, "_NET_WM_NAME", False),
        XInternAtom(disp, "UTF8_STRING", False),
        8, PropModeReplace, (const unsigned char*)winname,
        strlen(winname));   

    static int visual_attribs[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, true,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        None
    };

    std::cout << "Disp: " << disp << '\n';
    std::cout << "win:  " << win << '\n';

    int defaultScreen = XDefaultScreen(disp);
    std::cout << "default screen: " << defaultScreen << '\n';
    int num_fbc = 0;
    GLXFBConfig* fbc = glXChooseFBConfig(disp, DefaultScreen(disp), visual_attribs, &num_fbc);

    assert(fbc);

    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");

    assert(glXCreateContextAttribsARB);

    static int context_attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
        GLX_CONTEXT_MINOR_VERSION_ARB, 5,
        None
    };

    GLXContext ctx = glXCreateContextAttribsARB(disp, fbc[0], NULL, true, context_attribs);

    assert(ctx);

    std::cout << "GL Context created successfully" << '\n';

    XMapWindow(disp, win);
    glXMakeCurrent(disp, win, ctx);

    // end init gl context

    UsdImagingGLEngine glEngine;
    auto curRender = glEngine.GetCurrentRendererId();
    std::cout << curRender << '\n';


    const UsdPrim* cameraPrim = nullptr;
    for (const auto& prim : stage->Traverse()) 
    {
        if (prim.GetTypeName() == "Camera")
        {
            cameraPrim = &prim;
            std::cout << "Camera found: " << cameraPrim->GetPath() << "\n";
            break;
        }
    }
    if (!cameraPrim)
    {
        std::cout << "No camera found on stage" << '\n';
        exit(1);
    }
    glEngine.SetCameraPath(cameraPrim->GetPath());

    GfCamera cam = UsdGeomCamera(*cameraPrim).GetCamera(1);
    const GfFrustum frustum = cam.GetFrustum();
    const GfVec3d cameraPos = frustum.GetPosition();

    const GfVec4f SCENE_AMBIENT(0.01f, 0.01f, 0.01f, 1.0f);
    const GfVec4f SPECULAR_DEFAULT(0.1f, 0.1f, 0.1f, 1.0f);
    const GfVec4f AMBIENT_DEFAULT(0.2f, 0.2f, 0.2f, 1.0f);
    const float   SHININESS_DEFAULT(32.0);

    GlfSimpleLight cameraLight(
        GfVec4f(cameraPos[0], cameraPos[1], cameraPos[2], 1.0f));
    cameraLight.SetAmbient(SCENE_AMBIENT);

    const GlfSimpleLightVector lights({cameraLight});

    // Make default material and lighting match usdview's defaults... we expect 
    // GlfSimpleMaterial to go away soon, so not worth refactoring for sharing
    GlfSimpleMaterial material;
    material.SetAmbient(AMBIENT_DEFAULT);
    material.SetSpecular(SPECULAR_DEFAULT);
    material.SetShininess(SHININESS_DEFAULT);

    glEngine.SetLightingState(lights, material, SCENE_AMBIENT);

    glEngine.SetRendererAov(HdAovTokens->color);
    glEngine.SetCameraState(
        frustum.ComputeViewMatrix(),
        frustum.ComputeProjectionMatrix());
    //glEngine.SetCameraPath(cameraPrim->GetPath());
    glEngine.SetRenderViewport(GfVec4d(0, 0, 800, 800));
    const GfVec4f CLEAR_COLOR(0.0f);

    glEnable(GL_DEPTH_TEST);
    //glViewport(0, 0, 800, 800);
    glClearColor(1, 0, 0, 1);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    const GLfloat CLEAR_DEPTH[1] = { 1.0f };
    const UsdPrim& pseudoRoot = stage->GetPseudoRoot();

    UsdImagingGLRenderParams renderParams;

    glEngine.SetEnablePresentation(true);

    float frame = 0.0;
    std::cout << "Hydra enabled: " << (glEngine.IsHydraEnabled() ? "yes" : "no") << '\n';
    while (1) 
    {
        renderParams.frame = frame;
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEngine.Render(pseudoRoot, renderParams);
        frame += 1.0;
        glXSwapBuffers(disp, win);
        usleep(20000);
    }


    //HgiTextureHandle color = glEngine.GetAovTexture(HdAovTokens->color);

    
    return 0;
}
