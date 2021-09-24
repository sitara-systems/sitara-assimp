#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"
#include "cinder/CameraUi.h"
#include "cinder/Timeline.h"
#include "AssimpLoader.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class BasicAssimpExampleApp : public App {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void mouseWheel(MouseEvent event) override;
	sitara::assimp::AssimpLoader mAssimpLoader;

	ci::CameraUi mCameraUi;
	ci::CameraPersp mCamera;
	ci::Anim<ci::vec2> mMousePos;

	ci::params::InterfaceGl mParams;
	bool mEnableTextures;
	bool mEnableWireframe;
	bool mEnableSkinning;
	bool mEnableAnimation;
	bool mDrawBBox;
	float mFps;
};

void BasicAssimpExampleApp::setup()
{
	mAssimpLoader = sitara::assimp::AssimpLoader(getAssetPath("astroboy_walk.dae"));

	mCamera.setPerspective(60, getWindowAspectRatio(), 1.0f, 20000.0f);
	mCamera.lookAt(vec3(0.0f, 7.0f, 20.0), vec3(0.0, 7.0, 0.0), vec3(0, 1, 0));
	mCameraUi = CameraUi(&mCamera);

	mParams = params::InterfaceGl("Parameters", vec2(200, 300));
	mEnableWireframe = false;
	mParams.addParam("Wireframe", &mEnableWireframe);
	mEnableTextures = true;
	mParams.addParam("Textures", &mEnableTextures);
	mEnableSkinning = true;
	mParams.addParam("Skinning", &mEnableSkinning);
	mEnableAnimation = false;
	mParams.addParam("Animation", &mEnableAnimation);
	mDrawBBox = false;
	mParams.addParam("Bounding box", &mDrawBBox);
	mParams.addSeparator();
	mParams.addParam("Fps", &mFps, "", true);
}

void BasicAssimpExampleApp::update()
{
	mAssimpLoader.enableTextures(mEnableTextures);
	mAssimpLoader.enableSkinning(mEnableSkinning);
	mAssimpLoader.enableAnimation(mEnableAnimation);
	mAssimpLoader.disableMaterials();

	/*
	double time = fmod(getElapsedSeconds(), mAssimpLoader.getAnimationDuration(0));
	mAssimpLoader.setTime(time);
	mAssimpLoader.update();
	*/

	mFps = getAverageFps();
}

void BasicAssimpExampleApp::draw() {
	ci::gl::clear(Color::black());

	ci::gl::setMatrices(mCamera);

	ci::gl::enableDepthWrite();
	ci::gl::enableDepthRead();

	ci::gl::color(Color::white());

	if (mEnableWireframe) {
		ci::gl::enableWireframe();
	}

	mAssimpLoader.draw();

	if (mEnableWireframe) {
		ci::gl::disableWireframe();
	}

	if (mDrawBBox) {
		ci::gl::drawStrokedCube(mAssimpLoader.getBoundingBox());
	}

	ci::gl::disableDepthRead();
	ci::gl::disableDepthWrite();

	mParams.draw();
}

void BasicAssimpExampleApp::mouseDown(MouseEvent event) {
	mMousePos.stop();
	mMousePos = event.getPos();
	mCameraUi.mouseDown(mMousePos);
}

void BasicAssimpExampleApp::mouseDrag(MouseEvent event) {
	timeline().apply(&mMousePos, vec2(event.getPos()), 1.0f, EaseOutQuint()).updateFn([=]
		{
			mCameraUi.mouseDrag(mMousePos(), event.isLeftDown(), false, event.isRightDown());
		});
}

void BasicAssimpExampleApp::mouseWheel(MouseEvent event) {
	mCameraUi.mouseWheel(event.getWheelIncrement());
}

CINDER_APP(BasicAssimpExampleApp, RendererGl, [=](cinder::app::App::Settings* settings) {
	settings->setConsoleWindowEnabled();
	settings->setWindowSize(1920, 1080);
	})