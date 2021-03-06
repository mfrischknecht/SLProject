//#############################################################################
//  File:      ARSceneView.h
//  Purpose:   Augmented Reality Demo
//  Author:    Michael Goettlicher
//  Date:      Spring 2016
//  Codestyle: https://github.com/cpvrlab/SLProject/wiki/Coding-Style-Guidelines
//  Copyright: Marcus Hudritsch, Michael Goettlicher
//             This software is provide under the GNU General Public License
//             Please visit: http://opensource.org/licenses/GPL-3.0
//#############################################################################

#ifdef SL_MEMLEAKDETECT       // set in SL.h for debug config only
#include <debug_new.h>        // memory leak detector
#endif

#include "ARSceneView.h"

#include <SLBox.h>
#include <SLLightSpot.h>
#include <ARTracker.h>
#include <SLAssimpImporter.h>
#include <SLImage.h>
#include <SLTexFont.h>
#include <SLText.h>
#include <SLCVCapture.h>

#include <ARChessboardTracker.h>
#include <ARArucoTracker.h>
#include <AR2DMapper.h>
#include <AR2DTracker.h>

#include <GLFW/glfw3.h>
#include <sstream>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

using namespace cv;

//-----------------------------------------------------------------------------
extern GLFWwindow* window;
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void SLScene::onLoad(SLSceneView* sv, SLCommand cmd)
{
    init();    

    //setup camera
    SLCamera* cam1 = new SLCamera;

    float fov = 1.0f;
    if(ARSceneView* arSV = dynamic_cast<ARSceneView*>(sv))
        fov = _activeCalib->cameraFovDeg();

    cam1->fov(fov);
    cam1->clipNear(0.01f);
    cam1->clipFar(10.0f);

    //initial translation: will be overwritten as soon as first camera pose is estimated in ARTracker
    cam1->translate(0,0,0.5f);

    //set video image as background texture
    cam1->background().texture(&_videoTexture);
    _videoType = VT_MAIN;

    SLLightSpot* light1 = new SLLightSpot(0.3f);
    light1->translation(0,0,10);

    SLNode* scene = new SLNode;
    scene->addChild(light1);
    scene->addChild(cam1);

    _root3D = scene;

    sv->camera(cam1);
    sv->showMenu(false);
    sv->waitEvents(false);
    sv->onInitialize();
}
//-----------------------------------------------------------------------------
ARSceneView::ARSceneView() :
    _tracker(nullptr),
    _infoText(nullptr),
    _infoBottomText(nullptr),
    _newMode(Idle),
    _currMode(Idle)
{
}
//-----------------------------------------------------------------------------
ARSceneView::~ARSceneView()
{
    if(_tracker) delete _tracker; _tracker = nullptr;
    if(_infoText) delete _infoText; _infoText = nullptr;
    if(_infoBottomText) delete _infoBottomText; _infoBottomText = nullptr;
}
//-----------------------------------------------------------------------------
void ARSceneView::postSceneLoad()
{
    updateInfoText();
}
//-----------------------------------------------------------------------------
void ARSceneView::setCVImageToTexture(cv::Mat& image)
{
    SLScene::current->videoTexture()->copyVideoImage(image.cols,
                                                     image.rows,
                                                     SLCVCapture::format,
                                                     image.data,
                                                     image.isContinuous(),
                                                     true);
}
//-----------------------------------------------------------------------------
void ARSceneView::preDraw()
{
    SLScene* s = SLScene::current;

    if(_tracker)
    {
        if(!SLCVCapture::lastFrame.empty())
        {   
            if(_currMode != ARSceneViewMode::Idle || 
               _currMode != ARSceneViewMode::CalibrationMode)
            {   _tracker->track(SLCVCapture::lastFrame, s->activeCalib());
                _tracker->updateSceneView(this);
            }
        }

        //show undistorted image
        if(s->activeCalib()->showUndistorted())
        {   Mat undistorted;
            undistort(SLCVCapture::lastFrame,
                      undistorted,
                      s->activeCalib()->cameraMat(),
                      s->activeCalib()->distortion());
            setCVImageToTexture(undistorted);
        }
    }
    else if(_currMode == CalibrationMode)
    {
        if (s->activeCalib()->state() == CS_calibrateStream ||
            s->activeCalib()->state() == CS_calibrateGrab)
        {               
            s->activeCalib()->findChessboard(SLCVCapture::lastFrame,
                                             SLCVCapture::lastFrameGray,
                                             true);

            int imgsToCap = s->activeCalib()->numImgsToCapture();
            int imgsCaped = s->activeCalib()->numCapturedImgs();

            //update Info line
            stringstream ss;
            if(imgsCaped < imgsToCap)
                ss << "Click on the screen to create a calibration foto. Created " 
                   << imgsCaped << " of " << imgsToCap;
            else
            {   ss << "Calculating, please wait ...";
                s->activeCalib()->calculate();
            }

            setInfoLineText(ss.str());

            setCVImageToTexture(SLCVCapture::lastFrame);
        }
        else if(s->activeCalib()->state() == CS_calibrated)
        {
            float reprojError = s->activeCalib()->reprojectionError();
            this->camera()->fov(s->activeCalib()->cameraFovDeg());
            //update Info line
            std::stringstream ss;
            ss << "Calibrated: Reprojection error: " << reprojError;
            setInfoLineText(ss.str());
        }
    }
    else if(_currMode == Mapper2D)
    {
        //undistort image for map creation
        Mat undistorted;
        undistort(SLCVCapture::lastFrame, 
                  undistorted, 
                  s->activeCalib()->cameraMat(),
                  s->activeCalib()->distortion());

        if(_mapper2D.stateIsLineInput())
        {
            //update info line
            String msg = "Insert reference width of captured image in m: " + _mapper2D.getRefWidthStr();
            setInfoLineText(msg);
        }
        else if(_mapper2D.stateIsCapture())
        {
            //simulate a snapshot
			//Problem: bitwise not is good for visual feedback but we dont want to use it to create the map
            //cv::bitwise_not(undistorted, undistorted);

            //use image to generate a new mapping
            _mapper2D.createMap(undistorted, 0.0f, 0.0f, "map2d", AR2DMap::AR_ORB);
            _mapper2D.state(AR2DMapper::Mapper2DState::IDLE);
        }
        else if(_mapper2D.stateIsIdle())
        {
            setInfoLineText("Press 'l' to create a new map.");
        }
        //set image
        setCVImageToTexture(undistorted);
    }
}
//-----------------------------------------------------------------------------
void ARSceneView::postDraw()
{
    renderText();
}
//-----------------------------------------------------------------------------
void ARSceneView::updateInfoText()
{
    if (_infoText) delete _infoText;
    if (_infoBottomText) delete _infoBottomText;

    SLchar m[2550];   // message character array
    m[0]=0;           // set zero length

    SLstring modes;
    modes = "Mode selection: \\n";
    modes += "c: Calibrate \\n";
    modes += "0: Tracking disabled \\n";
    modes += "1: Track chessboard \\n";
    modes += "2: Track ArUco markers \\n";
    modes += "3: Track 2D map \\n";
    modes += "4: Enter 2D mapping mode (press 'l' for snapshot)\\n";

    SLstring modeName;
    switch (_newMode)
    {   case CalibrationMode:   modeName = "Calibration Mode"; break;
        case Idle:              modeName = "Tracking Disabled Mode"; break;
        case ChessboardMode:    modeName = "Chessboard Tracking Mode"; break;
        case ArucoMode:         modeName = "Aruco Tracking Mode"; break;
        case Mapper2D:          modeName = "2D Mapping Mode"; break;
        case Tracker2D:         modeName = "2D Tracking Mode"; break;
    }

    sprintf(m+strlen(m), "%s", modes.c_str());

    string title = modeName;
    glfwSetWindowTitle(window, title.c_str());

    SLTexFont* f = SLTexFont::getFont(1.2f, SL::dpi);
    _infoText = new SLText(m, f, SLCol4f::BLACK, (SLfloat)_scrW, 1.0f);
    _infoText->translate(10.0f, -_infoText->size().y-5.0f, 0.0f, TS_object);

    if(_infoLine.size())
    {
        //update bottom info line
        SLchar info[2550];   // message character array
        info[0]=0;           // set zero length

        sprintf(info+strlen(info), "%s", _infoLine.c_str());

        SLTexFont* fi = SLTexFont::getFont(2.4f, SL::dpi);
        _infoBottomText = new SLText(info, fi, SLCol4f::RED, (SLfloat)_scrW, 1.0f);
        _infoBottomText->translate(10.0f, -_infoBottomText->size().y-5.0f, 0.0f, TS_object);
    }
}
//-----------------------------------------------------------------------------
void ARSceneView::renderText()
{
    if (!_infoText)
        return;

    SLScene* s = SLScene::current;
    SLfloat w2 = (SLfloat)_scrWdiv2;
    SLfloat h2 = (SLfloat)_scrHdiv2;
    SLfloat depth = 0.9f;               // Render depth between -1 & 1

    _stateGL->depthMask(false);         // Freeze depth buffer for blending
    _stateGL->depthTest(false);         // Disable depth testing
    _stateGL->blend(true);              // Enable blending
    _stateGL->polygonLine(false);       // Only filled polygons

    // Set orthographic projection with 0,0,0 in the screen center
    _stateGL->projectionMatrix.ortho(-w2, w2,-h2, h2, 1.0f, -1.0f);

    // Set viewport over entire screen
    _stateGL->viewport(0, 0, _scrW, _scrH);

    _stateGL->modelViewMatrix.identity();
    _stateGL->modelViewMatrix.translate(-w2, h2, depth);
    _stateGL->modelViewMatrix.multiply(_infoText->om());
    _infoText->drawRec(this);

    _stateGL->blend(false);       // turn off blending
    _stateGL->depthMask(true);    // enable depth buffer writing
    _stateGL->depthTest(true);    // enable depth testing
    GET_GL_ERROR;                 // check if any OGL errors occured

    if (!_infoBottomText)
        return;

    _stateGL->depthMask(false);         // Freeze depth buffer for blending
    _stateGL->depthTest(false);         // Disable depth testing
    _stateGL->blend(true);              // Enable blending
    _stateGL->polygonLine(false);       // Only filled polygons

    // Set orthographic projection with 0,0,0 in the screen center
    _stateGL->projectionMatrix.ortho(-w2, w2,-h2, h2, 1.0f, -1.0f);

    // Set viewport over entire screen
    _stateGL->viewport(0, 0, _scrW, _scrH);

    _stateGL->modelViewMatrix.identity();
    _stateGL->modelViewMatrix.translate(-w2, -h2+50.0f, depth);
    _stateGL->modelViewMatrix.multiply(_infoBottomText->om());
    _infoBottomText->drawRec(this);

    _stateGL->blend(false);       // turn off blending
    _stateGL->depthMask(true);    // enable depth buffer writing
    _stateGL->depthTest(true);    // enable depth testing
    GET_GL_ERROR;                 // check if any OGL errors occured
}
//-----------------------------------------------------------------------------
void ARSceneView::processModeChange()
{
    SLScene* s = SLScene::current;

    //check if mode has changed
    if(_newMode != _currMode)
    {
        if(_tracker)
        {   _tracker->unloadSGObjects();
            delete _tracker;
            _tracker = nullptr;
        }

        //try to init this mode
        switch(_newMode)
        {
            case ARSceneViewMode::Idle:
                clearInfoLine();
                break;

            case ARSceneViewMode::CalibrationMode:
                //execute calibration
                if(s->activeCalib()->loadCalibParams())
                {   s->activeCalib()->clear();
                    s->activeCalib()->state(CS_calibrateStream);
                }
                else setInfoLineText("Info: Could not load calibration parameter file.");
                break;

            case ARSceneViewMode::ChessboardMode:
                if(s->activeCalib()->state() == CS_uncalibrated)
                {   setInfoLineText("Info: System uncalibrated. Perform camera calibration.");
                    break;
                }
                clearInfoLine();
                _tracker = new ARChessboardTracker();
                if(!_tracker->init())
                    _newMode = ARSceneViewMode::ArucoMode;
                break;

            case ARSceneViewMode::ArucoMode:
                if(s->activeCalib()->state() == CS_uncalibrated)
                {   setInfoLineText("Info: System uncalibrated. Perform camera calibration.");
                    break;
                }
                clearInfoLine();
                _tracker = new ARArucoTracker();
                if(!_tracker->init())
                    _newMode = ARSceneViewMode::Idle;
                break;

            case ARSceneViewMode::Tracker2D:
                if(s->activeCalib()->state() == CS_uncalibrated)
                {   setInfoLineText("Info: System uncalibrated. Perform camera calibration.");
                    break;
                }
                clearInfoLine();
                _tracker = new AR2DTracker();
                if(!_tracker->init())
                    _newMode = ARSceneViewMode::Idle;
                break;

            case ARSceneViewMode::Mapper2D:
                if(s->activeCalib()->state() == CS_uncalibrated)
                    setInfoLineText("Info: System uncalibrated. Perform camera calibration.");
                break;
        }

        //at last set _oldMode to _curMode
        _currMode = _newMode;
        updateInfoText();
    }
}
//-----------------------------------------------------------------------------
SLbool ARSceneView::onKeyPress(const SLKey key, const SLKey mod)
{
    SLScene* s = SLScene::current;

    if(_currMode == ARSceneViewMode::Mapper2D && _mapper2D.stateIsLineInput())
    {
        switch(key)
        {   case '0': _mapper2D.addDigit("0"); break;
            case '1': _mapper2D.addDigit("1"); break;
            case '2': _mapper2D.addDigit("2"); break;
            case '3': _mapper2D.addDigit("3"); break;
            case '4': _mapper2D.addDigit("4"); break;
            case '5': _mapper2D.addDigit("5"); break;
            case '6': _mapper2D.addDigit("6"); break;
            case '7': _mapper2D.addDigit("7"); break;
            case '8': _mapper2D.addDigit("8"); break;
            case '9': _mapper2D.addDigit("9"); break;
            case '.': _mapper2D.addDigit("."); break;
            case SLKey::K_backspace: _mapper2D.removeLastDigit(); break;
            case SLKey::K_enter: _mapper2D.state(AR2DMapper::IDLE); break;
        }
    } else
    {   switch(key)
        {   case 'C': _newMode = ARSceneViewMode::CalibrationMode; break;
            case '0': _newMode = ARSceneViewMode::Idle; break;
            case '1': _newMode = ARSceneViewMode::ChessboardMode; break;
            case '2': _newMode = ARSceneViewMode::ArucoMode; break;
            case '3': _newMode = ARSceneViewMode::Tracker2D; break;
            case '4': _newMode = ARSceneViewMode::Mapper2D;
                      _mapper2D.clear();
                      _mapper2D.state(AR2DMapper::LINE_INPUT); break;
            case 'L': _mapper2D.state(AR2DMapper::CAPTURE); break;
            case 'U': s->activeCalib()->showUndistorted(!s->activeCalib()->showUndistorted()); break;
        }
    }

    processModeChange();
    return true;
}
//-----------------------------------------------------------------------------
void ARSceneView::clearInfoLine()
{
    _infoLine = "";
    delete _infoBottomText;
    _infoBottomText = nullptr;
}
//-----------------------------------------------------------------------------
void ARSceneView::setInfoLineText(SLstring text)
{
    if(text != _infoLine)
    {
        _infoLine = text;
        updateInfoText();
    }
}
//-----------------------------------------------------------------------------
