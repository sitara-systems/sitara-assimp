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

using namespace mndl;

class BasicAssimpExampleApp : public App {
  public:
	void prepareSettings(Settings* settings);
	void setup() override;
	void update() override;
	void draw() override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	assimp::AssimpLoader mAssimpLoader;

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

void BasicAssimpExampleApp::prepareSettings(Settings* settings)
{
	settings->setWindowSize(800, 600);
}

void BasicAssimpExampleApp::setup()
{
	mAssimpLoader = assimp::AssimpLoader(getAssetPath("astroboy_walk.dae"));
	mAssimpLoader.setAnimation(0);

	mCamera.setPerspective(60, getWindowAspectRatio(), 0.1f, 100.0f);
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

	double time = fmod(getElapsedSeconds(), mAssimpLoader.getAnimationDuration(0));
	mAssimpLoader.setTime(time);
	mAssimpLoader.update();

	mFps = getAverageFps();
}

void BasicAssimpExampleApp::draw()
{
	gl::clear(Color::black());

	gl::setMatrices(mCamera);

	gl::enableDepthWrite();
	gl::enableDepthRead();

	gl::color(Color::white());

	if (mEnableWireframe) {
		gl::enableWireframe();
	}

	/*
	ci::gl::Light light(gl::Light::DIRECTIONAL, 0);
	light.setAmbient(Color::white());
	light.setDiffuse(Color::white());
	light.setSpecular(Color::white());
	light.lookAt(vec3(0, 5, -20), vec3(0, 5, 0));
	light.update(mMayaCam);
	light.enable();
	*/

	gl::enable(GL_LIGHTING);
	gl::enable(GL_NORMALIZE);
	mAssimpLoader.draw();
	gl::disable(GL_LIGHTING);

	if (mEnableWireframe)
		gl::disableWireframe();

	if (mDrawBBox)
		gl::drawStrokedCube(mAssimpLoader.getBoundingBox());

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

CINDER_APP(BasicAssimpExampleApp, RendererGl, [=](cinder::app::App::Settings* settings) {
	settings->setConsoleWindowEnabled();
	})
