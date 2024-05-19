// workaround for a compilation issue 
// https://github.com/microsoft/STL/issues/2335#issuecomment-967306862
#define _STL_CRT_SECURE_INVALID_PARAMETER(expr) _CRT_SECURE_INVALID_PARAMETER(expr)

#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/plane.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>
#include <pxr/usd/usdLux/rectLight.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>
#include <pxr/base/gf/camera.h>
#include <pxr/base/tf/diagnosticMgr.h>
#include <pxr/base/gf/vec2i.h>
#include <pxr/imaging/glf/simpleLightingContext.h>
#include <pxr/imaging/glf/drawTarget.h>
#include <pxr/base/tf/notice.h>
#include <pxr/usd/usd/notice.h>
#include <pxr/imaging/glf/contextCaps.h>
#include <pxr/imaging/glf/glContext.h>
#include <pxr/imaging/glf/info.h>
#include <pxr/usd/usdLux/domeLight.h>
#include <pxr/usd/usdLux/domeLight_1.h>
#include <pxr/base/gf/bbox3d.h>
#include <pxr/usd/usdGeom/bboxCache.h>
#include <pxr/pxr.h>

#include <json/json.h>
#include <json/value.h>

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <map>

#include <pointfont.h>

#include <rapidsettings.h>

#define WIDTH 1024
#define HEIGHT 768

pxr::UsdStageRefPtr stage;

PointFont pointFont;
RapidSettings::Settings settings;

double baseSize = 0.0;
double lookAtDistance = 6.0;
double yaw = 7.0;
double pitch = 0.0;
double rollX = 0.0;
double rollY = 0.0;
double offsetX = 0.0;
double offsetY = 0.0;

int currentDelegate = 0;
int newDelegate = 0;

std::string currentFilename = "";
std::string newFilename = "./Pieta_A_3D_Tribute_to_Michelangelos_Masterpiece.usdz";

int angle = 0;
int frame = 0;
int frameStart = 0;
int frameEnd = 0;

// fullscreen holds the monitor to fullscreen on
int fullscreen = -1;

float eyesHeight = 0.0f;
float fov = 35.0f;

pxr::GfVec3d cameraPos(0, 0, 0);
pxr::GfVec3d cameraPivot(0, eyesHeight, 0);

int display_w = WIDTH;
int display_h = HEIGHT;
int window_w = WIDTH;
int window_h = HEIGHT;
int window_pos_x = 100;
int window_pos_y = 100;

pxr::GfBBox3d bbox_orig;

double maxHeight = 1.0;
bool showHelp = false;

void InitSettings()
{
    
    settings.addSwitch("skyDome", false, GLFW_KEY_L, "Toggle Sky Dome");
    settings.addSwitch("ceilingLight", true, GLFW_KEY_K, "Toggle Ceiling Light");
    settings.addSwitch("verticallyAligned", false, GLFW_KEY_V, "Toggle vertically aligned");
    settings.addSwitch("proxyPurpose", true, GLFW_KEY_B, "Toggles proxy purpose");
    settings.addSwitch("rotate", false, GLFW_KEY_SPACE, "Rotate");
    settings.addSwitch("animate", false, GLFW_KEY_P, "Playback animation");

    settings.addFloat("lightExposure", 1.0f, GLFW_KEY_E, "Ceiling light exposure");
    settings.addFloat("lightIntensity", 1.0f, GLFW_KEY_I, "Ceiling light intensity");
    settings.addFloat("lookAtDistanceMultiplier", 1.0f, GLFW_KEY_W, "CameraTarget Distance multiplier");
    settings.addFloat("marginWidthMultiplier", 2.0f, GLFW_KEY_Z, "Margin width multiplier");
    settings.addFloat("marginHeightMultiplier", 2.0f, GLFW_KEY_X, "Margin height multiplier");
    settings.addFloat("skyExposure", 0.0f, GLFW_KEY_T, "Sky Dome Exposure");
    settings.addFloat("fontSize", 1.0f, GLFW_KEY_J, "Font size");
    settings.addFloat("fontLineSpace", 1.5f, GLFW_KEY_G, "Font line space");
    settings.addFloat("cameraDistanceMultiplier", 0.0f, GLFW_KEY_A, "Camera Distance Multiplier");
}

void ReadSettings()
{
    Json::Value jsonRoot;
    std::string filename("settings.json");
    std::ifstream config_doc(filename.c_str(), std::ifstream::binary);
    if (config_doc.is_open())
    {
        config_doc >> jsonRoot;
        display_w = jsonRoot["display_w"].asInt();
        display_h = jsonRoot["display_h"].asInt();
        window_w = jsonRoot["window_w"].asInt();
        window_h = jsonRoot["window_h"].asInt();
        window_pos_x = std::max( 10, jsonRoot["window_pos_x"].asInt() ); // handle zero-ed via fullscreen
        window_pos_y = std::max( 10, jsonRoot["window_pos_y"].asInt() ); // handle zero-ed via fullscreen
        fullscreen = jsonRoot["fullscreen"].asInt();
        angle = jsonRoot["angle"].asInt();
        frame = jsonRoot["frame"].asInt();

        settings.InitializeFromJson(jsonRoot);

        newFilename = jsonRoot["newFilename"].asString();
        newDelegate = jsonRoot["newDelegate"].asInt();
    }
}

void SaveSettings()
{
    Json::Value jsonRoot;
    jsonRoot["display_w"] = display_w;
    jsonRoot["display_h"] = display_h;
    jsonRoot["window_w"] = window_w;
    jsonRoot["window_h"] = window_h;
    jsonRoot["window_pos_x"] = window_pos_x;
    jsonRoot["window_pos_y"] = window_pos_y;
    jsonRoot["fullscreen"] = fullscreen;

    settings.PopulateJson(jsonRoot);

    jsonRoot["angle"] = angle;
    jsonRoot["frame"] = frame;

    // NOTE: this takes currentFilename and saves it to newFilename
    //       so that when reading it at startup, it triggers a new
    //       stage to be loaded from the newFilename
    //       Same story for newDelegate
    jsonRoot["newFilename"] = currentFilename;
    jsonRoot["newDelegate"] = currentDelegate;

    std::string filename("settings.json");
    std::ofstream config_doc(filename.c_str(), std::ofstream::binary);
    config_doc << jsonRoot;
}

void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void ToggleFullscreen(GLFWwindow* window)
{
    int monitorsCount = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorsCount);
    if (fullscreen >= monitorsCount)
        fullscreen = -1;

    if (fullscreen >= 0)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(monitors[fullscreen]);
        glfwSetWindowMonitor(window, monitors[fullscreen], 0, 0, mode->width, mode->height, mode->refreshRate);
    }
    else
    {
        // this resets window position
        glfwSetWindowMonitor(window, NULL, 200, 200, window_w, window_h, 0);
    }
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwGetWindowPos(window, &window_pos_x, &window_pos_y);
        SaveSettings();
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

#define CHECK_FOR_KEY_DELEGATE(num) else if (key == GLFW_KEY_ ## num && action == GLFW_PRESS) { newDelegate = num; }
    CHECK_FOR_KEY_DELEGATE(0)
    CHECK_FOR_KEY_DELEGATE(1)
    CHECK_FOR_KEY_DELEGATE(2)
    CHECK_FOR_KEY_DELEGATE(3)
    CHECK_FOR_KEY_DELEGATE(4)
    CHECK_FOR_KEY_DELEGATE(5)
    CHECK_FOR_KEY_DELEGATE(6)
    CHECK_FOR_KEY_DELEGATE(7)
    CHECK_FOR_KEY_DELEGATE(8)
    CHECK_FOR_KEY_DELEGATE(9)
#undef CHECK_FOR_KEY_DELEGATE

    else if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        fullscreen++;
        ToggleFullscreen(window);
    }
    else if (key == GLFW_KEY_R)
    {
        settings.ResetToDefaults();
    }
    else if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        showHelp = !showHelp;
    }
    else if (key == GLFW_KEY_D && action == GLFW_PRESS)
    {
        stage->GetRootLayer()->Export("./temp.usda");
    }

    settings.KeyPressed(key, action);
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    int i;
    for (i = 0; i < count; i++)
    {
        // only first one, and it replaces current one
        newFilename = std::string(paths[i]);
        break;
    }
}

void mouse_button_callback(
    GLFWwindow* window,
    int button, int action, int mods)
{
    settings.MousePressed(button, action);
}

void mouse_cursor_callback(
    GLFWwindow* window,
    double xpos, double ypos)
{
    settings.MouseMoved(xpos, ypos);
}

void AddMeshPlane(
    pxr::UsdStageRefPtr i_stage, 
    pxr::SdfPath& i_path, 
    pxr::GfVec3d& i_pos, 
    pxr::GfVec3f& i_up,
    pxr::GfVec3f& i_scale,
    pxr::GfVec3f& i_color)
{
    pxr::UsdGeomMesh& mesh = pxr::UsdGeomMesh::Define(i_stage, i_path);
    mesh.CreateOrientationAttr().Set(pxr::UsdGeomTokens->leftHanded);
    mesh.CreateSubdivisionSchemeAttr().Set(pxr::UsdGeomTokens->none);

    pxr::VtArray<pxr::GfVec3f> points;
    points.emplace_back(pxr::GfVec3f(0.5, 0.0, 0.5));
    points.emplace_back(pxr::GfVec3f(0.5, 0.0, -0.5));
    points.emplace_back(pxr::GfVec3f(-0.5, 0.0, -0.5));
    points.emplace_back(pxr::GfVec3f(-0.5, 0.0, 0.5));
    mesh.CreatePointsAttr().Set(points);
    pxr::VtArray<pxr::GfVec3f> displayColors;
    displayColors.emplace_back(i_color);
    displayColors.emplace_back(i_color);
    displayColors.emplace_back(i_color);
    displayColors.emplace_back(i_color);
    pxr::UsdAttribute& displayColorsAttr = mesh.CreateDisplayColorAttr();
    displayColorsAttr.Set(displayColors);
    pxr::UsdGeomPrimvar displayColorsPrimvar(displayColorsAttr);
    displayColorsPrimvar.SetInterpolation(pxr::UsdGeomTokens->vertex);

    pxr::VtArray<int> faceVertexCounts = { 4 };
    mesh.CreateFaceVertexCountsAttr().Set(faceVertexCounts);
    pxr::VtArray<int> faceVertexIndices = { 0, 1, 2, 3 };
    mesh.CreateFaceVertexIndicesAttr().Set(faceVertexIndices);

    mesh.AddTranslateOp().Set(i_pos);
    mesh.AddRotateXYZOp().Set(i_up);
    mesh.AddScaleOp().Set(i_scale);
}

void CreateOrUpdateCornellBox(pxr::UsdStageRefPtr i_stage)
{
    // create or update a cornell-box environment
    // Not using the physical measurements here,
    // as we are deforming it based on window-size,
    // but here is the link to the cornell-box specs:
    // https://www.graphics.cornell.edu/online/box/data.html

    pxr::UsdPrim& cornellBox = i_stage->GetPrimAtPath(pxr::SdfPath("/cornellBox"));

    if (!cornellBox.IsValid())
    {
        pxr::TfTokenVector purposes;
        purposes.push_back(pxr::UsdGeomTokens->default_);
        purposes.push_back(pxr::UsdGeomTokens->guide);
        purposes.push_back(pxr::UsdGeomTokens->proxy);
        purposes.push_back(pxr::UsdGeomTokens->render);

        // Extent hints are sometimes authored as an optimization to avoid
        // computing bounds, they are particularly useful for some tests where
        // there is no bound on the first frame.
        bool useExtentHints = true;
        pxr::UsdGeomBBoxCache bboxCache(0.0f, purposes, useExtentHints);

        bbox_orig = bboxCache.ComputeWorldBound(i_stage->GetPseudoRoot());
        if (bbox_orig.GetBox().IsEmpty())
        {
            bbox_orig = pxr::GfBBox3d(pxr::GfRange3d(pxr::GfVec3d(-1, -1, -1), pxr::GfVec3d(1, 1, 1)));
        }
    }
    // get a cube that fits the bbox including offset from zero/origin
    pxr::GfRange3d range_orig = bbox_orig.ComputeAlignedRange();
    pxr::GfVec3d min_orig = range_orig.GetMin();
    pxr::GfVec3d max_orig = range_orig.GetMax();
    double maxWidth = 0.0;
    for (int c = 0; c < 8; ++c)
    {
        maxWidth = std::max(maxWidth, std::abs(range_orig.GetCorner(c).data()[0]));
        maxWidth = std::max(maxWidth, std::abs(range_orig.GetCorner(c).data()[2]));
    }
    maxHeight = std::max(maxWidth, max_orig[1] - min_orig[1]);
    // scale bbox to leave margins
    maxWidth *= double(settings.getFloat("marginWidthMultiplier"));
    maxHeight *= double(settings.getFloat("marginHeightMultiplier"));

    pxr::GfVec3d newMin = pxr::GfVec3d(-maxWidth, min_orig[1], -maxWidth);
    pxr::GfVec3d newMax = pxr::GfVec3d(maxWidth, min_orig[1] + maxHeight, maxWidth);
    // scale by window-ratio and bbox-ratio
    double wr = double(display_w) / double(display_h);
    double br = maxHeight / (maxWidth * 2.0);
    if (wr >= 1.0)
    {
        newMin = pxr::GfVec3d(newMin[0] * br * wr, newMin[1], newMin[2] * br * wr);
        newMax = pxr::GfVec3d(newMax[0] * br * wr, newMax[1], newMax[2] * br * wr);
    }
    else
    {
        wr = 1.0 / wr;
        newMin = pxr::GfVec3d(newMin[0] * br, newMin[1] * wr, newMin[2] * br);
        newMax = pxr::GfVec3d(newMax[0] * br, newMax[1] * wr, newMax[2] * br);
    }

    pxr::GfBBox3d bbox = pxr::GfBBox3d(pxr::GfRange3d(newMin, newMax));
    pxr::GfVec3d bboxSize = bbox.ComputeAlignedRange().GetSize();
    baseSize = std::max(bboxSize[0], bboxSize[2]);
    cameraPivot = bbox.ComputeCentroid();
    lookAtDistance = bboxSize.GetLength()*2.0*double(settings.getFloat("lookAtDistanceMultiplier"));
    fov = float( 2.0 * std::atan(bboxSize[1] * 0.5 / lookAtDistance) * 180.0/M_PI );
    offsetX = 0.0;
    offsetY = 0.0;
    rollX = 0.0;
    rollY = 0.0;
    pitch = 0.0;
    yaw = 0.0;

    
    if (!cornellBox.IsValid())
    {
        pxr::UsdGeomXform::Define(i_stage, pxr::SdfPath("/cornellBox"));

        AddMeshPlane(i_stage, pxr::SdfPath("/cornellBox/redPlane"),
            pxr::GfVec3d(cameraPivot[0] - baseSize / 2.0, cameraPivot[1], cameraPivot[2]),
            pxr::GfVec3f(0.0, 0.0, 90.0),
            pxr::GfVec3f(bboxSize[1], 1.0, baseSize),
            pxr::GfVec3f(1.0, 0, 0));

        AddMeshPlane(i_stage, pxr::SdfPath("/cornellBox/greenPlane"),
            pxr::GfVec3d(cameraPivot[0] + baseSize / 2.0, cameraPivot[1], cameraPivot[2]),
            pxr::GfVec3f(0.0, 0.0, -90.0),
            pxr::GfVec3f(bboxSize[1], 1.0, baseSize),
            pxr::GfVec3f(0, 1.0, 0));

        AddMeshPlane(i_stage, pxr::SdfPath("/cornellBox/whiteBackPlane"),
            pxr::GfVec3d(cameraPivot[0], cameraPivot[1], cameraPivot[2] - baseSize / 2.0),
            pxr::GfVec3f(-90, 0.0, 0.0),
            pxr::GfVec3f(baseSize, 1.0, bboxSize[1]),
            pxr::GfVec3f(1.0, 1.0, 1.0));

        //AddMeshPlane(i_stage, pxr::SdfPath("/cornellBox/whiteFrontPlane"), // behind camera
        //    pxr::GfVec3d(cameraPivot[0], cameraPivot[1], cameraPivot[2] + baseSize),
        //    pxr::GfVec3f(90, 0.0, 0.0),
        //    pxr::GfVec3f(baseSize, 1.0, bboxSize[1]),
        //    pxr::GfVec3f(1.0, 1.0, 1.0));


        AddMeshPlane(i_stage, pxr::SdfPath("/cornellBox/whiteTopPlane"),
            pxr::GfVec3d(cameraPivot[0], cameraPivot[1] + bboxSize[1] / 2.0, cameraPivot[2]),
            pxr::GfVec3f(0.0, 0.0, 0.0),
            pxr::GfVec3f(baseSize, 1.0, baseSize),
            pxr::GfVec3f(1.0, 1.0, 1.0));

        AddMeshPlane(i_stage, pxr::SdfPath("/cornellBox/whiteBottomPlane"),
            pxr::GfVec3d(cameraPivot[0], cameraPivot[1] - bboxSize[1] / 2.0, cameraPivot[2]),
            pxr::GfVec3f(180.0, 0.0, 0.0),
            pxr::GfVec3f(baseSize, 1.0, baseSize),
            pxr::GfVec3f(1.0, 1.0, 1.0));

        pxr::UsdLuxRectLight& light = pxr::UsdLuxRectLight::Define(i_stage, pxr::SdfPath("/cornellBox/ceilingLight"));
        pxr::UsdLuxLightAPI::Apply(light.GetPrim());
        light.CreateExposureAttr().Set(settings.getFloat("lightExposure"));
        light.CreateIntensityAttr().Set(settings.getFloat("lightIntensity"));
        light.CreateWidthAttr().Set(float(baseSize / 5.0));
        light.CreateHeightAttr().Set(float(baseSize / 5.0));
        light.AddTranslateOp().Set(pxr::GfVec3d(cameraPivot[0], cameraPivot[1] + (bboxSize[1] / 2.0) - 0.0001, cameraPivot[2]));
        light.AddRotateXOp().Set(-90.0f);
        light.GetPrim().SetActive(settings.getSwitch("ceilingLight"));

        // TODO: need to find a way to add a schema LightAPI
        pxr::UsdLuxDomeLight& skyDome = pxr::UsdLuxDomeLight::Define(i_stage, pxr::SdfPath("/cornellBox/sky"));
        pxr::UsdLuxLightAPI::Apply(skyDome.GetPrim());
        //skyDome.CreateTextureFormatAttr().Set(pxr::TfToken("latlong"));
        skyDome.CreateTextureFileAttr().Set(pxr::SdfAssetPath("sky_1k.hdr"));
        skyDome.CreateColorAttr().Set(pxr::GfVec3f(1,1,1));
        skyDome.CreateExposureAttr().Set(settings.getFloat("skyExposure"));
        skyDome.GetPrim().SetActive(settings.getSwitch("skyDome"));
    }

    pxr::UsdGeomMesh& redPlane = pxr::UsdGeomMesh( i_stage->GetPrimAtPath(pxr::SdfPath("/cornellBox/redPlane")) );
    redPlane.GetTranslateOp().Set(pxr::GfVec3d(cameraPivot[0] - baseSize / 2.0, cameraPivot[1], cameraPivot[2]));
    redPlane.GetScaleOp().Set(pxr::GfVec3f(bboxSize[1], 1.0, baseSize));

    pxr::UsdGeomMesh& greenPlane = pxr::UsdGeomMesh(i_stage->GetPrimAtPath(pxr::SdfPath("/cornellBox/greenPlane")));
    greenPlane.GetTranslateOp().Set(pxr::GfVec3d(cameraPivot[0] + baseSize / 2.0, cameraPivot[1], cameraPivot[2]));
    greenPlane.GetScaleOp().Set(pxr::GfVec3f(bboxSize[1], 1.0, baseSize));

    pxr::UsdGeomMesh& whiteBackPlane = pxr::UsdGeomMesh(i_stage->GetPrimAtPath(pxr::SdfPath("/cornellBox/whiteBackPlane")));
    whiteBackPlane.GetTranslateOp().Set(pxr::GfVec3d(cameraPivot[0], cameraPivot[1], cameraPivot[2] - baseSize / 2.0));
    whiteBackPlane.GetScaleOp().Set(pxr::GfVec3f(baseSize, 1.0, bboxSize[1]));

    //pxr::UsdGeomMesh& whiteFrontPlane = pxr::UsdGeomMesh(i_stage->GetPrimAtPath(pxr::SdfPath("/cornellBox/whiteFrontPlane")));
    //whiteFrontPlane.GetTranslateOp().Set(pxr::GfVec3d(cameraPivot[0], cameraPivot[1], cameraPivot[2] + baseSize));
    //whiteFrontPlane.GetScaleOp().Set(pxr::GfVec3f(baseSize, 1.0, bboxSize[1]));

    pxr::UsdGeomMesh& whiteTopPlane = pxr::UsdGeomMesh(i_stage->GetPrimAtPath(pxr::SdfPath("/cornellBox/whiteTopPlane")));
    whiteTopPlane.GetTranslateOp().Set(pxr::GfVec3d(cameraPivot[0], cameraPivot[1] + bboxSize[1] / 2.0, cameraPivot[2]));
    whiteTopPlane.GetScaleOp().Set(pxr::GfVec3f(baseSize, 1.0, baseSize));

    pxr::UsdGeomMesh& whiteBottomPlane = pxr::UsdGeomMesh(i_stage->GetPrimAtPath(pxr::SdfPath("/cornellBox/whiteBottomPlane")));
    whiteBottomPlane.GetTranslateOp().Set(pxr::GfVec3d(cameraPivot[0], cameraPivot[1] - bboxSize[1] / 2.0, cameraPivot[2]));
    whiteBottomPlane.GetScaleOp().Set(pxr::GfVec3f(baseSize, 1.0, baseSize));

    pxr::UsdLuxRectLight& light = pxr::UsdLuxRectLight(i_stage->GetPrimAtPath(pxr::SdfPath("/cornellBox/ceilingLight")));
    light.GetExposureAttr().Set(settings.getFloat("lightExposure"));
    light.GetIntensityAttr().Set(settings.getFloat("lightIntensity"));
    light.GetWidthAttr().Set(float(baseSize / 5.0));
    light.GetHeightAttr().Set(float(baseSize / 5.0));
    light.GetTranslateOp().Set(pxr::GfVec3d(cameraPivot[0], cameraPivot[1] + (bboxSize[1] / 2.0) - 0.0001, cameraPivot[2]));
    if (light.GetPrim().IsActive() != settings.getSwitch("ceilingLight"))
        light.GetPrim().SetActive(settings.getSwitch("ceilingLight"));

    pxr::UsdLuxDomeLight& skyDome = pxr::UsdLuxDomeLight(i_stage->GetPrimAtPath(pxr::SdfPath("/cornellBox/sky")));
    skyDome.GetExposureAttr().Set(settings.getFloat("skyExposure"));
    if( skyDome.GetPrim().IsActive() != settings.getSwitch("skyDome"))
        skyDome.GetPrim().SetActive(settings.getSwitch("skyDome"));
}

void AddMeshCube(pxr::UsdStageRefPtr i_stage, pxr::SdfPath& i_path, pxr::GfVec3d& i_pos)
{
    pxr::UsdGeomMesh& cubeMesh = pxr::UsdGeomMesh::Define(i_stage, i_path);
    cubeMesh.CreateOrientationAttr().Set(pxr::UsdGeomTokens->leftHanded);
    cubeMesh.CreateSubdivisionSchemeAttr().Set(pxr::UsdGeomTokens->none);

    pxr::VtArray<pxr::GfVec3f> points;
    points.emplace_back(pxr::GfVec3f(0.5, -0.5, 0.5));
    points.emplace_back(pxr::GfVec3f(-0.5, -0.5, 0.5));
    points.emplace_back(pxr::GfVec3f(0.5, 0.5, 0.5));
    points.emplace_back(pxr::GfVec3f(-0.5, 0.5, 0.5));
    points.emplace_back(pxr::GfVec3f(-0.5, -0.5, -0.5));
    points.emplace_back(pxr::GfVec3f(0.5, -0.5, -0.5));
    points.emplace_back(pxr::GfVec3f(-0.5, 0.5, -0.5));
    points.emplace_back(pxr::GfVec3f(0.5, 0.5, -0.5));
    cubeMesh.CreatePointsAttr().Set(points);
    pxr::VtArray<pxr::GfVec3f> displayColors;
    displayColors.emplace_back(pxr::GfVec3f(1, 0, 1));
    displayColors.emplace_back(pxr::GfVec3f(0, 0, 1));
    displayColors.emplace_back(pxr::GfVec3f(1, 1, 1));
    displayColors.emplace_back(pxr::GfVec3f(0, 1, 1));
    displayColors.emplace_back(pxr::GfVec3f(0, 0, 0));
    displayColors.emplace_back(pxr::GfVec3f(1, 0, 0));
    displayColors.emplace_back(pxr::GfVec3f(0, 1, 0));
    displayColors.emplace_back(pxr::GfVec3f(1, 1, 0));
    pxr::UsdAttribute& displayColorsAttr = cubeMesh.CreateDisplayColorAttr();
    displayColorsAttr.Set(displayColors);
    pxr::UsdGeomPrimvar displayColorsPrimvar(displayColorsAttr);
    displayColorsPrimvar.SetInterpolation(pxr::UsdGeomTokens->vertex);

    pxr::VtArray<int> faceVertexCounts = { 4, 4, 4, 4, 4, 4 };
    cubeMesh.CreateFaceVertexCountsAttr().Set(faceVertexCounts);
    pxr::VtArray<int> faceVertexIndices = { 0, 1, 3, 2, 4, 5, 7, 6, 6, 7, 2, 3, 5, 4, 1, 0, 5, 0, 2, 7, 1, 4, 6, 3 };
    cubeMesh.CreateFaceVertexIndicesAttr().Set(faceVertexIndices);
    
    cubeMesh.AddTranslateOp().Set(i_pos);
}

int main(int argc, char** argv)
{
    // be quiet...
    pxr::TfDiagnosticMgr::GetInstance().SetQuiet(true);

    InitSettings();
    ReadSettings();

	if (!glfwInit())
	{
		std::cout << "Failed initializing glfw" << std::endl;
		return 1;
	}
    glfwSetErrorCallback(error_callback);

    GLFWwindow* window = glfwCreateWindow(window_w, window_h, "usdCornellBox", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed creating a glfw window" << std::endl;
        return 1;
    }
    ToggleFullscreen(window);
    glfwSetWindowPos(window, window_pos_x, window_pos_y);

    glfwSetKeyCallback(window, key_callback);
    glfwSetDropCallback(window, drop_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, mouse_cursor_callback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    pxr::GlfContextCaps::InitInstance();

    std::unique_ptr<class pxr::UsdImagingGLEngine> engine;
    pxr::GfCamera camera;
    pxr::GfMatrix4d cameraTransform;

    pxr::GfMatrix4d viewMatrix;
    pxr::GfMatrix4d projectionMatrix;
    pxr::GfFrustum frustum;

    pxr::UsdImagingGLRenderParams renderParams;

    // we handle input file in the same way as if it
    // was dropped in the viewer, so here we are just
    // going to create a stage in memory, with an empty
    // cornell-box
    //
    stage = pxr::UsdStage::CreateInMemory();

    // create cornell-box with default size
    //
    CreateOrUpdateCornellBox(stage);

    pxr::GfVec4f clearColor(0.18f, 0.18f, 0.18f, 1.0f);

    std::vector< pxr::UsdGeomXformOp > ops;

    while (!glfwWindowShouldClose(window))
    {
        if (settings.getSwitch("animate"))
            frame = frame > frameEnd ? frameStart : frame + 1;
        if (settings.getSwitch("rotate"))
            angle++;

        glfwMakeContextCurrent(window);

        glfwPollEvents();

        // get display size (inner display buffer)
        //
        glfwGetFramebufferSize(window, &display_w, &display_h);

        settings.UpdateDisplaySize(display_w, display_h);

        // get full-window size (borders included)
        //
        glfwGetWindowSize(window, &window_w, &window_h);

        // check for engine and build it in the render call
        //
        if (!engine)
        {
            pxr::SdfPathVector excludedPaths;
            engine.reset(new pxr::UsdImagingGLEngine(
                stage->GetPseudoRoot().GetPath(), excludedPaths));

            auto& renderDelegates = engine->GetRendererPlugins();
            engine->SetRendererPlugin(renderDelegates[currentDelegate]);
        }

        if (currentFilename != newFilename)
        {
            currentFilename = newFilename;

            // we are going to reset the stage now
            
            // load the input file as is...
            // NOTE: we could load the input file as reference on a prim
            //       and just rotate that prim
            //       but we need to handle default prims, and it will
            //       also change the fullpaths of the prims, compare
            //       to what the user will expect. So, we just load the file
            //       and do the dance of the root-prims and transform ops.
            //
            stage = pxr::UsdStage::Open(currentFilename);
            stage->Load(pxr::SdfPath::AbsoluteRootPath());

            pxr::VtValue startTimeCode;
            stage->GetMetadata(pxr::TfToken("startTimeCode"), &startTimeCode);
            frameStart = int(startTimeCode.Get<double>());
            pxr::VtValue endTimeCode;
            stage->GetMetadata(pxr::TfToken("endTimeCode"), &endTimeCode);
            frameEnd = int(endTimeCode.Get<double>());

            // find all root prims, and create transform ops
            // so that we know what we need to rotate
            //
            for (auto& p : stage->GetPseudoRoot().GetChildren())
            {
                pxr::UsdGeomXform(p).AddRotateYOp(pxr::UsdGeomXformOp::PrecisionFloat, pxr::TfToken("spinning"));
                pxr::UsdGeomXform(p).AddTranslateOp(pxr::UsdGeomXformOp::PrecisionDouble, pxr::TfToken("verticalOffset"));
            }

            // now create the cornell-box fitting bbox of the entire scene
            //
            CreateOrUpdateCornellBox(stage);

            // reset the engine with the new stage
            //
            engine.reset();
        }

        if (!engine)
            continue;

        if (newDelegate != currentDelegate)
        {
            currentDelegate = newDelegate;
            auto& renderDelegates = engine->GetRendererPlugins();
            engine->SetRendererPlugin(renderDelegates[currentDelegate]);
        }

        CreateOrUpdateCornellBox(stage);

        // update all transform ops if any
        //
        for (auto& p : stage->GetPseudoRoot().GetChildren())
        {
            if (p.GetName() == "cornellBox")
                continue;
            pxr::UsdGeomXform(p).GetRotateYOp(pxr::TfToken("spinning")).Set(float(angle));
            if(settings.getSwitch("verticallyAligned"))
                pxr::UsdGeomXform(p).GetTranslateOp(pxr::TfToken("verticalOffset")).Set(pxr::GfVec3d(0,(maxHeight/2.0)-bbox_orig.GetRange().GetSize()[1]/2.0, 0));
            else
                pxr::UsdGeomXform(p).GetTranslateOp(pxr::TfToken("verticalOffset")).Set(pxr::GfVec3d(0, 0, 0));
        }

        cameraTransform.SetIdentity();
        cameraTransform *= pxr::GfMatrix4d().SetTranslate(pxr::GfVec3d(-offsetX, -offsetY, baseSize*settings.getFloat("cameraDistanceMultiplier")));
        cameraTransform *= pxr::GfMatrix4d().SetTranslate(pxr::GfVec3d(0, 0, lookAtDistance));
        cameraTransform *= pxr::GfMatrix4d().SetRotate(pxr::GfRotation(pxr::GfVec3d(0, 0, 1), -rollX * 5.0));
        cameraTransform *= pxr::GfMatrix4d().SetRotate(pxr::GfRotation(pxr::GfVec3d(1, 0, 0), -yaw * 5.0));
        cameraTransform *= pxr::GfMatrix4d().SetRotate(pxr::GfRotation(pxr::GfVec3d(0, 1, 0), -pitch * 5.0));
        cameraTransform *= pxr::GfMatrix4d().SetRotate(pxr::GfRotation(pxr::GfVec3d(0, 0, 1), -rollY * 5.0));
        cameraTransform *= pxr::GfMatrix4d().SetTranslate(cameraPivot);

        camera.SetTransform(cameraTransform);
        frustum = camera.GetFrustum();
        double fovy = fov;
        double znear = 0.5;
        double zfar = 100000.0;
        const double aspectRatio = double(display_w) / double(display_h);
        frustum.SetPerspective(fovy, aspectRatio, znear, zfar);
        projectionMatrix = frustum.ComputeProjectionMatrix();
        viewMatrix = frustum.ComputeViewMatrix();
        cameraPos = viewMatrix.Transform(pxr::GfVec3d(0, 0, 0));

        glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // setup viewport for 3d scene render
        glPushMatrix();
        {
            glViewport(0, 0, display_w, display_h);
            pxr::GfVec4d viewport(0, 0, display_w, display_h);

            // scene has lighting
            glEnable(GL_LIGHTING);
            glEnable(GL_LIGHT0);

            // Set engine properties and parameters
            engine->SetRenderBufferSize(pxr::GfVec2i(window_w, window_h));
            engine->SetSelectionColor(pxr::GfVec4f(1.0, 1.0, 0.0, 1.0));
            engine->SetCameraState(viewMatrix, projectionMatrix);
            engine->SetRenderViewport(viewport);
            engine->SetEnablePresentation(true);

            // update render params
            renderParams.frame = frame;
            renderParams.enableLighting = true;
            renderParams.enableSceneLights = true;
            renderParams.enableSceneMaterials = true;
            renderParams.cullStyle = pxr::UsdImagingGLCullStyle::CULL_STYLE_BACK;
            renderParams.showProxy = settings.getSwitch("proxyPurpose");
            renderParams.showRender = !settings.getSwitch("proxyPurpose");
            renderParams.showGuides = false;
            renderParams.forceRefresh = false;
            renderParams.highlight = false;
            renderParams.enableUsdDrawModes = false;
            renderParams.drawMode = pxr::UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
            renderParams.gammaCorrectColors = true;
            renderParams.clearColor = clearColor;
            renderParams.enableIdRender = false;
            renderParams.enableSampleAlphaToCoverage = false;
            renderParams.complexity = 1.0f;

            // render all paths from root
            engine->Render(stage->GetPseudoRoot(), renderParams);
        }
        glPopMatrix();

        if (showHelp)
        {
            glClear(GL_DEPTH_BUFFER_BIT);

            // HUD
            glDisable(GL_LIGHTING);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glEnable(GL_LINE_SMOOTH);
            glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glPushMatrix();
            {
                glViewport(0, 0, display_w, display_h);
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();

                // draw text
                pointFont.reset();
                pointFont.setLineSpace(settings.getFloat("fontLineSpace"));
                pointFont.setPixelSize(settings.getFloat("fontSize"));
                pointFont.setDisplaySize(display_w, display_h);

                pointFont.drawText("ABOUT");
                pointFont.drawText("");
                pointFont.drawText("usdCornellBox autogenerates a cornell-box around bounds of drag&dropped usd-file.");
                pointFont.drawText("");
                pointFont.drawText("All multipliers are affecting input stage bounds, changing size of the cornell-box.");
                pointFont.drawText("Input stage scale isn't changed, the cornell-box is scaled to fit.");
                pointFont.drawText("With rotation-enabled, all root-prims are affected by a RotateYOp.");
                pointFont.drawText("Changing window size will re-fit the cornell-box to always cover the whole area.");
                pointFont.drawText("");
                pointFont.drawText("Add your custom-plugins in the usd-extra folder to load them (delegates,procedurals,sceneindex,etc).");
                pointFont.drawText("");
                pointFont.drawText("Launch it, adjust settings, make it fullscreen, ");
                pointFont.drawText("save settings and use it as screensaver rotating your favouring usd model.");
                pointFont.drawText("");
                pointFont.drawText("Available keys:");
                pointFont.drawText("          R : Reset values");
                pointFont.drawText("          F : Toggles fullscreen (rotating across all monitors)");
                for (RapidSettings::SettingsSwitches::iterator it = settings.getSwitches().begin(); it != settings.getSwitches().end(); ++it)
                {
                    pointFont.drawText(std::string("          ") + it->second.getHelpText(true));
                }
                for (RapidSettings::SettingsFloats::iterator it = settings.getFloats().begin(); it != settings.getFloats().end(); ++it)
                {
                    pointFont.drawText(std::string("          ") + it->second.getHelpText(), it->second.status > 0);
                }
                pointFont.drawText("          D : Save current scene to ./temp.usda");
                pointFont.drawText("        Esc : Quit and save settings");
                pointFont.drawText("");
                pointFont.drawText("paoloemilioselva@gmail.com for any comment/feedback");
                pointFont.drawText("");
                pointFont.drawText(std::string("Available Hydra Delegates:"));
                auto& renderDelegates = engine->GetRendererPlugins();
                for (size_t i = 0; i < renderDelegates.size(); ++i)
                {
                    const std::string currDelegate = "  [" + std::to_string(i) + "] "
                        + engine->GetRendererDisplayName(renderDelegates[i]);
                    pointFont.drawText(currDelegate, currentDelegate == i);
                }
            }
            glPopMatrix();
        }

        // Keep running
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

	return 0;
}